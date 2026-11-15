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
#include "core/object/callable_mp.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"

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
		default: return Rect2();
	}
	return Rect2(c.x - half, c.y - half, hs, hs);
}

ClockGearsUiCanvasOverlay::HandleType ClockGearsUiCanvasOverlay::_hit_test_handle(const Rect2 &p_elem_rect, const Vector2 &p_pt) const {
	if (!p_elem_rect.has_area()) {
		return HANDLE_NONE;
	}
	for (int h = HANDLE_TL; h <= HANDLE_BR; h++) {
		Rect2 hr = _get_handle_rect(p_elem_rect, static_cast<HandleType>(h)).grow(4.0f);
		if (hr.has_point(p_pt)) {
			return static_cast<HandleType>(h);
		}
	}
	return HANDLE_NONE;
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
		if (mb->get_button_index() == MouseButton::LEFT) {
			Vector2 pt = mb->get_position();
			if (mb->is_pressed()) {
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				Rect2 sel_r = get_selected_widget_rect();
				HandleType hh = _hit_test_handle(sel_r, pt);
				String hit_id = (hh != HANDLE_NONE) ? owner_editor->get_selected_element_id() : _hit_test_element(pt);
				if (hit_id.is_empty()) {
					owner_editor->select_element("");
					dragging = false;
					active_handle = HANDLE_NONE;
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
					dragging = !drag_initial_elem.is_empty();
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
				queue_redraw();
				accept_event();
				return;
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Vector2 pt = mm->get_position();
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

				Dictionary next_elem = drag_initial_elem.duplicate(true);
				if (active_handle == HANDLE_MOVE) {
					next_elem["offset_x"] = CLAMP(ox0 + dx, -ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_W);
					next_elem["offset_y"] = CLAMP(oy0 + dy, -ClockGearsUiDesignEditor::BASE_H, ClockGearsUiDesignEditor::BASE_H);
				} else {
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
						int(next_elem["width"]) != w0 || int(next_elem["height"]) != h0) {
					drag_changed = true;
				}
				dm->update_ui_element(owner_editor->get_current_screen(), drag_elem_id, next_elem, false);
				owner_editor->refresh_from_data_manager();
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

		// Marco sutil del área base 960x540
		draw_rect(stage, Color(0.95f, 0.72f, 0.30f, 0.35f), false, 1.0f);

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
			Color c = (eid == hover_elem_id) ? Color(0.45f, 0.78f, 1.0f, 0.85f) : Color(0.55f, 0.62f, 0.74f, 0.30f);
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
			}
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
	_build_ui();
}

ClockGearsUiDesignEditor::~ClockGearsUiDesignEditor() {
	if (test_window) {
		test_window->queue_free();
		test_window = nullptr;
	}
}

void ClockGearsUiDesignEditor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("design_modified"));

	ClassDB::bind_method(D_METHOD("refresh_from_data_manager"), &ClockGearsUiDesignEditor::refresh_from_data_manager);
	ClassDB::bind_method(D_METHOD("adapt_to_width", "window_width"), &ClockGearsUiDesignEditor::adapt_to_width);
	ClassDB::bind_method(D_METHOD("select_screen", "screen_id"), &ClockGearsUiDesignEditor::select_screen);
	ClassDB::bind_method(D_METHOD("get_current_screen"), &ClockGearsUiDesignEditor::get_current_screen);
	ClassDB::bind_method(D_METHOD("set_canvas_zoom", "zoom"), &ClockGearsUiDesignEditor::set_canvas_zoom);
	ClassDB::bind_method(D_METHOD("get_canvas_zoom"), &ClockGearsUiDesignEditor::get_canvas_zoom);
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
		}
	} else if (p_what == NOTIFICATION_RESIZED) {
		int w = int(get_size().x);
		if (w > 200) {
			_rebalance_responsive_layout(w);
		}
	}
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
	screen_opt->set_custom_minimum_size(Size2(220 * EDSCALE, row_h));
	Array screens = ClockGearsDataManager::get_ui_design_screens();
	for (int i = 0; i < screens.size(); i++) {
		Dictionary sd = screens[i];
		screen_opt->add_item(String(sd.get("label", "")), i);
		screen_opt->set_item_metadata(i, String(sd.get("id", "")));
	}
	screen_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_screen_selected));
	top_bar->add_child(screen_opt);

	HBoxContainer *zoom_hb = memnew(HBoxContainer);
	zoom_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_flow->add_child(zoom_hb);
	zoom_hb->add_child(memnew(Label("Zoom:")));
	for (int z = 1; z <= 3; z++) {
		Button *zb = memnew(Button(itos(z) + "x"));
		zb->set_toggle_mode(true);
		zb->set_pressed(z == canvas_zoom);
		zb->set_custom_minimum_size(Size2(42 * EDSCALE, row_h));
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

	reset_btn = memnew(Button(String::utf8("Restablecer a diseño por defecto")));
	reset_btn->set_custom_minimum_size(Size2(0, row_h));
	s_make_rounded_action_btn(reset_btn);
	reset_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_reset_pressed));
	act_hb->add_child(reset_btn);

	test_btn = memnew(Button("Probar"));
	test_btn->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
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
	wide_left_col->set_custom_minimum_size(Size2(204 * EDSCALE, 0));
	wide_left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_left_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	wide_hbox->add_child(wide_left_col);

	wide_center_col = memnew(VBoxContainer);
	wide_center_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_center_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_center_col->set_stretch_ratio(1.5f);
	wide_hbox->add_child(wide_center_col);

	wide_right_col = memnew(VBoxContainer);
	wide_right_col->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	wide_right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_right_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	wide_hbox->add_child(wide_right_col);

	compact_vbox = memnew(VBoxContainer);
	compact_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	compact_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	compact_vbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	compact_vbox->hide();
	layout_root->add_child(compact_vbox);

	compact_top_hbox = memnew(HBoxContainer);
	compact_top_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	compact_top_hbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	compact_vbox->add_child(compact_top_hbox);

	compact_bottom_hbox = memnew(HBoxContainer);
	compact_bottom_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	compact_bottom_hbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	compact_vbox->add_child(compact_bottom_hbox);

	// --- Tarjeta 1: Paleta lateral ---
	palette_card = memnew(PanelContainer);
	palette_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	wide_left_col->add_child(palette_card);

	VBoxContainer *pal_vb = memnew(VBoxContainer);
	pal_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	palette_card->add_child(pal_vb);

	Label *pal_hdr = memnew(Label("Elementos (arrastrar)"));
	pal_hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	pal_vb->add_child(pal_hdr);

	palette_grid = memnew(GridContainer);
	palette_grid->set_columns(1);
	palette_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	palette_grid->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	pal_vb->add_child(palette_grid);

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

	Label *can_hdr = memnew(Label("Lienzo 960x540 (vista idéntica al juego)"));
	can_hdr->set_text(String::utf8("Lienzo 960x540 (vista idéntica al juego)"));
	can_hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	can_vb->add_child(can_hdr);

	canvas_scroll = memnew(ScrollContainer);
	canvas_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_scroll->set_custom_minimum_size(Size2(BASE_W, BASE_H));
	canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	can_vb->add_child(canvas_scroll);

	canvas_stage = memnew(Control);
	canvas_stage->set_name("UiDesignerCanvasStage");
	canvas_stage->set_custom_minimum_size(Size2(BASE_W, BASE_H));
	canvas_stage->set_size(Size2(BASE_W, BASE_H));
	canvas_stage->set_clip_contents(true);
	canvas_scroll->add_child(canvas_stage);

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

	// Previews por defecto (solo visibles cuando !has_custom_ui_design)
	default_title_preview = memnew(ClockGearsTitleScreen);
	default_title_preview->set_preview_mode(true);
	default_title_preview->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	default_title_preview->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	default_title_preview->hide();
	canvas_stage->add_child(default_title_preview);

	default_gmenu_svc = memnew(SubViewportContainer);
	default_gmenu_svc->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	default_gmenu_svc->set_stretch(true);
	default_gmenu_svc->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	default_gmenu_svc->hide();
	canvas_stage->add_child(default_gmenu_svc);

	default_gmenu_vp = memnew(SubViewport);
	default_gmenu_vp->set_size(Size2i(BASE_W, BASE_H));
	default_gmenu_vp->set_disable_3d(true);
	default_gmenu_vp->set_transparent_background(true);
	default_gmenu_vp->set_update_mode(SubViewport::UPDATE_WHEN_VISIBLE);
	default_gmenu_svc->add_child(default_gmenu_vp);

	default_gmenu_preview = memnew(ClockGearsGameMenu);
	default_gmenu_preview->set_preview_mode(true);
	default_gmenu_preview->set_editor_test_popup_mode(true);
	default_gmenu_preview->hide();
	default_gmenu_vp->add_child(default_gmenu_preview);

	default_credits_preview = memnew(ClockGearsCreditsPlayer);
	default_credits_preview->set_preview_mode(true);
	default_credits_preview->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	default_credits_preview->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	default_credits_preview->hide();
	canvas_stage->add_child(default_credits_preview);

	// Mismo renderizador del juego (ClockGearsUiDesignLayer)
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

	// --- Tarjeta 3: Lista de capas con ojo y orden ---
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

	Label *lay_hdr = memnew(Label("Capas"));
	lay_hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	lay_vb->add_child(lay_hdr);

	HBoxContainer *lay_btns = memnew(HBoxContainer);
	lay_btns->add_theme_constant_override("separation", int(4 * EDSCALE));
	lay_vb->add_child(lay_btns);

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
	lay_scroll->set_custom_minimum_size(Size2(0, 150 * EDSCALE));
	lay_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	lay_vb->add_child(lay_scroll);

	layers_list_vbox = memnew(VBoxContainer);
	layers_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	layers_list_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	lay_scroll->add_child(layers_list_vbox);

	// --- Tarjeta 4: Panel de propiedades ---
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

	Label *props_hdr = memnew(Label("Propiedades"));
	props_hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	props_outer->add_child(props_hdr);

	props_sel_lbl = memnew(Label(String::utf8("Selecciona un elemento en el lienzo o las capas.")));
	props_sel_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	props_sel_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	props_outer->add_child(props_sel_lbl);

	ScrollContainer *props_scroll = memnew(ScrollContainer);
	props_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll->set_custom_minimum_size(Size2(250 * EDSCALE, 340 * EDSCALE));
	props_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	props_outer->add_child(props_scroll);

	props_fields_box = memnew(VBoxContainer);
	props_fields_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_fields_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	props_fields_box->hide();
	props_scroll->add_child(props_fields_box);

	GridContainer *g = memnew(GridContainer);
	g->set_columns(2);
	g->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	g->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	g->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	props_fields_box->add_child(g);

	auto add_lbl = [&](const String &p_t) {
		Label *l = memnew(Label(p_t));
		l->set_custom_minimum_size(Size2(92 * EDSCALE, 0));
		g->add_child(l);
	};

	add_lbl("Nombre:");
	prop_name_edit = memnew(LineEdit);
	prop_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_name_edit->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	prop_name_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	g->add_child(prop_name_edit);

	add_lbl("Anclaje:");
	GridContainer *anc_grid = memnew(GridContainer);
	anc_grid->set_columns(3);
	anc_grid->add_theme_constant_override("h_separation", int(3 * EDSCALE));
	anc_grid->add_theme_constant_override("v_separation", int(3 * EDSCALE));
	g->add_child(anc_grid);

	static const char *k_anc_glyphs[9] = { "NO", "N", "NE", "O", "C", "E", "SO", "S", "SE" };
	for (int i = 0; i < 9; i++) {
		Button *ab = memnew(Button(k_anc_glyphs[i]));
		ab->set_toggle_mode(true);
		ab->set_custom_minimum_size(Size2(34 * EDSCALE, 24 * EDSCALE));
		s_make_rounded_action_btn(ab);
		ab->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_anchor_btn_pressed).bind(i));
		anc_grid->add_child(ab);
		anchor_btns[i] = ab;
	}

	auto make_spin = [&](const String &p_lbl, int p_min, int p_max, SpinBox *&r_sp) {
		add_lbl(p_lbl);
		r_sp = memnew(SpinBox);
		r_sp->set_min(p_min);
		r_sp->set_max(p_max);
		r_sp->set_step(1);
		r_sp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_sp->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
		r_sp->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
		g->add_child(r_sp);
	};

	make_spin("Desplaz. X:", -960, 960, prop_ox_spin);
	make_spin("Desplaz. Y:", -540, 540, prop_oy_spin);
	make_spin("Ancho:", 16, 960, prop_w_spin);
	make_spin("Alto:", 16, 540, prop_h_spin);
	make_spin("Capa:", -100, 1000, prop_layer_spin);

	add_lbl("Visible:");
	prop_visible_chk = memnew(CheckBox("Mostrar en juego"));
	prop_visible_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	g->add_child(prop_visible_chk);

	add_lbl("Estilo ventana:");
	prop_style_opt = memnew(OptionButton);
	prop_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_style_opt->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	prop_style_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	g->add_child(prop_style_opt);

	// Texto UTF-8
	prop_text_row = memnew(VBoxContainer);
	prop_text_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_text_row);
	prop_text_row->add_child(memnew(Label("Texto / plantilla ({value}):")));
	prop_text_edit = memnew(LineEdit);
	prop_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_text_edit->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	prop_text_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	prop_text_row->add_child(prop_text_edit);

	// Imagen (importar)
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

	// Enlace de datos (desde catálogo)
	prop_binding_row = memnew(VBoxContainer);
	prop_binding_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_binding_row);
	prop_binding_row->add_child(memnew(Label("Enlace de datos:")));
	prop_binding_opt = memnew(OptionButton);
	prop_binding_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_binding_opt->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
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

	// Acción de botón + orden de foco (desde catálogo de pantalla)
	prop_action_row = memnew(VBoxContainer);
	prop_action_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_action_row);
	prop_action_row->add_child(memnew(Label(String::utf8("Acción del botón:"))));
	prop_action_opt = memnew(OptionButton);
	prop_action_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_action_opt->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
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

	// Diálogos auxiliares
	import_image_dialog = memnew(FileDialog);
	import_image_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	import_image_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	import_image_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; Imagen");
	import_image_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_image_file_selected));
	add_child(import_image_dialog);

	reset_confirm_dialog = memnew(ConfirmationDialog);
	reset_confirm_dialog->set_title(String::utf8("Restablecer diseño"));
	reset_confirm_dialog->set_text(String::utf8("¿Restablecer esta pantalla a su diseño por defecto?"));
	reset_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_reset_confirmed));
	add_child(reset_confirm_dialog);
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
	bool want_compact = (p_avail_w > 0 && p_avail_w < 1460);
	compact_two_row_layout = want_compact;

	if (want_compact) {
		wide_hbox->hide();
		compact_vbox->show();
		s_reparent_ctrl(palette_card, compact_top_hbox);
		s_reparent_ctrl(canvas_card, compact_top_hbox);
		s_reparent_ctrl(layers_card, compact_bottom_hbox);
		s_reparent_ctrl(props_card, compact_bottom_hbox);
		palette_card->set_custom_minimum_size(Size2(168 * EDSCALE, 0));
		layers_card->set_custom_minimum_size(Size2(340 * EDSCALE, 0));
		props_card->set_custom_minimum_size(Size2(520 * EDSCALE, 0));
	} else {
		compact_vbox->hide();
		wide_hbox->show();
		s_reparent_ctrl(palette_card, wide_left_col);
		s_reparent_ctrl(layers_card, wide_left_col);
		s_reparent_ctrl(canvas_card, wide_center_col);
		s_reparent_ctrl(props_card, wide_right_col);
		palette_card->set_custom_minimum_size(Size2(0, 0));
		layers_card->set_custom_minimum_size(Size2(0, 0));
		props_card->set_custom_minimum_size(Size2(0, 0));
	}
}

void ClockGearsUiDesignEditor::adapt_to_width(int p_window_width) {
	_rebalance_responsive_layout(p_window_width);
}

void ClockGearsUiDesignEditor::refresh_from_data_manager() {
	_refresh_style_binding_action_options();
	_refresh_canvas_stage();
	_refresh_layers_list();
	_refresh_properties_panel();
	_refresh_undo_redo_buttons();
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

	Size2 stage_sz(float(BASE_W * canvas_zoom), float(BASE_H * canvas_zoom));
	default_title_preview->hide();
	default_gmenu_svc->hide();
	default_gmenu_preview->hide();
	default_credits_preview->hide();

	if (!p_has_custom && !is_hud) {
		if (current_screen_id == "title") {
			default_title_preview->set_size(stage_sz);
			default_title_preview->apply_title_config_preview(dm->get_title_menu_config(), dm->get_system_config());
			default_title_preview->show();
		} else if (current_screen_id.begins_with("game_menu_")) {
			String sub = current_screen_id.trim_prefix("game_menu_");
			default_gmenu_svc->set_size(stage_sz);
			default_gmenu_svc->set_stretch_shrink(MAX(1, canvas_zoom));
			default_gmenu_svc->show();
			default_gmenu_preview->open_menu();
			default_gmenu_preview->open_subscreen(sub);
			default_gmenu_preview->show();
		} else if (current_screen_id == "credits") {
			default_credits_preview->set_size(stage_sz);
			default_credits_preview->apply_credits_config_preview(dm->get_credits_config());
			default_credits_preview->show();
		}
	}
}

void ClockGearsUiDesignEditor::_refresh_canvas_stage() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !design_layer || !canvas_stage) {
		return;
	}
	Size2 stage_sz(float(BASE_W * canvas_zoom), float(BASE_H * canvas_zoom));
	canvas_stage->set_custom_minimum_size(stage_sz);
	canvas_stage->set_size(stage_sz);
	if (canvas_scroll) {
		if (canvas_zoom <= 1) {
			canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		} else {
			canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
			canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
		}
	}
	if (zoom_info_lbl) {
		zoom_info_lbl->set_text(itos(BASE_W * canvas_zoom) + "x" + itos(BASE_H * canvas_zoom));
	}

	Dictionary design = dm->get_ui_screen_design(current_screen_id);
	bool has_custom = dm->has_custom_ui_design(current_screen_id);
	_refresh_background_and_default_preview(has_custom);

	if (screen_status_lbl) {
		if (current_screen_id == "hud") {
			Array elems = design.get("elements", Array());
			screen_status_lbl->set_text(String::utf8("HUD activo (") + itos(elems.size()) + ")");
		} else if (has_custom) {
			screen_status_lbl->set_text(String::utf8("Diseño personalizado activo"));
		} else {
			screen_status_lbl->set_text(String::utf8("Por defecto (arrastra elementos para personalizar)"));
		}
	}

	design_layer->set_viewport_size_override(stage_sz);
	design_layer->set_size(stage_sz);
	if (current_screen_id == "hud" || has_custom) {
		design_layer->apply_screen_design(current_screen_id, design);
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

void ClockGearsUiDesignEditor::_refresh_layers_list() {
	if (!layers_list_vbox) {
		return;
	}
	while (layers_list_vbox->get_child_count() > 0) {
		Node *ch = layers_list_vbox->get_child(0);
		layers_list_vbox->remove_child(ch);
		memdelete(ch);
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

	if (!selected_elem_id.is_empty() && dm->get_ui_element(current_screen_id, selected_elem_id).is_empty()) {
		selected_elem_id = "";
	}

	for (int i = 0; i < sorted.size(); i++) {
		Dictionary e = sorted[i];
		String eid = String(e.get("id", ""));
		String ename = String(e.get("name", eid));
		int lo = int(e.get("layer_order", 0));
		bool vis = bool(e.get("visible", true));
		bool is_sel = (eid == selected_elem_id);

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_theme_constant_override("separation", int(4 * EDSCALE));
		layers_list_vbox->add_child(row);

		Button *eye_btn = memnew(Button);
		eye_btn->set_toggle_mode(true);
		eye_btn->set_pressed(vis);
		eye_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 28 * EDSCALE));
		eye_btn->set_tooltip_text(vis ? String::utf8("Ocultar capa") : String::utf8("Mostrar capa"));
		eye_btn->set_text(vis ? "Ver" : "---");
		s_make_rounded_action_btn(eye_btn);
		eye_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_eye_toggled).bind(eid));
		row->add_child(eye_btn);

		Button *sel_btn = memnew(Button(ename + " [C" + itos(lo) + "]"));
		sel_btn->set_toggle_mode(true);
		sel_btn->set_pressed(is_sel);
		sel_btn->set_clip_text(true);
		sel_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sel_btn->set_custom_minimum_size(Size2(0, 28 * EDSCALE));
		sel_btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		s_make_rounded_action_btn(sel_btn);
		sel_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_row_pressed).bind(eid));
		row->add_child(sel_btn);
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

	if (prop_style_opt) {
		prop_style_opt->clear();
		prop_style_opt->add_item("(Ninguno)", 0);
		prop_style_opt->set_item_metadata(0, String(""));
		Array styles = dm->get_window_styles();
		for (int i = 0; i < styles.size(); i++) {
			if (styles[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary st = styles[i];
			int idx = prop_style_opt->get_item_count();
			prop_style_opt->add_item(String(st.get("name", st.get("id", ""))), idx);
			prop_style_opt->set_item_metadata(idx, String(st.get("id", "")));
		}
	}

	if (prop_binding_opt) {
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

	if (prop_action_opt) {
		prop_action_opt->clear();
		Array actions = ClockGearsDataManager::get_ui_button_actions(current_screen_id);
		for (int i = 0; i < actions.size(); i++) {
			Dictionary ad = actions[i];
			prop_action_opt->add_item(String(ad.get("label", ad.get("id", ""))), i);
			prop_action_opt->set_item_metadata(i, String(ad.get("id", "")));
		}
	}

	if (prop_var_opt) {
		prop_var_opt->clear();
		prop_var_opt->add_item("(Elegir variable)", 0);
		prop_var_opt->set_item_metadata(0, String(""));
		Dictionary vs = dm->get_variables_names();
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
	}

	if (prop_item_opt) {
		prop_item_opt->clear();
		prop_item_opt->add_item("(Elegir objeto)", 0);
		prop_item_opt->set_item_metadata(0, String(""));
		Array items = dm->get_category_array(CAT_ITEMS);
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
	}

	updating_ui = prev_upd;
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

Dictionary ClockGearsUiDesignEditor::_get_selected_element_copy() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return Dictionary();
	}
	return dm->get_ui_element(current_screen_id, selected_elem_id);
}

void ClockGearsUiDesignEditor::_refresh_properties_panel() {
	Dictionary el = _get_selected_element_copy();
	if (el.is_empty()) {
		if (props_sel_lbl) {
			props_sel_lbl->set_text(String::utf8("Selecciona un elemento en el lienzo o en Capas."));
		}
		if (props_fields_box) {
			props_fields_box->hide();
		}
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
	s_select_option_meta(prop_style_opt, String(el.get("window_style_id", "")));

	bool has_text = (etype == "text" || etype == "button" || etype == "variable" || etype == "bar");
	prop_text_row->set_visible(has_text);
	if (has_text) {
		prop_text_edit->set_text(etype == "bar" ? String(el.get("label_text", "")) : String(el.get("text", "")));
	}

	bool has_img = (etype == "image" || etype == "button" || etype == "currency" || etype == "item");
	prop_image_row->set_visible(has_img);
	if (has_img) {
		String ikey = (etype == "currency" || etype == "item") ? "icon_path" : "image_path";
		prop_image_edit->set_text(String(el.get(ikey, "")));
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
	}

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
	String norm_sid = p_screen_id.strip_edges().to_lower();
	Array screens = ClockGearsDataManager::get_ui_design_screens();
	bool matched = false;
	for (int i = 0; i < screens.size(); i++) {
		Dictionary sd = screens[i];
		if (String(sd.get("id", "")) == norm_sid) {
			matched = true;
			if (screen_opt) {
				bool prev = updating_ui;
				updating_ui = true;
				screen_opt->select(i);
				updating_ui = prev;
			}
			break;
		}
	}
	if (!matched && !screens.is_empty()) {
		norm_sid = String(Dictionary(screens[0]).get("id", "title"));
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
	el["name"] = prop_name_edit->get_text();
	el["offset_x"] = int(Math::round(prop_ox_spin->get_value()));
	el["offset_y"] = int(Math::round(prop_oy_spin->get_value()));
	el["width"] = int(Math::round(prop_w_spin->get_value()));
	el["height"] = int(Math::round(prop_h_spin->get_value()));
	el["layer_order"] = int(Math::round(prop_layer_spin->get_value()));
	el["visible"] = prop_visible_chk->is_pressed();
	if (prop_style_opt && prop_style_opt->get_selected() >= 0) {
		el["window_style_id"] = String(prop_style_opt->get_item_metadata(prop_style_opt->get_selected()));
	}
	if (prop_text_row->is_visible()) {
		if (etype == "bar") {
			el["label_text"] = prop_text_edit->get_text();
		} else {
			el["text"] = prop_text_edit->get_text();
		}
	}
	if (prop_image_row->is_visible()) {
		String ikey = (etype == "currency" || etype == "item") ? "icon_path" : "image_path";
		el[ikey] = prop_image_edit->get_text().strip_edges();
	}
	if (prop_var_row->is_visible()) {
		el["variable_name"] = prop_var_edit->get_text().strip_edges();
		if (etype == "text" && String(el.get("data_binding", "none")) == "variable") {
			el["text_mode"] = "variable";
		}
	}
	if (etype == "button" && prop_action_row->is_visible()) {
		el["focus_order"] = int(Math::round(prop_focus_spin->get_value()));
	}
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
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
	if (!final_path.is_empty() && prop_image_edit) {
		prop_image_edit->set_text(final_path);
		_on_prop_field_changed();
	}
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
}

Window *ClockGearsUiDesignEditor::open_test_popup() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return nullptr;
	}
	if (test_window) {
		test_window->queue_free();
		test_window = nullptr;
	}
	test_title = nullptr;
	test_gmenu = nullptr;
	test_credits = nullptr;
	test_hud = nullptr;

	test_window = memnew(Window);
	test_window->set_name("UiDesignTestPopup");
	test_window->set_title(String::utf8("Probar interfaz (Esc para cerrar)"));
	test_window->set_size(Size2i(BASE_W, BASE_H));
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
	} else if (current_screen_id.begins_with("game_menu_")) {
		String sub = current_screen_id.trim_prefix("game_menu_");
		test_gmenu = memnew(ClockGearsGameMenu);
		test_gmenu->set_editor_test_popup_mode(true);
		test_window->add_child(test_gmenu);
		test_gmenu->open_menu();
		test_gmenu->open_subscreen(sub);
	} else if (current_screen_id == "credits") {
		test_credits = memnew(ClockGearsCreditsPlayer);
		test_credits->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(test_credits);
		test_credits->start_credits(dm->get_credits_config());
	} else {
		Control *bg = memnew(UiCanvasHudGridBackdrop);
		bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(bg);

		test_hud = memnew(ClockGearsHudOverlay);
		test_hud->set_preview_mode(false);
		test_hud->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(test_hud);
		test_hud->reload_from_data_manager();
	}

	test_window->popup_centered(Size2i(BASE_W, BASE_H));
	return test_window;
}

Dictionary ClockGearsUiDesignEditor::get_ui_handles() const {
	Dictionary out;
	out["panel"] = this;
	out["screen_opt"] = screen_opt;
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
