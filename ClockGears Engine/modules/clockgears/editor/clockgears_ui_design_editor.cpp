#include "clockgears_ui_design_editor.h"

#include "../clockgears_database.h"
#include "../core/clockgears_data_manager.h"
#include "../icons/clockgears_icons.h"
#include "../runtime/clockgears_credits.h"
#include "../runtime/clockgears_game_menu.h"
#include "../runtime/clockgears_hud.h"
#include "../runtime/clockgears_title_screen.h"
#include "../runtime/clockgears_ui_design_layer.h"
#include "clockgears_image_adjust_dialog.h"
#include "clockgears_theme.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/os/keyboard.h"
#include "core/os/time.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/aspect_ratio_container.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/text_edit.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "servers/display/display_server.h"
#include "scene/resources/texture.h"

namespace {
class UiCanvasHudGridBackdrop : public Control {
	GDCLASS(UiCanvasHudGridBackdrop, Control);

protected:
	void _notification(int p_what) {
		if (p_what == NOTIFICATION_DRAW) {
			Size2 sz = get_size();
			draw_rect(Rect2(Vector2(), sz), Color(0.11f, 0.14f, 0.19f, 1.0f));
		}
	}
};

class UiBattleBackdropPreview : public Control {
	GDCLASS(UiBattleBackdropPreview, Control);

protected:
	void _notification(int p_what) {
		if (p_what == NOTIFICATION_DRAW) {
			Size2 sz = get_size();
			const bool is_brass = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
			const bool is_light = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "light");
			const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, 1.0f);
			Color sky = tk.surface_base.darkened(0.15f);
			Color floor_c = tk.surface_panel.lightened(0.05f);
			draw_rect(Rect2(0, 0, sz.x, sz.y * 0.42f), sky);
			draw_rect(Rect2(0, sz.y * 0.42f, sz.x, sz.y * 0.58f), floor_c);
			draw_line(Vector2(0, sz.y * 0.42f), Vector2(sz.x, sz.y * 0.42f), tk.border_subtle, 1.0f);
			// Enemigo de ejemplo atenuado en el área de batalla (sin inventar datos)
			Ref<Texture2D> en_tex;
			String en_label = "Enemigo de ejemplo";
			if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
				Array ens = dm->get_category_array(CAT_ENEMIES);
				if (!ens.is_empty() && ens[0].get_type() == Variant::DICTIONARY) {
					Dictionary ed = ens[0];
					String en_name = String(ed.get("name", ""));
					if (!en_name.is_empty()) en_label = en_name;
					String gp = String(ed.get("graphic_path", ed.get("sprite_path", ""))).strip_edges();
					if (!gp.is_empty() && ResourceLoader::exists(gp)) {
						en_tex = ResourceLoader::load(gp);
					}
				}
			}
			Vector2 center(sz.x * 0.5f, sz.y * 0.36f);
			if (en_tex.is_valid()) {
				Size2 ts = en_tex->get_size();
				float s = MIN(128.0f / MAX(1.0f, ts.x), 128.0f / MAX(1.0f, ts.y));
				Size2 ds = ts * s;
				draw_texture_rect(en_tex, Rect2(center - ds * 0.5f, ds), false);
			} else {
				draw_circle(center + Vector2(0, 42), 34.0f, Color(tk.surface_base.r, tk.surface_base.g, tk.surface_base.b, 0.55f));
				draw_circle(center, 30.0f, tk.border_copper);
				draw_arc(center, 30.0f, 0, Math::TAU, 24, tk.border_subtle, 1.5f);
			}
			Ref<Font> f = get_theme_default_font();
			if (f.is_valid()) {
				int fs = 13;
				Size2 lsz = f->get_string_size(en_label, HORIZONTAL_ALIGNMENT_CENTER, -1, fs);
				draw_string(f, Vector2(center.x - lsz.x * 0.5f, center.y + 52.0f), en_label, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, ClockGearsTheme::color_text_secondary());
			}
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
	p_btn->set_theme_type_variation("Button");
}

static void s_sync_manual_adjust_btn_state(Button *p_btn, const String &p_scale_mode, bool p_has_image) {
	if (!p_btn) {
		return;
	}
	String sm = p_scale_mode.strip_edges().to_lower();
	if (sm == "nine_slice") {
		p_btn->set_disabled(true);
		p_btn->set_tooltip_text(String::utf8("Desactivado en modo 9-slice porque ya ajusta automáticamente los bordes."));
	} else if (!p_has_image) {
		p_btn->set_disabled(true);
		p_btn->set_tooltip_text(String::utf8("Elige primero una imagen para poder ajustarla manualmente."));
	} else {
		p_btn->set_disabled(false);
		p_btn->set_tooltip_text(String::utf8("Abre el editor visual para mover y escalar la imagen dentro del marco."));
	}
}

static bool s_elem_has_any_image(const Dictionary &p_el) {
	if (!String(p_el.get("image_path", "")).strip_edges().is_empty()) {
		return true;
	}
	if (!String(p_el.get("icon_path", "")).strip_edges().is_empty()) {
		return true;
	}
	for (const char *sk : { "image_normal", "image_hover", "image_pressed", "image_disabled" }) {
		if (!String(p_el.get(sk, "")).strip_edges().is_empty()) {
			return true;
		}
	}
	return false;
}

static void s_clamp_narrow_controls(Control *p_ctrl, float p_scale) {
	if (!p_ctrl) {
		return;
	}
	if (OptionButton *opt = Object::cast_to<OptionButton>(p_ctrl)) {
		opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		opt->set_fit_to_longest_item(true);
		opt->set_clip_text(false);
		opt->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	} else if (CheckBox *cb = Object::cast_to<CheckBox>(p_ctrl)) {
		cb->set_clip_text(false);
		cb->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	} else if (Label *lb = Object::cast_to<Label>(p_ctrl)) {
		if (lb->get_autowrap_mode() == TextServer::AUTOWRAP_OFF) {
			lb->set_clip_text(false);
			lb->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		}
	} else if (LineEdit *le = Object::cast_to<LineEdit>(p_ctrl)) {
		le->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		Size2 ms = le->get_custom_minimum_size();
		le->set_custom_minimum_size(Size2(MAX(ms.x, 100.0f * p_scale), MAX(ms.y, 32.0f * p_scale)));
	} else if (SpinBox *sp = Object::cast_to<SpinBox>(p_ctrl)) {
		sp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		Size2 ms = sp->get_custom_minimum_size();
		sp->set_custom_minimum_size(Size2(MAX(ms.x, 120.0f * p_scale), MAX(ms.y, 32.0f * p_scale)));
	} else if (HSlider *sl = Object::cast_to<HSlider>(p_ctrl)) {
		sl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		Size2 ms = sl->get_custom_minimum_size();
		sl->set_custom_minimum_size(Size2(MAX(ms.x, 80.0f * p_scale), MAX(ms.y, 24.0f * p_scale)));
	} else if (Button *bt = Object::cast_to<Button>(p_ctrl)) {
		if (!Object::cast_to<ColorPickerButton>(bt)) {
			bt->set_clip_text(false);
			bt->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		}
		Size2 ms = bt->get_custom_minimum_size();
		if (ms.y < 30.0f * p_scale) {
			bt->set_custom_minimum_size(Size2(ms.x, 30.0f * p_scale));
		}
	}
	for (int i = 0; i < p_ctrl->get_child_count(); i++) {
		if (Control *ch = Object::cast_to<Control>(p_ctrl->get_child(i))) {
			s_clamp_narrow_controls(ch, p_scale);
		}
	}
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
	set_tooltip_text(String::utf8("Pulsa para crear en el centro o arrastra al lienzo: ") + p_label);
	set_clip_text(true);
	set_custom_minimum_size(Size2(0, 30 * EDSCALE));
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_make_rounded_action_btn(this);
	if (!is_connected("pressed", callable_mp(this, &ClockGearsUiPaletteButton::_on_pressed))) {
		connect("pressed", callable_mp(this, &ClockGearsUiPaletteButton::_on_pressed));
	}
}

void ClockGearsUiPaletteButton::_on_pressed() {
	if (owner_editor) {
		(void)owner_editor->add_element_at(element_type, Vector2i(ClockGearsUiDesignEditor::BASE_W / 2, ClockGearsUiDesignEditor::BASE_H / 2));
	}
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
				Dictionary el = owner_editor->get_effective_element(owner_editor->get_selected_element_id());
				if (el.has("shape") && el["shape"].get_type() == Variant::DICTIONARY) {
					float z = MAX(0.5f, owner_editor->get_canvas_stage_scale());
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

bool ClockGearsUiCanvasOverlay::_get_radial_overlay_geometry(Vector2 &r_center, float &r_radius, float &r_start_rad, float p_scale) const {
	if (!owner_editor) {
		return false;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	Dictionary sel = owner_editor->get_effective_element(owner_editor->get_selected_element_id());
	if (!sel.is_empty() && String(sel.get("type", "")) == "radial") {
		Dictionary lay = sel.get("layout", Dictionary());
		Rect2 stage(0, 0, float(ClockGearsUiDesignEditor::BASE_W) * p_scale, float(ClockGearsUiDesignEditor::BASE_H) * p_scale);
		Array elems = owner_editor->get_effective_screen_elements();
		String sel_id = owner_editor->get_selected_element_id();
		Rect2 er = ClockGearsUiDesignLayer::compute_element_global_rect(sel_id, elems, stage, p_scale);
		if (!er.has_area()) {
			er = ClockGearsUiDesignLayer::compute_element_rect(sel, stage, p_scale);
		}
		r_center = er.get_center();
		float r = float(CLAMP(int(lay.get("radial_radius", 128)), 48, 250)) * p_scale;
		float max_r = MIN(er.size.x, er.size.y) * 0.38f;
		r_radius = MAX(16.0f, MIN(r, max_r));
		r_start_rad = Math::deg_to_rad(float(lay.get("radial_start_angle_deg", -90.0)));
		return true;
	}
	Dictionary design = dm->get_ui_screen_design(owner_editor->get_current_screen());
	Dictionary lay = design.get("layout", Dictionary());
	if (String(lay.get("mode", "free")) == "radial") {
		r_center = Vector2(float(lay.get("radial_center_x", 480)) * p_scale, float(lay.get("radial_center_y", 270)) * p_scale);
		r_radius = float(lay.get("radial_radius", 145)) * p_scale;
		r_start_rad = Math::deg_to_rad(float(lay.get("radial_start_angle_deg", -90.0)));
		return true;
	}
	return false;
}

ClockGearsUiCanvasOverlay::HandleType ClockGearsUiCanvasOverlay::_hit_test_radial_handle(const Vector2 &p_pt) const {
	if (!owner_editor) {
		return HANDLE_NONE;
	}
	float z = MAX(0.5f, owner_editor->get_canvas_stage_scale());
	Vector2 c;
	float rad = 0.0f;
	float ang = 0.0f;
	if (!_get_radial_overlay_geometry(c, rad, ang, z)) {
		return HANDLE_NONE;
	}
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
		if (owner_editor) {
			Array elems = owner_editor->get_effective_screen_elements();
			Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_H));
			HashMap<String, Rect2> all_rects = ClockGearsUiDesignLayer::compute_all_element_rects(elems, stage, 1.0f);
			Vector2 cur_pos = p_base_rect.position + r_delta;
			for (int i = 0; i < elems.size(); i++) {
				if (elems[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary e = elems[i];
				String eid = String(e.get("id", ""));
				if (eid == p_ignore_id || !ClockGearsUiDesignLayer::is_element_effectively_visible(eid, elems)) continue;
				Rect2 er = all_rects.has(eid) ? all_rects[eid] : ClockGearsUiDesignLayer::compute_element_rect(e, stage, 1.0f);
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

HashMap<String, Rect2> ClockGearsUiCanvasOverlay::_compute_all_overlay_rects(const Array &p_elems) const {
	HashMap<String, Rect2> rects;
	if (!owner_editor || p_elems.is_empty()) {
		return rects;
	}
	float scale = owner_editor->get_canvas_stage_scale();
	Rect2 stage = (get_size().x >= 16.0f && get_size().y >= 9.0f)
			? Rect2(Vector2(), get_size())
			: Rect2(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W * scale, ClockGearsUiDesignEditor::BASE_H * scale));
	float eff_scale = stage.size.x / float(ClockGearsUiDesignEditor::BASE_W);
	rects = ClockGearsUiDesignLayer::compute_all_element_rects(p_elems, stage, eff_scale);

	ClockGearsUiDesignLayer *dl = owner_editor->get_canvas_design_layer();
	if (dl && dl->is_visible() && dl->is_inside_tree()) {
		for (int i = 0; i < p_elems.size(); i++) {
			if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary e = p_elems[i];
			String eid = String(e.get("id", ""));
			String etype = String(e.get("type", "")).strip_edges().to_lower();
			if (eid.is_empty() || etype == "folder") continue;
			Control *ctl = dl->get_element_control(eid);
			if (!ctl || !ctl->is_inside_tree()) continue;
			Size2 csz = ctl->get_size();
			if (csz.x < 1.0f || csz.y < 1.0f) continue;
			Vector2 pos;
			Control *cur = ctl;
			int guard = 0;
			while (cur && cur != dl && guard < 32) {
				pos += cur->get_position();
				cur = cur->get_parent_control();
				guard++;
			}
			if (cur == dl) {
				rects[eid] = Rect2(pos, csz);
			}
		}
	}
	return rects;
}

String ClockGearsUiCanvasOverlay::_hit_test_element(const Vector2 &p_pt) const {
	if (!owner_editor) {
		return String();
	}
	Array elems = owner_editor->get_effective_screen_elements();
	if (elems.is_empty()) {
		return String();
	}
	HashMap<String, Rect2> rects = _compute_all_overlay_rects(elems);

	int best_score_layer = -999999;
	int best_idx = -1;
	String best_id;
	for (int i = 0; i < elems.size(); i++) {
		if (elems[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary e = elems[i];
		String eid = String(e.get("id", ""));
		if (String(e.get("type", "")) == "folder" || !ClockGearsUiDesignLayer::is_element_effectively_visible(eid, elems)) {
			continue;
		}
		if (!rects.has(eid)) {
			continue;
		}
		Rect2 r = rects[eid];
		if (r.has_point(p_pt)) {
			int lo = int(e.get("layer_order", 0));
			if (lo > best_score_layer || (lo == best_score_layer && i > best_idx)) {
				best_score_layer = lo;
				best_idx = i;
				best_id = eid;
			}
		}
	}
	return best_id;
}

Rect2 ClockGearsUiCanvasOverlay::get_element_widget_rect(const String &p_elem_id) const {
	if (!owner_editor || p_elem_id.is_empty()) {
		return Rect2();
	}
	Array elems = owner_editor->get_effective_screen_elements();
	Dictionary elem = owner_editor->get_effective_element(p_elem_id);
	if (elem.is_empty()) {
		return Rect2();
	}
	String canon_id = String(elem.get("id", p_elem_id));
	HashMap<String, Rect2> rects = _compute_all_overlay_rects(elems);
	String etype = String(elem.get("type", "")).strip_edges().to_lower();
	if (etype == "folder") {
		HashSet<String> desc;
		desc.insert(canon_id.to_lower());
		bool added = true;
		while (added) {
			added = false;
			for (int i = 0; i < elems.size(); i++) {
				if (elems[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary e = elems[i];
				String cid = String(e.get("id", "")).to_lower();
				String pid = String(e.get("parent_id", "")).strip_edges().to_lower();
				if (!cid.is_empty() && !pid.is_empty() && desc.has(pid) && !desc.has(cid)) {
					desc.insert(cid);
					added = true;
				}
			}
		}
		Rect2 uni;
		bool any = false;
		for (int i = 0; i < elems.size(); i++) {
			if (elems[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary e = elems[i];
			String eid = String(e.get("id", ""));
			String cet = String(e.get("type", "")).to_lower();
			if (cet == "folder" || !desc.has(eid.to_lower())) continue;
			if (!ClockGearsUiDesignLayer::is_element_effectively_visible(eid, elems)) continue;
			if (!rects.has(eid)) continue;
			Rect2 cr = rects[eid];
			if (!any) {
				uni = cr;
				any = true;
			} else {
				uni = uni.merge(cr);
			}
		}
		if (any) {
			return uni;
		}
		float scale = owner_editor->get_canvas_stage_scale();
		Rect2 fr = rects.has(canon_id) ? rects[canon_id] : Rect2();
		return Rect2(fr.position, Size2(MAX(fr.size.x, 48.0f * scale), MAX(fr.size.y, 32.0f * scale)));
	}
	return rects.has(canon_id) ? rects[canon_id] : Rect2();
}

Rect2 ClockGearsUiCanvasOverlay::get_selected_widget_rect() const {
	if (!owner_editor) {
		return Rect2();
	}
	return get_element_widget_rect(owner_editor->get_selected_element_id());
}

Rect2 ClockGearsUiCanvasOverlay::get_handle_widget_rect(int p_handle) const {
	Rect2 sr = get_selected_widget_rect();
	if (!sr.has_area() || p_handle < HANDLE_TL || p_handle > HANDLE_BR) {
		return Rect2();
	}
	return _get_handle_rect(sr, static_cast<HandleType>(p_handle));
}

static bool s_is_supported_image_ext(const String &p_path) {
	String ext = p_path.get_extension().strip_edges().to_lower();
	return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" || ext == "bmp" || ext == "svg";
}

bool ClockGearsUiCanvasOverlay::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary d = p_data;
	if (String(d.get("type", "")) == "clockgears_ui_palette" && !String(d.get("element_type", "")).is_empty()) {
		return true;
	}
	if (String(d.get("type", "")) == "files" && d.has("files")) {
		PackedStringArray files = d["files"];
		for (int i = 0; i < files.size(); i++) {
			if (s_is_supported_image_ext(files[i])) {
				return true;
			}
		}
	}
	return false;
}

void ClockGearsUiCanvasOverlay::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (!owner_editor || !can_drop_data(p_point, p_data)) {
		return;
	}
	Dictionary d = p_data;
	float z = MAX(0.25f, owner_editor->get_canvas_stage_scale());
	Vector2i logical_pos(
			CLAMP(int(Math::round(p_point.x / z)), 0, ClockGearsUiDesignEditor::BASE_W),
			CLAMP(int(Math::round(p_point.y / z)), 0, ClockGearsUiDesignEditor::BASE_H));
	if (String(d.get("type", "")) == "files" && d.has("files")) {
		PackedStringArray files = d["files"];
		for (int i = 0; i < files.size(); i++) {
			if (s_is_supported_image_ext(files[i])) {
				(void)owner_editor->drop_image_file_on_canvas(files[i], logical_pos);
				return;
			}
		}
		return;
	}
	String etype = String(d.get("element_type", ""));
	(void)owner_editor->add_element_at(etype, logical_pos);
}

void ClockGearsUiCanvasOverlay::gui_input(const Ref<InputEvent> &p_event) {
	if (!owner_editor) {
		return;
	}
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// Regla: la rueda del ratón desplaza el ScrollContainer si hay scroll, nunca hace zoom en el lienzo
		if (mb->get_button_index() == MouseButton::WHEEL_UP ||
				mb->get_button_index() == MouseButton::WHEEL_DOWN ||
				mb->get_button_index() == MouseButton::WHEEL_LEFT ||
				mb->get_button_index() == MouseButton::WHEEL_RIGHT) {
			if (mb->is_pressed()) {
				float step = 36.0f * MAX(1.0f, mb->get_factor());
				Vector2 d(0, 0);
				if (mb->get_button_index() == MouseButton::WHEEL_UP) d.y = -step;
				else if (mb->get_button_index() == MouseButton::WHEEL_DOWN) d.y = step;
				else if (mb->get_button_index() == MouseButton::WHEEL_LEFT) d.x = -step;
				else if (mb->get_button_index() == MouseButton::WHEEL_RIGHT) d.x = step;
				if (mb->is_shift_pressed() && d.y != 0.0f) {
					d.x = d.y;
					d.y = 0.0f;
				}
				owner_editor->forward_canvas_wheel(d, d.x != 0.0f);
			}
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
					owner_editor->open_floating_element_window(hit_id);
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
					owner_editor->log_trace_action("canvas_click_miss", vformat("screen=%s pt=(%.1f,%.1f) scale=%.3f ov_sz=(%.1f,%.1f)", owner_editor->get_current_screen(), pt.x, pt.y, owner_editor->get_canvas_stage_scale(), get_size().x, get_size().y));
					if (!mb->is_shift_pressed() && !mb->is_ctrl_pressed() && !mb->is_command_or_control_pressed()) {
						owner_editor->select_element("");
					}
					dragging = false;
					active_handle = HANDLE_NONE;
					drag_multi_initial.clear();
					marquee_selecting = true;
					marquee_start = pt;
					marquee_end = pt;
					guide_lines_x.clear();
					guide_lines_y.clear();
					queue_redraw();
					accept_event();
					return;
				}
				if (mb->is_shift_pressed() || mb->is_ctrl_pressed() || mb->is_command_or_control_pressed()) {
					owner_editor->toggle_element_in_selection(hit_id);
					hh = HANDLE_NONE;
				} else if (!owner_editor->is_element_selected(hit_id)) {
					if (hit_id != owner_editor->get_selected_element_id()) {
						owner_editor->select_element(hit_id);
						hh = HANDLE_NONE;
					}
				}
				if (dm) {
					Array eff_elems = owner_editor->get_effective_screen_elements();
					drag_initial_elem = owner_editor->get_effective_element(hit_id);
					drag_initial_screen_snapshot = dm->get_ui_screen_design(owner_editor->get_current_screen());
					drag_elem_id = String(drag_initial_elem.get("id", hit_id));
					drag_start_pos = pt;
					active_handle = (hh != HANDLE_NONE) ? hh : HANDLE_MOVE;
					drag_multi_initial.clear();
					PackedStringArray sel_ids = owner_editor->get_selected_element_ids();
					if (active_handle == HANDLE_MOVE && sel_ids.size() > 1) {
						for (int si = 0; si < sel_ids.size(); si++) {
							Dictionary me = owner_editor->get_effective_element(sel_ids[si]);
							if (!me.is_empty() && !ClockGearsUiDesignLayer::is_element_effectively_locked(sel_ids[si], eff_elems)) {
								drag_multi_initial[sel_ids[si]] = me;
							}
						}
					}
					dragging = (!drag_initial_elem.is_empty() && !ClockGearsUiDesignLayer::is_element_effectively_locked(drag_elem_id, eff_elems)) || !drag_multi_initial.is_empty();
					drag_changed = false;
					if (dragging) {
						owner_editor->log_trace_action(active_handle == HANDLE_MOVE ? "drag_start" : "resize_start", owner_editor->get_current_screen() + " elem=" + drag_elem_id);
					}
				}
				accept_event();
				return;
			} else if (marquee_selecting) {
				Rect2 mrect = Rect2(marquee_start, marquee_end - marquee_start).abs();
				if (mrect.size.x >= 4.0f || mrect.size.y >= 4.0f) {
					float scale = owner_editor->get_canvas_stage_scale();
					Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W * scale, ClockGearsUiDesignEditor::BASE_H * scale));
					PackedStringArray picked;
					if (mb->is_shift_pressed() || mb->is_ctrl_pressed() || mb->is_command_or_control_pressed()) {
						picked = owner_editor->get_selected_element_ids();
					}
					Array elems = owner_editor->get_effective_screen_elements();
					HashMap<String, Rect2> rects = _compute_all_overlay_rects(elems);
					for (int i = 0; i < elems.size(); i++) {
						if (elems[i].get_type() != Variant::DICTIONARY) continue;
						Dictionary e = elems[i];
						String eid = String(e.get("id", ""));
						if (String(e.get("type", "")) == "folder" || !ClockGearsUiDesignLayer::is_element_effectively_visible(eid, elems)) continue;
						if (!rects.has(eid)) continue;
						Rect2 er = rects[eid];
						if (mrect.intersects(er)) {
							if (!eid.is_empty() && !picked.has(eid)) {
								picked.push_back(eid);
							}
						}
					}
					if (!picked.is_empty()) {
						owner_editor->select_elements(picked, picked[picked.size() - 1]);
					}
				}
				marquee_selecting = false;
				queue_redraw();
				accept_event();
				return;
			} else if (dragging) {
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				if (dm && drag_changed && !drag_initial_screen_snapshot.is_empty()) {
					owner_editor->log_trace_action(active_handle == HANDLE_MOVE ? "drag_end" : "resize_end", owner_editor->get_current_screen() + " elem=" + drag_elem_id);
					owner_editor->commit_canvas_drag_undo(drag_initial_screen_snapshot);
				}
				dragging = false;
				active_handle = HANDLE_NONE;
				drag_changed = false;
				drag_initial_elem.clear();
				drag_multi_initial.clear();
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
		if (marquee_selecting) {
			marquee_end = pt;
			queue_redraw();
			accept_event();
			return;
		}
		if (dragging && (active_handle == HANDLE_RADIAL_RADIUS || active_handle == HANDLE_RADIAL_ANGLE)) {
			float z = MAX(0.25f, owner_editor->get_canvas_stage_scale());
			Vector2 c;
			float cur_rad = 0.0f;
			float cur_ang = 0.0f;
			if (_get_radial_overlay_geometry(c, cur_rad, cur_ang, z)) {
				Vector2 d = (pt - c) / z;
				int rad = int(Math::round(cur_rad / z));
				float ang = Math::rad_to_deg(cur_ang);
				if (active_handle == HANDLE_RADIAL_RADIUS) {
					rad = CLAMP(int(Math::round(d.length())), 48, 250);
				} else {
					ang = Math::rad_to_deg(Math::atan2(d.y, d.x));
				}
				if (!drag_changed) {
					owner_editor->ensure_screen_customizable_for_edit(false);
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
				float z = MAX(0.25f, owner_editor->get_canvas_stage_scale());
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
					Array snap_elems = drag_initial_screen_snapshot.has("elements") ? Array(drag_initial_screen_snapshot["elements"]) : owner_editor->get_effective_screen_elements();
					Rect2 base_r = ClockGearsUiDesignLayer::compute_element_global_rect(drag_elem_id, snap_elems, stage, 1.0f);
					if (!base_r.has_area()) {
						base_r = ClockGearsUiDesignLayer::compute_element_rect(drag_initial_elem, stage, 1.0f);
					}
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
					Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_H));
					Array snap_elems = drag_initial_screen_snapshot.has("elements") ? Array(drag_initial_screen_snapshot["elements"]) : owner_editor->get_effective_screen_elements();
					Rect2 pr(0, 0, ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_H);
					String raw_pid = String(drag_initial_elem.get("parent_id", "")).strip_edges();
					if (!raw_pid.is_empty()) {
						Rect2 cand_pr = ClockGearsUiDesignLayer::compute_element_global_rect(raw_pid, snap_elems, stage, 1.0f);
						if (cand_pr.size.x > 0.5f && cand_pr.size.y > 0.5f) {
							pr = cand_pr;
						} else if (cand_pr.position != Vector2() || ClockGearsUiDesignLayer::compute_all_element_rects(snap_elems, stage, 1.0f).has(raw_pid)) {
							pr = Rect2(cand_pr.position, Size2(ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_H));
						}
					}
					int px = int(Math::round(pr.position.x));
					int py = int(Math::round(pr.position.y));
					int pw = int(Math::round(pr.size.x));
					int ph = int(Math::round(pr.size.y));
					int x0 = px + int(Math::round(af.x * float(pw - w0))) + ox0;
					int y0 = py + int(Math::round(af.y * float(ph - h0))) + oy0;
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
					int nox = nx - px - int(Math::round(af.x * float(pw - nw)));
					int noy = ny - py - int(Math::round(af.y * float(ph - nh)));

					next_elem["width"] = nw;
					next_elem["height"] = nh;
					next_elem["offset_x"] = CLAMP(nox, -ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_W);
					next_elem["offset_y"] = CLAMP(noy, -ClockGearsUiDesignEditor::BASE_H, ClockGearsUiDesignEditor::BASE_H);
					if (String(next_elem.get("type", "")).strip_edges().to_lower() == "radial") {
						Dictionary rlay = (next_elem.has("layout") && next_elem["layout"].get_type() == Variant::DICTIONARY)
								? Dictionary(next_elem["layout"]).duplicate(true)
								: Dictionary();
						rlay["radial_radius"] = CLAMP(int(Math::round(float(MIN(nw, nh)) * 0.36f)), 32, 240);
						next_elem["layout"] = rlay;
					}
				}

				if (int(next_elem["offset_x"]) != ox0 || int(next_elem["offset_y"]) != oy0 ||
						int(next_elem["width"]) != w0 || int(next_elem["height"]) != h0 ||
						int(next_elem.get("corner_radius", 8)) != rad0) {
					if (!drag_changed) {
						owner_editor->ensure_screen_customizable_for_edit(false);
					}
					drag_changed = true;
					if (active_handle == HANDLE_MOVE && drag_multi_initial.size() > 1) {
						Array pairs;
						for (const KeyValue<String, Dictionary> &kv : drag_multi_initial) {
							Dictionary ne = kv.value.duplicate(true);
							ne["offset_x"] = CLAMP(int(ne.get("offset_x", 0)) + dx, -ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_W);
							ne["offset_y"] = CLAMP(int(ne.get("offset_y", 0)) + dy, -ClockGearsUiDesignEditor::BASE_H, ClockGearsUiDesignEditor::BASE_H);
							Dictionary pair;
							pair["id"] = kv.key;
							pair["elem"] = ne;
							pairs.push_back(pair);
						}
						owner_editor->update_multi_dragged_elements_fast(pairs);
					} else {
						owner_editor->update_dragged_element_fast(drag_elem_id, next_elem);
					}
				}
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
		float scale = owner_editor->get_canvas_stage_scale();
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

		// Asas interactivas radiales si hay elemento radial seleccionado o pantalla radial (4D / P7)
		{
			Vector2 c;
			float rad = 0.0f;
			float ang = 0.0f;
			if (_get_radial_overlay_geometry(c, rad, ang, scale)) {
				draw_arc(c, rad, 0.0f, Math::TAU, 48, Color(0.96f, 0.78f, 0.34f, 0.45f), 1.5f);
				Vector2 rad_pt = c + Vector2(rad, 0.0f);
				Vector2 ang_pt = c + Vector2(Math::cos(ang), Math::sin(ang)) * rad;
				draw_line(c, ang_pt, Color(0.55f, 0.88f, 1.0f, 0.5f), 1.0f);
				draw_circle(rad_pt, 6.0f, (hover_handle == HANDLE_RADIAL_RADIUS) ? Color(1.0f, 0.94f, 0.65f) : Color(0.92f, 0.65f, 0.20f));
				draw_circle(ang_pt, 6.0f, (hover_handle == HANDLE_RADIAL_ANGLE) ? Color(0.85f, 1.0f, 1.0f) : Color(0.35f, 0.80f, 0.95f));
			}
		}

		Array elems = owner_editor->get_effective_screen_elements();
		HashMap<String, Rect2> rects = _compute_all_overlay_rects(elems);
		String sel_id = owner_editor->get_selected_element_id();
		Ref<Font> badge_font = get_theme_font("font", "Button");
		for (int i = 0; i < elems.size(); i++) {
			if (elems[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary e = elems[i];
			String eid = String(e.get("id", ""));
			if (!ClockGearsUiDesignLayer::is_element_effectively_visible(eid, elems)) {
				continue;
			}
			String etype = String(e.get("type", "")).strip_edges().to_lower();
			bool is_locked = bool(e.get("locked", false));
			bool is_sel = owner_editor->is_element_selected(eid);
			if (etype == "folder" && !is_sel && eid != hover_elem_id) {
				continue;
			}
			Rect2 r = (etype == "folder") ? get_element_widget_rect(eid) : (rects.has(eid) ? rects[eid] : Rect2());
			if (!r.has_area()) {
				continue;
			}
			if (eid != sel_id) {
				Color c = is_sel ? ClockGearsTheme::color_accent_amber() : ((eid == hover_elem_id) ? Color(0.45f, 0.78f, 1.0f, 0.85f) : Color(0.55f, 0.62f, 0.74f, 0.25f));
				draw_rect(r, c, false, (is_sel || eid == hover_elem_id) ? 1.8f : 1.0f);
			}
			if (is_locked && r.size.x >= 24.0f && r.size.y >= 18.0f) {
				Rect2 lbg(r.position.x + 3.0f, r.position.y + 3.0f, 44.0f, 16.0f);
				draw_rect(lbg, Color(0.68f, 0.24f, 0.16f, 0.90f), true);
				draw_rect(lbg, Color(0.98f, 0.78f, 0.45f, 0.95f), false, 1.0f);
				if (badge_font.is_valid()) {
					draw_string(badge_font, Vector2(lbg.position.x + 4.0f, lbg.position.y + 12.0f), "Bloq", HORIZONTAL_ALIGNMENT_LEFT, 38.0f, 11, Color(1.0f, 0.95f, 0.85f));
				}
			}
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

		// Caja de selección múltiple por arrastre
		if (marquee_selecting) {
			Rect2 mrect = Rect2(marquee_start, marquee_end - marquee_start).abs();
			draw_rect(mrect, Color(0.95f, 0.72f, 0.30f, 0.18f), true);
			draw_rect(mrect, Color(0.95f, 0.72f, 0.30f, 0.92f), false, 1.5f);
		}

		// Guías inteligentes únicamente mientras se arrastra (4D / Punto 4)
		if (dragging && show_smart_guides) {
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

		// Vista previa fantasma en vivo al crear un botón desde el panel Diseños
		if (owner_editor->has_ghost_preview_element()) {
			Dictionary ghost = owner_editor->get_ghost_preview_element();
			if (!ghost.is_empty()) {
				Array g_elems = elems.duplicate();
				g_elems.push_back(ghost);
				String gid = String(ghost.get("id", "__creating_ghost__"));
				Rect2 gr = ClockGearsUiDesignLayer::compute_element_global_rect(gid, g_elems, stage, scale);
				if (!gr.has_area()) {
					gr = ClockGearsUiDesignLayer::compute_element_rect(ghost, stage, scale);
				}
				if (gr.has_area()) {
					Dictionary sh = ghost.get("shape", Dictionary());
					String stype = String(sh.get("shape_type", "rounded")).strip_edges().to_lower();
					Color fill_c = Color(0.18f, 0.22f, 0.30f, 0.58f);
					String fc_hex = String(sh.get("fill_color", ""));
					if (Color::html_is_valid(fc_hex)) {
						fill_c = Color::html(fc_hex);
						fill_c.a = CLAMP(fill_c.a * 0.65f, 0.25f, 0.78f);
					}
					Color hi = ClockGearsTheme::color_accent_amber();
					Color bd_c(hi.r, hi.g, hi.b, 0.92f);
					if (stype == "circle") {
						float rad = MIN(gr.size.x, gr.size.y) * 0.5f;
						Vector2 c = gr.get_center();
						draw_circle(c, rad, fill_c);
						draw_arc(c, rad, 0.0f, Math::TAU, 48, bd_c, 2.0f);
					} else {
						Ref<StyleBoxFlat> sb = memnew(StyleBoxFlat);
						sb->set_bg_color(fill_c);
						sb->set_border_color(bd_c);
						sb->set_border_width_all(2);
						int r_px = (stype == "rect") ? 0 : ((stype == "pill") ? int(MIN(gr.size.x, gr.size.y) * 0.5f) : CLAMP(int(float(sh.get("corner_radius", 8)) * scale), 0, int(MIN(gr.size.x, gr.size.y) * 0.5f)));
						sb->set_corner_radius_all(r_px);
						draw_style_box(sb, gr);
					}
					String gtxt = String(ghost.get("text", ghost.get("name", String::utf8("Nuevo botón"))));
					if (badge_font.is_valid() && !gtxt.is_empty()) {
						int fsz = MAX(10, int(Math::round(float(ghost.get("font_size", 16)) * scale)));
						Color tc(0.96f, 0.96f, 0.98f, 0.88f);
						String tc_hex = String(ghost.get("font_color", ""));
						if (Color::html_is_valid(tc_hex)) {
							tc = Color::html(tc_hex);
							tc.a = 0.88f;
						}
						float ty = gr.position.y + gr.size.y * 0.5f + float(fsz) * 0.35f;
						draw_string(badge_font, Vector2(gr.position.x + 6.0f, ty), gtxt, HORIZONTAL_ALIGNMENT_CENTER, MAX(12.0f, gr.size.x - 12.0f), fsz, tc);
					}
					if (badge_font.is_valid()) {
						Rect2 tag_rc(gr.position.x, MAX(0.0f, gr.position.y - 18.0f), 84.0f, 16.0f);
						draw_rect(tag_rc, Color(0.10f, 0.12f, 0.16f, 0.88f), true);
						draw_rect(tag_rc, bd_c, false, 1.0f);
						draw_string(badge_font, Vector2(tag_rc.position.x + 5.0f, tag_rc.position.y + 12.0f), "Vista previa", HORIZONTAL_ALIGNMENT_LEFT, 76.0f, 10, bd_c);
					}
				}
			}
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
// ClockGearsLayersTree — Árbol jerárquico de capas con D&D y renombrado lento
// ============================================================================

void ClockGearsLayersTree::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT &&
			!mb->is_ctrl_pressed() && !mb->is_shift_pressed() && !mb->is_double_click() && owner_editor) {
		TreeItem *hit = get_item_at_position(mb->get_position());
		int btn_hit = get_button_id_at_position(mb->get_position());
		if (hit && btn_hit < 0) {
			String eid = String(hit->get_metadata(0));
			uint64_t now = Time::get_singleton() ? Time::get_singleton()->get_ticks_msec() : 0;
			PackedStringArray cur_sel = owner_editor->get_selected_element_ids();
			if (!eid.is_empty() && cur_sel.size() == 1 && cur_sel[0] == eid && last_click_id == eid &&
					now > last_click_msec && (now - last_click_msec) >= 420 && (now - last_click_msec) <= 2400) {
				last_click_id = "";
				last_click_msec = 0;
				hit->set_editable(0, true);
				edit_selected(true);
				accept_event();
				return;
			}
			last_click_id = eid;
			last_click_msec = now;
		} else {
			last_click_id = "";
			last_click_msec = 0;
		}
	}
	Tree::gui_input(p_event);
}

Variant ClockGearsLayersTree::get_drag_data(const Point2 &p_point) {
	if (!owner_editor) {
		return Variant();
	}
	TreeItem *hit = get_item_at_position(p_point);
	if (!hit) {
		return Variant();
	}
	String hit_id = String(hit->get_metadata(0));
	if (hit_id.is_empty()) {
		return Variant();
	}
	PackedStringArray drag_ids;
	if (owner_editor->is_element_selected(hit_id)) {
		drag_ids = owner_editor->get_selected_element_ids();
	}
	if (!drag_ids.has(hit_id)) {
		drag_ids.push_back(hit_id);
	}
	PanelContainer *prev = memnew(PanelContainer);
	prev->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	Label *lbl = memnew(Label(drag_ids.size() > 1 ? vformat("%d capas", drag_ids.size()) : hit->get_text(0)));
	lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	prev->add_child(lbl);
	set_drag_preview(prev);

	set_drop_mode_flags(Tree::DROP_MODE_ON_ITEM | Tree::DROP_MODE_INBETWEEN);
	Dictionary d;
	d["type"] = String("clockgears_layer_tree");
	d["ids"] = drag_ids;
	return d;
}

bool ClockGearsLayersTree::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (!owner_editor || p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary d = p_data;
	if (String(d.get("type", "")) != "clockgears_layer_tree") {
		return false;
	}
	PackedStringArray ids = d.get("ids", PackedStringArray());
	TreeItem *target = get_item_at_position(p_point);
	int sec = get_drop_section_at_position(p_point);
	String target_id = target ? String(target->get_metadata(0)) : String("");
	return owner_editor->can_drop_in_layer_tree(ids, target_id, sec);
}

void ClockGearsLayersTree::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (!owner_editor || !can_drop_data(p_point, p_data)) {
		return;
	}
	Dictionary d = p_data;
	PackedStringArray ids = d.get("ids", PackedStringArray());
	TreeItem *target = get_item_at_position(p_point);
	int sec = get_drop_section_at_position(p_point);
	String target_id = target ? String(target->get_metadata(0)) : String("");
	owner_editor->reorder_layer_tree(ids, target_id, sec);
}

// ============================================================================
// ClockGearsDesignChildArrow / CardRow / CardsContainer (Tarjetas en "Diseños")
// ============================================================================

void ClockGearsDesignChildArrow::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Size2 sz = get_size();
		float cx = sz.x * 0.48f;
		float top_y = MAX(2.0f, sz.y * 0.18f);
		float bot_y = MIN(sz.y - 3.0f, sz.y * 0.78f);
		bool light = (ClockGearsTheme::get_singleton() && ClockGearsTheme::get_singleton()->is_light_mode());
		Color col = light
				? Color(0.55f, 0.36f, 0.12f, 0.92f)
				: Color(0.88f, 0.65f, 0.30f, 0.92f);
		// Trazo L con punta hacia arriba (equivalente vectorial a "↰")
		draw_line(Vector2(sz.x - 1.5f, bot_y), Vector2(cx, bot_y), col, MAX(1.5f, 1.5f * EDSCALE), true);
		draw_line(Vector2(cx, bot_y), Vector2(cx, top_y + 2.5f * EDSCALE), col, MAX(1.5f, 1.5f * EDSCALE), true);
		Vector<Vector2> head;
		float hw = 3.2f * EDSCALE;
		float hh = 4.2f * EDSCALE;
		head.push_back(Vector2(cx, top_y));
		head.push_back(Vector2(cx - hw, top_y + hh));
		head.push_back(Vector2(cx + hw, top_y + hh));
		draw_colored_polygon(head, col);
	}
}

void ClockGearsDesignCardRow::_on_mouse_entered() {
	hovered = true;
	update_visual_state(elem_selected);
}

void ClockGearsDesignCardRow::_on_mouse_exited() {
	hovered = false;
	update_visual_state(elem_selected);
}

void ClockGearsDesignCardRow::_on_collapse_pressed() {
	if (owner_editor && !elem_id.is_empty()) {
		owner_editor->set_element_collapsed(elem_id, !collapsed);
		owner_editor->refresh_geometry_fast(false);
		owner_editor->refresh_from_data_manager();
	}
}

void ClockGearsDesignCardRow::_on_eye_pressed() {
	if (owner_editor && !elem_id.is_empty()) {
		owner_editor->toggle_element_visibility(elem_id);
	}
}

void ClockGearsDesignCardRow::_on_lock_pressed() {
	if (owner_editor && !elem_id.is_empty()) {
		owner_editor->toggle_element_locked(elem_id);
	}
}

void ClockGearsDesignCardRow::_on_rename_submitted(const String &p_text) {
	if (!rename_edit || !rename_edit->is_visible()) {
		return;
	}
	rename_edit->hide();
	if (name_lbl) {
		name_lbl->show();
	}
	if (owner_editor && !elem_id.is_empty()) {
		owner_editor->rename_element_inline(elem_id, p_text);
	}
}

void ClockGearsDesignCardRow::_on_rename_focus_exited() {
	if (rename_edit && rename_edit->is_visible()) {
		_on_rename_submitted(rename_edit->get_text());
	}
}

void ClockGearsDesignCardRow::begin_inline_rename() {
	if (!rename_edit || !name_lbl) {
		return;
	}
	rename_edit->set_text(name_lbl->get_text());
	name_lbl->hide();
	rename_edit->show();
	rename_edit->grab_focus();
	rename_edit->select_all();
}

void ClockGearsDesignCardRow::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		if (has_meta("_cg_in_theme_refresh")) {
			return;
		}
		set_meta("_cg_in_theme_refresh", true);
		sb_normal.unref();
		sb_hover.unref();
		sb_selected.unref();
		sb_drop_child.unref();
		update_visual_state(elem_selected);
		remove_meta("_cg_in_theme_refresh");
		queue_redraw();
		return;
	}
	if (p_what == NOTIFICATION_DRAW && drop_section_preview != -999) {
		Size2 sz = get_size();
		Color line_col = ClockGearsTheme::color_accent_amber();
		if (drop_section_preview == -1) {
			float y = 2.0f * EDSCALE;
			draw_line(Vector2(4.0f, y), Vector2(sz.x - 4.0f, y), line_col, MAX(2.5f, 2.5f * EDSCALE));
			draw_circle(Vector2(6.0f, y), 3.5f * EDSCALE, line_col);
		} else if (drop_section_preview == 1) {
			float y = sz.y - 2.0f * EDSCALE;
			draw_line(Vector2(4.0f, y), Vector2(sz.x - 4.0f, y), line_col, MAX(2.5f, 2.5f * EDSCALE));
			draw_circle(Vector2(6.0f, y), 3.5f * EDSCALE, line_col);
		}
	}
}

void ClockGearsDesignCardRow::update_visual_state(bool p_selected) {
	elem_selected = p_selected;
	bool light = (ClockGearsTheme::get_singleton() && ClockGearsTheme::get_singleton()->is_light_mode());
	bool brass = (ClockGearsTheme::get_singleton() && ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
	const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(brass, light, EDSCALE);
	const int r6 = int(6 * EDSCALE);
	const int led_sz = MAX(4, int(Math::round(4.0f * EDSCALE)));
	if (sb_normal.is_null()) {
		sb_normal.instantiate();
		sb_normal->set_corner_radius_all(r6);
		sb_normal->set_content_margin(SIDE_LEFT, int(12 * EDSCALE));
		sb_normal->set_content_margin(SIDE_RIGHT, int(6 * EDSCALE));
		sb_normal->set_content_margin(SIDE_TOP, int(4 * EDSCALE));
		sb_normal->set_content_margin(SIDE_BOTTOM, int(4 * EDSCALE));
		sb_normal->set_bg_color(tk.surface_panel);
		sb_normal->set_border_width_all(1);
		sb_normal->set_border_color(tk.border_subtle);

		sb_hover = sb_normal->duplicate();
		sb_hover->set_bg_color(tk.surface_elevated);
		sb_hover->set_border_color(tk.border_copper_bright);
		sb_hover->set_shadow_color(tk.halo_color);
		sb_hover->set_shadow_size(tk.halo_radius_px);
		sb_hover->set_shadow_offset(Vector2(0, 0));

		Ref<ClockGearsBrassStyleBox> bsel;
		bsel.instantiate();
		bsel->set_anti_aliased(true);
		bsel->set_corner_radius_all(r6);
		bsel->set_content_margin(SIDE_LEFT, int(14 * EDSCALE));
		bsel->set_content_margin(SIDE_RIGHT, int(6 * EDSCALE));
		bsel->set_content_margin(SIDE_TOP, int(4 * EDSCALE));
		bsel->set_content_margin(SIDE_BOTTOM, int(4 * EDSCALE));
		bsel->set_bg_color(tk.surface_panel.lerp(tk.accent_neon, light ? 0.22f : 0.20f));
		bsel->set_border_width_all(1);
		bsel->set_border_color(tk.accent_neon);
		bsel->set_led_dot(true, led_sz, tk.accent_neon);
		bsel->set_shadow_color(tk.halo_color);
		bsel->set_shadow_size(tk.halo_radius_px);
		bsel->set_shadow_offset(Vector2(0, 0));
		sb_selected = bsel;

		sb_drop_child = sb_normal->duplicate();
		sb_drop_child->set_bg_color(tk.surface_panel.lerp(tk.accent_neon, 0.30f));
		sb_drop_child->set_border_width_all(2);
		sb_drop_child->set_border_color(tk.accent_neon);
	}
	Ref<StyleBoxFlat> sb = sb_normal;
	if (drop_section_preview == 0) {
		sb = sb_drop_child;
	} else if (elem_selected) {
		sb = sb_selected;
	} else if (hovered) {
		sb = sb_hover;
	}
	set_meta("_cg_theme_managed_container", true);
	add_theme_style_override("panel", sb);
	if (name_lbl) {
		Color txt_col = light ? Color(0.14f, 0.11f, 0.08f, 1.0f) : Color(0.93f, 0.94f, 0.96f, 1.0f);
		if (!elem_visible) {
			txt_col.a = 0.55f;
		} else if (elem_selected) {
			txt_col = light ? Color(0.20f, 0.12f, 0.03f, 1.0f) : tk.accent_neon.lightened(0.28f);
		}
		name_lbl->add_theme_color_override("font_color", txt_col);
	}
}

void ClockGearsDesignCardRow::set_drop_section_preview(int p_sec) {
	if (drop_section_preview == p_sec) {
		return;
	}
	drop_section_preview = p_sec;
	update_visual_state(elem_selected);
	queue_redraw();
}

int ClockGearsDesignCardRow::compute_drop_section(const Point2 &p_local_pos) const {
	float h = MAX(1.0f, get_size().y);
	float frac = p_local_pos.y / h;
	if (frac < 0.28f) {
		return -1;
	}
	if (frac > 0.72f) {
		return 1;
	}
	return 0;
}

void ClockGearsDesignCardRow::setup(ClockGearsUiDesignEditor *p_owner, ClockGearsDesignCardsContainer *p_container,
		const Dictionary &p_elem, int p_depth, bool p_has_children,
		const Ref<Texture2D> &p_type_icon, const Ref<Texture2D> &p_eye_icon, const Ref<Texture2D> &p_lock_icon,
		bool p_selected, const String &p_parent_name) {
	owner_editor = p_owner;
	cards_container = p_container;
	elem_id = String(p_elem.get("id", ""));
	parent_id = String(p_elem.get("parent_id", ""));
	elem_type = String(p_elem.get("type", "text")).strip_edges().to_lower();
	depth = MAX(0, p_depth);
	has_children = p_has_children;
	collapsed = bool(p_elem.get("collapsed", false));
	elem_visible = bool(p_elem.get("visible", true));
	elem_locked = bool(p_elem.get("locked", false));
	elem_selected = p_selected;

	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_custom_minimum_size(Size2(0, int(30 * EDSCALE)));
	set_mouse_filter(Control::MOUSE_FILTER_STOP);

	String ename = String(p_elem.get("name", elem_id));
	String tip = ename + " (" + (elem_type == "folder" ? String("carpeta") : elem_type) + String::utf8(" · Capa ") + itos(int(p_elem.get("layer_order", 0))) + ")";
	if (!p_parent_name.is_empty()) {
		tip += String::utf8("\nHijo de: ") + p_parent_name;
	}
	set_tooltip_text(tip);
	String card_lh = ename + String::utf8(" (") + (elem_type == "folder" ? String("carpeta") : elem_type) + String::utf8(")\nHaz clic para seleccionarlo y editar sus propiedades en el panel derecho, o arrástralo para cambiar el orden de capas.");
	ClockGearsTheme::set_control_long_help(this, card_lh);

	if (!is_connected("mouse_entered", callable_mp(this, &ClockGearsDesignCardRow::_on_mouse_entered))) {
		connect("mouse_entered", callable_mp(this, &ClockGearsDesignCardRow::_on_mouse_entered));
		connect("mouse_exited", callable_mp(this, &ClockGearsDesignCardRow::_on_mouse_exited));
	}

	row_hb = memnew(HBoxContainer);
	row_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	row_hb->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	row_hb->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	row_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	add_child(row_hb);

	if (depth > 0) {
		int indent_px = MIN(depth, 4) * int(14 * EDSCALE);
		if (depth > 1) {
			Control *indent_sp = memnew(Control);
			indent_sp->set_custom_minimum_size(Size2(indent_px - int(14 * EDSCALE), 0));
			indent_sp->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
			row_hb->add_child(indent_sp);
		}
		ClockGearsDesignChildArrow *arrow = memnew(ClockGearsDesignChildArrow);
		arrow->set_custom_minimum_size(Size2(int(13 * EDSCALE), int(18 * EDSCALE)));
		arrow->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		arrow->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		arrow->set_tooltip_text(!p_parent_name.is_empty() ? (String::utf8("Hijo de: ") + p_parent_name) : String::utf8("Elemento hijo"));
		row_hb->add_child(arrow);
	}

	if (has_children || elem_type == "folder") {
		collapse_btn = memnew(Button);
		collapse_btn->set_flat(true);
		collapse_btn->set_focus_mode(Control::FOCUS_NONE);
		collapse_btn->set_text(collapsed ? String::utf8("▸") : String::utf8("▾"));
		collapse_btn->set_custom_minimum_size(Size2(int(18 * EDSCALE), int(20 * EDSCALE)));
		collapse_btn->set_tooltip_text(collapsed ? String::utf8("Expandir hijos") : String::utf8("Contraer hijos"));
		collapse_btn->connect("pressed", callable_mp(this, &ClockGearsDesignCardRow::_on_collapse_pressed), CONNECT_DEFERRED);
		row_hb->add_child(collapse_btn);
	}

	type_icon_rect = memnew(TextureRect);
	type_icon_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	type_icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	type_icon_rect->set_custom_minimum_size(Size2(int(15 * EDSCALE), int(15 * EDSCALE)));
	type_icon_rect->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	type_icon_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	if (p_type_icon.is_valid()) {
		type_icon_rect->set_texture(p_type_icon);
	}
	row_hb->add_child(type_icon_rect);

	name_lbl = memnew(Label);
	name_lbl->set_text(ename);
	name_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	name_lbl->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	name_lbl->set_clip_text(true);
	name_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	name_lbl->set_tooltip_text(ename);
	name_lbl->set_custom_minimum_size(Size2(0, int(18 * EDSCALE)));
	name_lbl->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	row_hb->add_child(name_lbl);

	rename_edit = memnew(LineEdit);
	rename_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rename_edit->set_custom_minimum_size(Size2(0, int(22 * EDSCALE)));
	rename_edit->hide();
	rename_edit->connect("text_submitted", callable_mp(this, &ClockGearsDesignCardRow::_on_rename_submitted), CONNECT_DEFERRED);
	rename_edit->connect("focus_exited", callable_mp(this, &ClockGearsDesignCardRow::_on_rename_focus_exited), CONNECT_DEFERRED);
	row_hb->add_child(rename_edit);

	eye_btn = memnew(Button);
	eye_btn->set_flat(true);
	eye_btn->set_focus_mode(Control::FOCUS_NONE);
	eye_btn->set_custom_minimum_size(Size2(int(22 * EDSCALE), int(22 * EDSCALE)));
	eye_btn->set_tooltip_text(elem_visible ? String::utf8("Ocultar elemento") : String::utf8("Mostrar elemento"));
	ClockGearsTheme::set_control_long_help(eye_btn, elem_visible ? String::utf8("Ocultar elemento en el lienzo\nDesactiva temporalmente la visibilidad de este elemento en la vista previa y el editor.") : String::utf8("Mostrar elemento en el lienzo\nVuelve a mostrar este elemento oculto en el lienzo y en la vista previa."));
	if (p_eye_icon.is_valid()) {
		eye_btn->set_button_icon(p_eye_icon);
		eye_btn->add_theme_constant_override("icon_max_width", int(14 * EDSCALE));
	} else {
		eye_btn->set_text(elem_visible ? "V" : "-");
	}
	eye_btn->connect("pressed", callable_mp(this, &ClockGearsDesignCardRow::_on_eye_pressed), CONNECT_DEFERRED);
	row_hb->add_child(eye_btn);

	lock_btn = memnew(Button);
	lock_btn->set_flat(true);
	lock_btn->set_focus_mode(Control::FOCUS_NONE);
	lock_btn->set_custom_minimum_size(Size2(int(22 * EDSCALE), int(22 * EDSCALE)));
	lock_btn->set_tooltip_text(elem_locked ? String::utf8("Desbloquear elemento") : String::utf8("Bloquear elemento"));
	ClockGearsTheme::set_control_long_help(lock_btn, elem_locked ? String::utf8("Desbloquear elemento\nPermite mover o redimensionar de nuevo este elemento directamente desde el lienzo.") : String::utf8("Bloquear elemento\nEvita que este elemento se mueva o seleccione por accidente al arrastrar otros elementos."));
	if (p_lock_icon.is_valid()) {
		lock_btn->set_button_icon(p_lock_icon);
		lock_btn->add_theme_constant_override("icon_max_width", int(14 * EDSCALE));
	} else {
		lock_btn->set_text(elem_locked ? "B" : "");
	}
	lock_btn->connect("pressed", callable_mp(this, &ClockGearsDesignCardRow::_on_lock_pressed), CONNECT_DEFERRED);
	row_hb->add_child(lock_btn);

	update_visual_state(elem_selected);
}

void ClockGearsDesignCardRow::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && owner_editor) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_double_click()) {
				owner_editor->select_element(elem_id);
				owner_editor->open_floating_element_window(elem_id);
				accept_event();
				return;
			}
			if (mb->is_ctrl_pressed() || mb->is_command_or_control_pressed()) {
				owner_editor->toggle_element_in_selection(elem_id);
			} else {
				owner_editor->select_element(elem_id);
			}
			accept_event();
			return;
		} else if (mb->get_button_index() == MouseButton::RIGHT) {
			if (!owner_editor->is_element_selected(elem_id)) {
				owner_editor->select_element(elem_id);
			}
			owner_editor->show_layer_context_menu(get_screen_transform().xform(mb->get_position()));
			accept_event();
			return;
		}
	}
	PanelContainer::gui_input(p_event);
}

Variant ClockGearsDesignCardRow::get_drag_data(const Point2 &p_at_position) {
	if (!owner_editor || elem_id.is_empty()) {
		return Variant();
	}
	PackedStringArray drag_ids;
	if (owner_editor->is_element_selected(elem_id)) {
		drag_ids = owner_editor->get_selected_element_ids();
	}
	if (!drag_ids.has(elem_id)) {
		drag_ids.push_back(elem_id);
	}
	PanelContainer *prev = memnew(PanelContainer);
	prev->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	String lbl_txt = drag_ids.size() > 1 ? vformat("%d elementos", drag_ids.size()) : (name_lbl ? name_lbl->get_text() : elem_id);
	Label *lbl = memnew(Label(lbl_txt));
	lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	prev->add_child(lbl);
	set_drag_preview(prev);

	Dictionary d;
	d["type"] = String("clockgears_layer_tree");
	d["ids"] = drag_ids;
	return d;
}

bool ClockGearsDesignCardRow::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (!owner_editor || p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary d = p_data;
	if (String(d.get("type", "")) != "clockgears_layer_tree") {
		return false;
	}
	PackedStringArray ids = d.get("ids", PackedStringArray());
	int sec = compute_drop_section(p_point);
	bool ok = owner_editor->can_drop_in_layer_tree(ids, elem_id, sec);
	if (cards_container) {
		ClockGearsDesignCardRow *self_mut = const_cast<ClockGearsDesignCardRow *>(this);
		if (ok) {
			cards_container->set_drop_preview(self_mut, sec);
		} else {
			cards_container->clear_drop_preview();
		}
	}
	return ok;
}

void ClockGearsDesignCardRow::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (cards_container) {
		cards_container->clear_drop_preview();
	}
	if (!owner_editor || p_data.get_type() != Variant::DICTIONARY) {
		return;
	}
	Dictionary d = p_data;
	if (String(d.get("type", "")) != "clockgears_layer_tree") {
		return;
	}
	PackedStringArray ids = d.get("ids", PackedStringArray());
	int sec = compute_drop_section(p_point);
	owner_editor->reorder_layer_tree(ids, elem_id, sec);
}

void ClockGearsDesignCardsContainer::set_drop_preview(ClockGearsDesignCardRow *p_row, int p_sec) {
	if (drop_preview_row && drop_preview_row != p_row) {
		drop_preview_row->set_drop_section_preview(-999);
	}
	drop_preview_row = p_row;
	drop_preview_section = p_sec;
	if (drop_preview_row) {
		drop_preview_row->set_drop_section_preview(p_sec);
	}
	queue_redraw();
}

void ClockGearsDesignCardsContainer::clear_drop_preview() {
	if (drop_preview_row) {
		drop_preview_row->set_drop_section_preview(-999);
		drop_preview_row = nullptr;
	}
	drop_preview_section = -999;
	queue_redraw();
}

void ClockGearsDesignCardsContainer::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAG_END || p_what == NOTIFICATION_MOUSE_EXIT) {
		clear_drop_preview();
	}
}

bool ClockGearsDesignCardsContainer::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (!owner_editor || p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary d = p_data;
	if (String(d.get("type", "")) != "clockgears_layer_tree") {
		return false;
	}
	PackedStringArray ids = d.get("ids", PackedStringArray());
	if (const_cast<ClockGearsDesignCardsContainer *>(this)->drop_preview_row) {
		const_cast<ClockGearsDesignCardsContainer *>(this)->clear_drop_preview();
	}
	return owner_editor->can_drop_in_layer_tree(ids, String(""), -100);
}

void ClockGearsDesignCardsContainer::drop_data(const Point2 &p_point, const Variant &p_data) {
	clear_drop_preview();
	if (!owner_editor || p_data.get_type() != Variant::DICTIONARY) {
		return;
	}
	Dictionary d = p_data;
	if (String(d.get("type", "")) != "clockgears_layer_tree") {
		return;
	}
	PackedStringArray ids = d.get("ids", PackedStringArray());
	owner_editor->reorder_layer_tree(ids, String(""), -100);
}

// ============================================================================
// ClockGearsUiDesignEditor
// ============================================================================

ClockGearsUiDesignEditor::ClockGearsUiDesignEditor() {
	set_name("UiDesignEditorPanel");
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", int(6 * EDSCALE));
	set_process(true);
	set_process_input(true);
	set_process_shortcut_input(true);
	_build_ui();
}

ClockGearsUiDesignEditor::~ClockGearsUiDesignEditor() {
	long_help_ctrl.cleanup();
	open_floating_windows.clear();
	create_floating_window = nullptr;
	ghost_preview_active = false;
	ghost_preview_elem.clear();
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
}

void ClockGearsUiDesignEditor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("design_modified"));

	ClassDB::bind_method(D_METHOD("refresh_from_data_manager"), &ClockGearsUiDesignEditor::refresh_from_data_manager);
	ClassDB::bind_method(D_METHOD("adapt_to_width", "window_width"), &ClockGearsUiDesignEditor::adapt_to_width);
	ClassDB::bind_method(D_METHOD("set_system_scope", "root_screen_id"), &ClockGearsUiDesignEditor::set_system_scope);
	ClassDB::bind_method(D_METHOD("get_system_scope"), &ClockGearsUiDesignEditor::get_system_scope);
	ClassDB::bind_method(D_METHOD("add_submenu_to_current", "name"), &ClockGearsUiDesignEditor::add_submenu_to_current, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("quick_add_button_element", "text"), &ClockGearsUiDesignEditor::quick_add_button_element, DEFVAL(""));
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
	ClassDB::bind_method(D_METHOD("set_canvas_zoom_mode", "mode"), &ClockGearsUiDesignEditor::set_canvas_zoom_mode);
	ClassDB::bind_method(D_METHOD("get_canvas_zoom_mode"), &ClockGearsUiDesignEditor::get_canvas_zoom_mode);
	ClassDB::bind_method(D_METHOD("set_preview_resolution", "width", "height", "sync_owner_db"), &ClockGearsUiDesignEditor::set_preview_resolution, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_preview_resolution"), &ClockGearsUiDesignEditor::get_preview_resolution);
	ClassDB::bind_method(D_METHOD("add_element_at", "type", "canvas_pos"), &ClockGearsUiDesignEditor::add_element_at, DEFVAL(Vector2i(-1, -1)));
	ClassDB::bind_method(D_METHOD("drop_image_file_on_canvas", "path", "canvas_pos"), &ClockGearsUiDesignEditor::drop_image_file_on_canvas, DEFVAL(Vector2i(-1, -1)));
	ClassDB::bind_method(D_METHOD("select_element", "elem_id"), &ClockGearsUiDesignEditor::select_element);
	ClassDB::bind_method(D_METHOD("get_selected_element_id"), &ClockGearsUiDesignEditor::get_selected_element_id);
	ClassDB::bind_method(D_METHOD("get_effective_screen_elements"), &ClockGearsUiDesignEditor::get_effective_screen_elements);
	ClassDB::bind_method(D_METHOD("get_effective_element", "elem_id"), &ClockGearsUiDesignEditor::get_effective_element);
	ClassDB::bind_method(D_METHOD("delete_selected_element"), &ClockGearsUiDesignEditor::delete_selected_element);
	ClassDB::bind_method(D_METHOD("delete_selected_element_with_mode", "mode"), &ClockGearsUiDesignEditor::delete_selected_element_with_mode);
	ClassDB::bind_method(D_METHOD("toggle_element_visibility", "elem_id"), &ClockGearsUiDesignEditor::toggle_element_visibility);
	ClassDB::bind_method(D_METHOD("toggle_element_locked", "elem_id"), &ClockGearsUiDesignEditor::toggle_element_locked);
	ClassDB::bind_method(D_METHOD("rename_element_inline", "elem_id", "new_name"), &ClockGearsUiDesignEditor::rename_element_inline);
	ClassDB::bind_method(D_METHOD("begin_selected_tree_rename"), &ClockGearsUiDesignEditor::begin_selected_tree_rename);
	ClassDB::bind_method(D_METHOD("create_folder_element", "name", "parent_id"), &ClockGearsUiDesignEditor::create_folder_element, DEFVAL(""), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("group_selected_into_folder", "name"), &ClockGearsUiDesignEditor::group_selected_into_folder, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("ungroup_selected_folder"), &ClockGearsUiDesignEditor::ungroup_selected_folder);
	ClassDB::bind_method(D_METHOD("reorder_layer_tree", "elem_ids", "target_id", "drop_section"), &ClockGearsUiDesignEditor::reorder_layer_tree);
	ClassDB::bind_method(D_METHOD("can_drop_in_layer_tree", "elem_ids", "target_id", "drop_section"), &ClockGearsUiDesignEditor::can_drop_in_layer_tree);
	ClassDB::bind_method(D_METHOD("set_element_collapsed", "elem_id", "collapsed"), &ClockGearsUiDesignEditor::set_element_collapsed);
	ClassDB::bind_method(D_METHOD("get_layer_tree_info"), &ClockGearsUiDesignEditor::get_layer_tree_info);
	ClassDB::bind_method(D_METHOD("get_design_cards_info"), &ClockGearsUiDesignEditor::get_design_cards_info);
	ClassDB::bind_method(D_METHOD("get_design_card_control", "elem_id"), &ClockGearsUiDesignEditor::get_design_card_control);
	ClassDB::bind_method(D_METHOD("add_child_element_of_type", "parent_id", "type"), &ClockGearsUiDesignEditor::add_child_element_of_type);
	ClassDB::bind_method(D_METHOD("reparent_element_to", "elem_id", "new_parent_id"), &ClockGearsUiDesignEditor::reparent_element_to);
	ClassDB::bind_method(D_METHOD("unparent_element", "elem_id"), &ClockGearsUiDesignEditor::unparent_element);
	ClassDB::bind_method(D_METHOD("drop_card_on_target", "elem_ids", "target_id", "drop_section"), &ClockGearsUiDesignEditor::drop_card_on_target);
	ClassDB::bind_method(D_METHOD("simulate_card_drop_at", "target_id", "local_pos", "drag_ids"), &ClockGearsUiDesignEditor::simulate_card_drop_at);
	ClassDB::bind_method(D_METHOD("move_selected_layer", "delta"), &ClockGearsUiDesignEditor::move_selected_layer);
	ClassDB::bind_method(D_METHOD("apply_undo"), &ClockGearsUiDesignEditor::apply_undo);
	ClassDB::bind_method(D_METHOD("apply_redo"), &ClockGearsUiDesignEditor::apply_redo);
	ClassDB::bind_method(D_METHOD("request_reset_to_default", "confirmed_directly"), &ClockGearsUiDesignEditor::request_reset_to_default, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("open_test_popup"), &ClockGearsUiDesignEditor::open_test_popup);
	ClassDB::bind_method(D_METHOD("close_test_popup"), &ClockGearsUiDesignEditor::close_test_popup);
	ClassDB::bind_method(D_METHOD("open_floating_element_window", "elem_id"), &ClockGearsUiDesignEditor::open_floating_element_window, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_floating_window_for_element", "elem_id"), &ClockGearsUiDesignEditor::get_floating_window_for_element);
	ClassDB::bind_method(D_METHOD("close_all_floating_element_windows"), &ClockGearsUiDesignEditor::close_all_floating_element_windows);
	ClassDB::bind_method(D_METHOD("get_open_floating_window_count"), &ClockGearsUiDesignEditor::get_open_floating_window_count);
	ClassDB::bind_method(D_METHOD("open_create_button_window"), &ClockGearsUiDesignEditor::open_create_button_window);
	ClassDB::bind_method(D_METHOD("get_create_button_window"), &ClockGearsUiDesignEditor::get_create_button_window);
	ClassDB::bind_method(D_METHOD("confirm_create_button_from_draft", "draft"), &ClockGearsUiDesignEditor::confirm_create_button_from_draft);
	ClassDB::bind_method(D_METHOD("cancel_create_button_window"), &ClockGearsUiDesignEditor::cancel_create_button_window);
	ClassDB::bind_method(D_METHOD("has_ghost_preview_element"), &ClockGearsUiDesignEditor::has_ghost_preview_element);
	ClassDB::bind_method(D_METHOD("get_ghost_preview_element"), &ClockGearsUiDesignEditor::get_ghost_preview_element);
	ClassDB::bind_method(D_METHOD("begin_inline_folder_create"), &ClockGearsUiDesignEditor::begin_inline_folder_create);
	ClassDB::bind_method(D_METHOD("confirm_inline_folder_create", "name_override"), &ClockGearsUiDesignEditor::confirm_inline_folder_create, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("cancel_inline_folder_create"), &ClockGearsUiDesignEditor::cancel_inline_folder_create);
	ClassDB::bind_method(D_METHOD("get_canvas_design_layer"), &ClockGearsUiDesignEditor::get_canvas_design_layer);
	ClassDB::bind_method(D_METHOD("get_canvas_stage"), &ClockGearsUiDesignEditor::get_canvas_stage);
	ClassDB::bind_method(D_METHOD("get_canvas_overlay"), &ClockGearsUiDesignEditor::get_canvas_overlay);
	ClassDB::bind_method(D_METHOD("get_overlay_element_rect", "elem_id"), &ClockGearsUiDesignEditor::get_overlay_element_rect);
	ClassDB::bind_method(D_METHOD("get_overlay_selected_rect"), &ClockGearsUiDesignEditor::get_overlay_selected_rect);
	ClassDB::bind_method(D_METHOD("get_overlay_handle_rect", "handle"), &ClockGearsUiDesignEditor::get_overlay_handle_rect);
	ClassDB::bind_method(D_METHOD("get_rendered_element_stage_rect", "elem_id"), &ClockGearsUiDesignEditor::get_rendered_element_stage_rect);
	ClassDB::bind_method(D_METHOD("set_grid_step", "step"), &ClockGearsUiDesignEditor::set_grid_step);
	ClassDB::bind_method(D_METHOD("get_grid_step"), &ClockGearsUiDesignEditor::get_grid_step);
	ClassDB::bind_method(D_METHOD("select_elements", "ids", "primary_id"), &ClockGearsUiDesignEditor::select_elements, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("toggle_element_in_selection", "elem_id"), &ClockGearsUiDesignEditor::toggle_element_in_selection);
	ClassDB::bind_method(D_METHOD("get_selected_element_ids"), &ClockGearsUiDesignEditor::get_selected_element_ids);
	ClassDB::bind_method(D_METHOD("is_element_selected", "elem_id"), &ClockGearsUiDesignEditor::is_element_selected);
	ClassDB::bind_method(D_METHOD("get_canvas_draw_stats"), &ClockGearsUiDesignEditor::get_canvas_draw_stats);
	ClassDB::bind_method(D_METHOD("get_ui_handles"), &ClockGearsUiDesignEditor::get_ui_handles);
	ClassDB::bind_method(D_METHOD("get_test_popup_info"), &ClockGearsUiDesignEditor::get_test_popup_info);
	ClassDB::bind_method(D_METHOD("reset_split_layout", "split_idx"), &ClockGearsUiDesignEditor::_reset_wide_split, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("open_manual_image_adjust_for_selected"), &ClockGearsUiDesignEditor::open_manual_image_adjust_for_selected);
	ClassDB::bind_method(D_METHOD("open_manual_image_adjust_for_screen_bg"), &ClockGearsUiDesignEditor::open_manual_image_adjust_for_screen_bg);
	ClassDB::bind_method(D_METHOD("debug_step_long_help", "delta_sec", "pointer_pos"), &ClockGearsUiDesignEditor::debug_step_long_help, DEFVAL(Vector2(-1, -1)));
	ClassDB::bind_method(D_METHOD("debug_get_long_help_state"), &ClockGearsUiDesignEditor::debug_get_long_help_state);
}

float ClockGearsUiDesignEditor::persisted_left_split_w = -1.0f;
float ClockGearsUiDesignEditor::persisted_right_split_w = -1.0f;
bool ClockGearsUiDesignEditor::persisted_split_loaded = false;

void ClockGearsUiDesignEditor::_load_saved_split_widths() {
	if (persisted_split_loaded) {
		return;
	}
	persisted_split_loaded = true;
#ifdef TOOLS_ENABLED
	if (EditorSettings *es = EditorSettings::get_singleton()) {
		if (es->has_setting("clockgears/ui_editor/left_split_width")) {
			persisted_left_split_w = float(es->get("clockgears/ui_editor/left_split_width"));
		}
		if (es->has_setting("clockgears/ui_editor/right_split_width")) {
			persisted_right_split_w = float(es->get("clockgears/ui_editor/right_split_width"));
		}
	}
#endif
}

void ClockGearsUiDesignEditor::_save_split_widths() {
#ifdef TOOLS_ENABLED
	if (EditorSettings *es = EditorSettings::get_singleton()) {
		es->set("clockgears/ui_editor/left_split_width", persisted_left_split_w);
		es->set("clockgears/ui_editor/right_split_width", persisted_right_split_w);
	}
#endif
}

void ClockGearsUiDesignEditor::_apply_wide_split_widths(float p_avail_w) {
	if (!wide_hbox || !wide_right_split || !left_col_scroll || !wide_right_col || compact_two_row_layout) {
		return;
	}
	_load_saved_split_widths();
	float scale = MAX(0.5f, float(EDSCALE));
	float panel_w = get_size().x > 200.0f ? get_size().x : (p_avail_w > 200.0f ? p_avail_w : 1280.0f * scale);
	float eff_w = panel_w;

	const float left_min_drag = 200.0f * scale;
	const float right_min_drag = 280.0f * scale;
	const float center_min = 180.0f * scale;

	float default_left = CLAMP(eff_w * 0.21f, 260.0f * scale, 320.0f * scale);
	float default_right = CLAMP(eff_w * 0.34f, 440.0f * scale, 560.0f * scale);

	float target_left = (persisted_left_split_w > 0.0f) ? (persisted_left_split_w * scale) : default_left;
	float target_right = (persisted_right_split_w > 0.0f) ? (persisted_right_split_w * scale) : default_right;

	target_left = CLAMP(target_left, left_min_drag, MAX(left_min_drag, panel_w * 0.45f));
	target_right = CLAMP(target_right, right_min_drag, MAX(right_min_drag, panel_w * 0.60f));
	float max_pair = MAX(left_min_drag + right_min_drag, panel_w - center_min - 24.0f * scale);
	if (target_left + target_right > max_pair) {
		float excess = target_left + target_right - max_pair;
		float take_r = MIN(excess, MAX(0.0f, target_right - right_min_drag));
		target_right -= take_r;
		excess -= take_r;
		if (excess > 0.0f) {
			target_left = MAX(left_min_drag, target_left - excess);
		}
	}

	left_col_scroll->set_custom_minimum_size(Size2(left_min_drag, 0));
	wide_right_col->set_custom_minimum_size(Size2(right_min_drag, 0));

	updating_splits = true;
	// SplitContainer: con child0 sin EXPAND, default_dragger_pos = 0 -> split_offset = ancho deseado de izq.
	// Con child0 EXPAND y child1 sin EXPAND, default_dragger_pos = size - sep -> split_offset = -ancho deseado de der.
	wide_hbox->set_split_offset(int(Math::round(target_left)));
	wide_right_split->set_split_offset(-int(Math::round(target_right)));
	updating_splits = false;

	if (palette_grid) {
		palette_grid->set_columns(target_left >= 290.0f * scale ? 2 : 1);
	}
}

void ClockGearsUiDesignEditor::_on_wide_split_dragged(int /*p_offset*/, int p_split_idx) {
	if (updating_splits) {
		return;
	}
	callable_mp(this, &ClockGearsUiDesignEditor::_capture_dragged_split_widths).call_deferred(p_split_idx);
}

void ClockGearsUiDesignEditor::_capture_dragged_split_widths(int p_split_idx) {
	if (compact_two_row_layout || updating_splits) {
		return;
	}
	float scale = MAX(0.5f, float(EDSCALE));
	if (p_split_idx == 0 && left_col_scroll && left_col_scroll->get_size().x > 10.0f) {
		persisted_left_split_w = left_col_scroll->get_size().x / scale;
	} else if (p_split_idx == 1 && wide_right_col && wide_right_col->get_size().x > 10.0f) {
		persisted_right_split_w = wide_right_col->get_size().x / scale;
	}
	_save_split_widths();
	if (palette_grid && left_col_scroll) {
		palette_grid->set_columns(left_col_scroll->get_size().x >= 290.0f * scale ? 2 : 1);
	}
	fit_recalc_dirty = true;
	_update_canvas_fit_scale();
}

void ClockGearsUiDesignEditor::_reset_wide_split(int p_split_idx) {
	if (p_split_idx == 0 || p_split_idx < 0) {
		persisted_left_split_w = -1.0f;
	}
	if (p_split_idx == 1 || p_split_idx < 0) {
		persisted_right_split_w = -1.0f;
	}
	_save_split_widths();
	float avail = last_adapt_width > 200 ? float(last_adapt_width) : (get_size().x > 200.0f ? get_size().x : 1280.0f * float(EDSCALE));
	_apply_wide_split_widths(avail);
	fit_recalc_dirty = true;
	callable_mp(this, &ClockGearsUiDesignEditor::_apply_canvas_zoom_layout).bind(true).call_deferred();
}

void ClockGearsUiDesignEditor::_on_split_dragger_gui_input(const Ref<InputEvent> &p_ev, int p_split_idx) {
	Ref<InputEventMouseButton> mb = p_ev;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT && mb->is_double_click() && mb->is_pressed()) {
		_reset_wide_split(p_split_idx);
		Control *dr = (p_split_idx == 0 && wide_hbox) ? wide_hbox->get_drag_area_control() : ((p_split_idx == 1 && wide_right_split) ? wide_right_split->get_drag_area_control() : nullptr);
		if (dr) {
			dr->accept_event();
		}
	}
}

void ClockGearsUiDesignEditor::_refresh_theme_icons() {
	const float icon_scale = float(ClockGearsIcons::get_editor_icon_scale(this));
	const int icon_px = int(16 * EDSCALE);
	ClockGearsIcons *cg_icons = ClockGearsIcons::get_singleton();
	auto assign_btn_icon = [&](Button *p_btn, const StringName &p_ed_name, const String &p_cg_name, bool p_icon_only = false, const String &p_fallback_txt = String()) {
		if (!p_btn) {
			return;
		}
		Ref<Texture2D> ic;
		if (is_inside_tree() && p_btn->is_inside_tree() && p_ed_name != StringName() && p_btn->has_theme_icon(p_ed_name, SNAME("EditorIcons"))) {
			ic = p_btn->get_theme_icon(p_ed_name, SNAME("EditorIcons"));
		}
		if (ic.is_null() && cg_icons && !p_cg_name.is_empty()) {
			ic = cg_icons->get_icon(p_cg_name, icon_scale);
		}
		if (ic.is_valid()) {
			p_btn->set_button_icon(ic);
			p_btn->add_theme_constant_override("icon_max_width", icon_px);
			if (p_icon_only) {
				p_btn->set_text(String());
			}
		} else if (p_icon_only && !p_fallback_txt.is_empty()) {
			p_btn->set_text(p_fallback_txt);
		}
	};
	assign_btn_icon(new_menu_wizard_btn, SNAME("New"), "ui_wizard");
	assign_btn_icon(add_submenu_btn, SNAME("Window"), "ui_frame");
	assign_btn_icon(templates_btn, SNAME("FileList"), "ui_templates");
	assign_btn_icon(undo_btn, SNAME("UndoRedo"), "ui_undo", true, "Deshacer");
	assign_btn_icon(redo_btn, SNAME("Redo"), "ui_redo", true, "Rehacer");
	assign_btn_icon(reset_btn, SNAME("Reload"), "ui_reset", true, "Restaurar");
	if (top_card) {
		top_card->add_theme_style_override("panel", ClockGearsTheme::make_container_stylebox(EDSCALE, 6, 4, 0));
	}
	assign_btn_icon(menu_restore_btn, SNAME("Reload"), "ui_reset");
	assign_btn_icon(test_btn, SNAME("Play"), "ui_play");
	assign_btn_icon(align_center_h_btn, SNAME("AlignHorizontalCenter"), "ui_align_ch", true, "H");
	assign_btn_icon(align_center_v_btn, SNAME("AlignVerticalCenter"), "ui_align_cv", true, "V");
	assign_btn_icon(dist_v_btn, SNAME("VBoxContainer"), "ui_dist_v", true, "Dist");
	assign_btn_icon(dup_elem_btn, SNAME("Duplicate"), "ui_duplicate", true, "Dup");
	assign_btn_icon(copy_elem_btn, SNAME("ActionCopy"), "copy", true, "Cop");
	assign_btn_icon(paste_elem_btn, SNAME("ActionPaste"), "paste", true, "Peg");
	assign_btn_icon(layer_add_button_btn, SNAME("Button"), "ui_button", false);
	assign_btn_icon(layer_add_folder_btn, SNAME("FolderNew"), "add", false);
	assign_btn_icon(layer_group_btn, SNAME("Folder"), "ui_templates", true, "Agr");
	assign_btn_icon(layer_ungroup_btn, SNAME("MoveLeft"), "remove", true, "Desagr");
	assign_btn_icon(layer_up_btn, SNAME("MoveUp"), "ui_layer_up", true, "Sub");
	assign_btn_icon(layer_down_btn, SNAME("MoveDown"), "ui_layer_down", true, "Baj");
	assign_btn_icon(layer_del_btn, SNAME("Remove"), "remove", true, "Bor");
	assign_btn_icon(menu_dup_btn, SNAME("Duplicate"), "ui_duplicate");
	assign_btn_icon(menu_up_btn, SNAME("MoveUp"), "ui_layer_up");
	assign_btn_icon(menu_down_btn, SNAME("MoveDown"), "ui_layer_down");
	assign_btn_icon(menu_del_btn, SNAME("Remove"), "remove");

	if (layers_list_vbox) {
		for (int i = 0; i < layers_list_vbox->get_child_count(); i++) {
			HBoxContainer *row = Object::cast_to<HBoxContainer>(layers_list_vbox->get_child(i));
			if (row && row->get_child_count() >= 1) {
				if (Button *eb = Object::cast_to<Button>(row->get_child(0))) {
					bool vis = eb->is_pressed();
					assign_btn_icon(eb, vis ? SNAME("GuiVisibilityVisible") : SNAME("GuiVisibilityHidden"), vis ? "ui_eye_on" : "ui_eye_off");
					eb->set_text(eb->get_button_icon().is_valid() ? String() : (vis ? "Ver" : "Ocul"));
				}
			}
		}
	}
	if (is_inside_tree() && design_cards_vbox) {
		_refresh_layers_list();
	}
	for (const KeyValue<String, ClockGearsUiElementFloatingWindow *> &kv : open_floating_windows) {
		if (kv.value) {
			kv.value->refresh_theme();
		}
	}
}

void ClockGearsUiDesignEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		_refresh_theme_icons();
		if (Window *win = get_window()) {
			Callable cb = callable_mp(this, &ClockGearsUiDesignEditor::_on_window_files_dropped);
			if (!win->is_connected("files_dropped", cb)) {
				win->connect("files_dropped", cb);
			}
		}
	} else if (p_what == NOTIFICATION_READY || p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		if (is_visible()) {
			refresh_from_data_manager();
			if (last_canvas_viewport_sz.x < 16.0f) {
				fit_recalc_dirty = true;
			}
			callable_mp(this, &ClockGearsUiDesignEditor::_apply_canvas_zoom_layout).bind(fit_recalc_dirty).call_deferred();
		} else {
			long_help_ctrl.hide_help();
			close_test_popup();
		}
	} else if (p_what == NOTIFICATION_PROCESS) {
		const bool db_owns = (owner_db != nullptr && owner_db->is_visible());
		if (!db_owns && is_visible_in_tree()) {
			Window *host_parent = get_window();
			if (get_tree() && get_tree()->get_root()) {
				host_parent = get_tree()->get_root();
			}
			Vector2 pointer_pos = host_parent ? host_parent->get_mouse_position() : Vector2(-10000, -10000);
			if (host_parent && !host_parent->is_embedding_subwindows() && DisplayServer::get_singleton()) {
				pointer_pos = Vector2(DisplayServer::get_singleton()->mouse_get_position());
			}
			ClockGearsLongHelpController::Target tgt;
			resolve_long_help_target(host_parent, pointer_pos, tgt);
			long_help_ctrl.step(this, get_process_delta_time(), pointer_pos, tgt);
		}
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		long_help_ctrl.cleanup();
		close_test_popup();
		close_all_floating_element_windows();
	} else if (p_what == NOTIFICATION_RESIZED) {
		int w = int(get_size().x);
		if (w <= 200 && get_window() && get_window()->get_size().x > 0) {
			w = int(get_window()->get_size().x);
		}
		fit_recalc_dirty = true;
		if (w > 200) {
			_rebalance_responsive_layout(w);
		}
		callable_mp(this, &ClockGearsUiDesignEditor::_apply_canvas_zoom_layout).bind(true).call_deferred();
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
	if (!capturing_key_for_screen && !capturing_key_for_wizard && !capturing_key_for_btn_shortcut) {
		return;
	}
	if (!is_visible_in_tree()) {
		capturing_key_for_screen = false;
		capturing_key_for_wizard = false;
		capturing_key_for_btn_shortcut = false;
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
	} else if (capturing_key_for_btn_shortcut) {
		capturing_key_for_btn_shortcut = false;
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		Dictionary el = _get_selected_element_copy();
		if (dm && !el.is_empty()) {
			el["shortcut"] = ClockGearsDataManager::normalize_menu_trigger(tr);
			Dictionary snap = dm->get_ui_screen_design(current_screen_id);
			bool prev_custom = dm->has_custom_ui_design(current_screen_id);
			ensure_screen_customizable_for_edit(false);
			dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
			if (!prev_custom) {
				dm->push_ui_design_undo_state(current_screen_id, snap);
			}
			_notify_data_mutated(true);
		}
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
	} else if (ctrl && kc == Key::G) {
		if (!group_selected_into_folder().is_empty()) {
			if (get_viewport()) get_viewport()->set_input_as_handled();
		}
	} else if (kc == Key::F2 && !selected_elem_id.is_empty()) {
		if (begin_selected_tree_rename()) {
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
	} else if ((kc == Key::ENTER || kc == Key::KP_ENTER) && !ctrl && !k->is_alt_pressed() && !selected_elem_id.is_empty()) {
		Control *fo = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
		if (!fo || (!Object::cast_to<LineEdit>(fo) && !Object::cast_to<TextEdit>(fo))) {
			open_floating_element_window(selected_elem_id);
			if (get_viewport()) get_viewport()->set_input_as_handled();
		}
	} else if ((kc == Key::KEY_DELETE || kc == Key::BACKSPACE) && !selected_elem_id.is_empty()) {
		delete_selected_element();
		if (get_viewport()) get_viewport()->set_input_as_handled();
	} else if (!selected_elem_id.is_empty() && (kc == Key::LEFT || kc == Key::RIGHT || kc == Key::UP || kc == Key::DOWN)) {
		int step = k->is_shift_pressed() ? 8 : 1;
		int dx = (kc == Key::LEFT ? -step : (kc == Key::RIGHT ? step : 0));
		int dy = (kc == Key::UP ? -step : (kc == Key::DOWN ? step : 0));
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Array eff_elems = get_effective_screen_elements();
			Dictionary el = get_effective_element(selected_elem_id);
			if (!el.is_empty() && !ClockGearsUiDesignLayer::is_element_effectively_locked(selected_elem_id, eff_elems)) {
					Dictionary snap = dm->get_ui_screen_design(current_screen_id);
					bool was_custom = dm->has_custom_ui_design(current_screen_id);
					ensure_screen_customizable_for_edit(false);
					el["offset_x"] = CLAMP(int(el.get("offset_x", 0)) + dx, -BASE_W, BASE_W);
					el["offset_y"] = CLAMP(int(el.get("offset_y", 0)) + dy, -BASE_H, BASE_H);
					dm->update_ui_element(current_screen_id, selected_elem_id, el, was_custom);
					if (!was_custom) {
						dm->push_ui_design_undo_state(current_screen_id, snap);
					}
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
	hdr->set_clip_text(false);
	hdr->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	hdr->set_tooltip_text(p_title);
	const String sec_help = get_section_long_help(p_title);
	if (!sec_help.is_empty()) {
		ClockGearsTheme::set_control_long_help(hdr, sec_help);
	}
	hdr->set_custom_minimum_size(Size2(0, 28 * EDSCALE));
	// Oro luminoso (>= 9.5:1 contraste tanto en tema Oscuro como en Latón)
	const Color hdr_gold(0.98f, 0.87f, 0.56f);
	hdr->add_theme_color_override("font_color", hdr_gold);
	hdr->add_theme_color_override("font_pressed_color", hdr_gold);
	hdr->add_theme_color_override("font_hover_color", Color(1.0f, 0.94f, 0.72f));
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

static float _cg_zoom_factor_for_mode(int p_mode) {
	switch (p_mode) {
		case 3: return 0.5f;
		case 1: return 1.0f;
		case 2: return 2.0f;
		case 4: return 3.0f;
		default: return 1.0f;
	}
}

void ClockGearsUiDesignEditor::_apply_canvas_zoom_layout(bool p_recalc_fit) {
	if (!canvas_stage || !canvas_fit_holder) {
		return;
	}
	float zf = _cg_zoom_factor_for_mode(canvas_zoom_mode);
	Size2 stage_sz(
			MAX(160.0f, float(int(Math::round(float(preview_resolution.x) * zf)))),
			MAX(90.0f, float(int(Math::round(float(preview_resolution.y) * zf)))));
	if (zoom_info_lbl) {
		String pct = (canvas_zoom_mode == 0) ? String("Ajustar") : (canvas_zoom_mode == 3 ? String("50%") : (canvas_zoom_mode == 1 ? String("100%") : (canvas_zoom_mode == 2 ? String("200%") : String("300%"))));
		zoom_info_lbl->set_text(itos(int(stage_sz.x)) + "x" + itos(int(stage_sz.y)) + " (" + pct + ")");
	}
	if (canvas_zoom_mode == 0 && canvas_aspect && canvas_scroll) {
		canvas_aspect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		canvas_aspect->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		Size2 min_16_9(160.0f * EDSCALE, 90.0f * EDSCALE);
		if (canvas_aspect->get_custom_minimum_size().distance_to(min_16_9) > 1.5f) {
			canvas_aspect->set_custom_minimum_size(min_16_9);
		}
		if (canvas_stage->get_custom_minimum_size() != Size2(0, 0)) {
			canvas_stage->set_custom_minimum_size(Size2(0, 0));
		}
		if (canvas_fit_holder->get_custom_minimum_size() != Size2(0, 0)) {
			canvas_fit_holder->set_custom_minimum_size(Size2(0, 0));
		}
		canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	} else if (canvas_aspect && canvas_scroll) {
		Size2 sc_sz = canvas_scroll->get_size();
		bool exceeds = (sc_sz.x > 16.0f && sc_sz.y > 16.0f) ? (stage_sz.x > sc_sz.x + 2.0f || stage_sz.y > sc_sz.y + 2.0f) : (canvas_zoom_mode == 2 || canvas_zoom_mode == 4);
		canvas_scroll->set_horizontal_scroll_mode(exceeds ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
		canvas_scroll->set_vertical_scroll_mode(exceeds ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
		canvas_aspect->set_h_size_flags(exceeds ? Control::SIZE_SHRINK_CENTER : Control::SIZE_EXPAND_FILL);
		canvas_aspect->set_v_size_flags(exceeds ? Control::SIZE_SHRINK_CENTER : Control::SIZE_EXPAND_FILL);
		if (canvas_stage->get_custom_minimum_size().distance_to(stage_sz) > 1.5f) {
			canvas_stage->set_custom_minimum_size(stage_sz);
		}
		if (canvas_aspect->get_custom_minimum_size().distance_to(stage_sz) > 1.5f) {
			canvas_aspect->set_custom_minimum_size(stage_sz);
		}
		if (canvas_fit_holder->get_custom_minimum_size().distance_to(stage_sz) > 1.5f) {
			canvas_fit_holder->set_custom_minimum_size(stage_sz);
		}
	}
	if (p_recalc_fit) {
		_update_canvas_fit_scale();
	}
}

void ClockGearsUiDesignEditor::_on_canvas_scroll_resized() {
	if (canvas_zoom_mode != 0 || !canvas_scroll) {
		return;
	}
	Size2 sc_sz = canvas_scroll->get_size();
	if (sc_sz.x < 16.0f || sc_sz.y < 16.0f) {
		return;
	}
	if (!fit_recalc_dirty && last_canvas_viewport_sz.x >= 16.0f && sc_sz.distance_to(last_canvas_viewport_sz) <= 0.5f) {
		return;
	}
	last_canvas_viewport_sz = sc_sz;
	fit_recalc_dirty = true;
	_update_canvas_fit_scale();
	callable_mp(this, &ClockGearsUiDesignEditor::_on_canvas_fit_holder_resized).call_deferred();
}

void ClockGearsUiDesignEditor::_on_canvas_fit_holder_resized() {
	if (canvas_zoom_mode != 0) {
		return;
	}
	Size2 sc_sz = canvas_scroll ? canvas_scroll->get_size() : Size2();
	if (!fit_recalc_dirty && last_canvas_viewport_sz.x >= 16.0f && sc_sz.distance_to(last_canvas_viewport_sz) <= 0.5f) {
		return;
	}
	_update_canvas_fit_scale();
}

void ClockGearsUiDesignEditor::_restore_canvas_scroll_pos(const Vector2i &p_pos) {
	if (!canvas_scroll || canvas_zoom_mode == 0) {
		return;
	}
	canvas_scroll->set_h_scroll(MAX(0, p_pos.x));
	canvas_scroll->set_v_scroll(MAX(0, p_pos.y));
}

void ClockGearsUiDesignEditor::_update_canvas_fit_scale() {
	if (!canvas_stage || !canvas_fit_holder) {
		return;
	}
	Size2 sc_sz;
	if (canvas_scroll) {
		sc_sz = canvas_scroll->get_size();
		if (sc_sz.x > 16.0f && sc_sz.y > 16.0f) {
			last_canvas_viewport_sz = sc_sz;
		}
	}
	float zf = _cg_zoom_factor_for_mode(canvas_zoom_mode);
	Size2 stage_sz(
			MAX(160.0f, float(int(Math::round(float(preview_resolution.x) * zf)))),
			MAX(90.0f, float(int(Math::round(float(preview_resolution.y) * zf)))));
	Size2 holder_sz = canvas_fit_holder->get_size();
	if (holder_sz.x < 16.0f || holder_sz.y < 16.0f) {
		return;
	}
	if (canvas_zoom_mode == 0) {
		if (!fit_recalc_dirty && last_canvas_viewport_sz.x >= 16.0f && sc_sz.distance_to(last_canvas_viewport_sz) <= 0.5f) {
			return;
		}
		Size2 fit_box = (sc_sz.x >= 16.0f && sc_sz.y >= 16.0f) ? sc_sz : holder_sz;
		float margin = 8.0f * EDSCALE;
		Size2 avail(MAX(32.0f, fit_box.x - margin * 2.0f), MAX(18.0f, fit_box.y - margin * 2.0f));
		float s = MIN(avail.x / MAX(1.0f, stage_sz.x), avail.y / MAX(1.0f, stage_sz.y));
		canvas_stage->set_size(stage_sz);
		canvas_stage->set_scale(Vector2(s, s));
		canvas_stage->set_position((holder_sz - stage_sz * s) * 0.5f);
		if (sc_sz.x >= 16.0f && sc_sz.y >= 16.0f) {
			last_canvas_viewport_sz = sc_sz;
			fit_recalc_dirty = false;
		}
	} else {
		canvas_stage->set_size(stage_sz);
		canvas_stage->set_scale(Vector2(1.0f, 1.0f));
		canvas_stage->set_position((holder_sz - stage_sz) * 0.5f);
	}
}

void ClockGearsUiDesignEditor::forward_canvas_wheel(const Vector2 &p_delta, bool p_horizontal) {
	if (!canvas_scroll) {
		return;
	}
	if (p_horizontal && canvas_scroll->get_horizontal_scroll_mode() != ScrollContainer::SCROLL_MODE_DISABLED) {
		canvas_scroll->set_h_scroll(MAX(0, canvas_scroll->get_h_scroll() + int(Math::round(p_delta.x))));
	} else if (canvas_scroll->get_vertical_scroll_mode() != ScrollContainer::SCROLL_MODE_DISABLED) {
		canvas_scroll->set_v_scroll(MAX(0, canvas_scroll->get_v_scroll() + int(Math::round(p_delta.y))));
	}
}

void ClockGearsUiDesignEditor::_register_prop_filter_row(Control *p_row, const String &p_keywords, bool p_advanced_only, const String &p_elem_types) {
	if (!p_row) {
		return;
	}
	PropRowFilterEntry ent;
	ent.row = p_row;
	ent.search_text = p_keywords.to_lower();
	ent.advanced_only = p_advanced_only;
	ent.elem_types = p_elem_types.strip_edges().to_lower();
	prop_filter_entries.push_back(ent);
}

void ClockGearsUiDesignEditor::_build_ui() {
	const float row_h = 32.0f * EDSCALE;

	// --- Barra superior COMPACTA (máx. 2 filas a 1366x768 = <=96 px; 1 fila desde 1920 px = <=48 px) ---
	top_card = memnew(PanelContainer);
	top_card->set_name("UiDesignerTopBarCard");
	top_card->set_meta("_cg_theme_managed_container", true);
	top_card->add_theme_style_override("panel", ClockGearsTheme::make_container_stylebox(EDSCALE, 6, 4, 0));
	top_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_card->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	top_card->set_clip_contents(false);
	add_child(top_card);

	top_rows_vbox = memnew(VBoxContainer);
	top_rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_rows_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_card->add_child(top_rows_vbox);

	top_row1_flow = memnew(HFlowContainer);
	top_row1_flow->set_name("UiDesignerTopRow1");
	top_row1_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_row1_flow->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	top_row1_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	top_rows_vbox->add_child(top_row1_flow);

	top_row2_flow = memnew(HFlowContainer);
	top_row2_flow->set_name("UiDesignerTopRow2");
	top_row2_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_row2_flow->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	top_row2_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	top_rows_vbox->add_child(top_row2_flow);

	auto make_top_sep = [&](Control *p_parent) -> VSeparator * {
		VSeparator *sep = memnew(VSeparator);
		sep->set_custom_minimum_size(Size2(int(4 * EDSCALE), int(22 * EDSCALE)));
		sep->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		p_parent->add_child(sep);
		return sep;
	};

	// Fila 1 - Grupo 1: Pantalla, Nuevo diseño, + Submenú, Plantillas..., Restaurar (solo icono)
	top_bar = memnew(HBoxContainer);
	top_bar->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_row1_flow->add_child(top_bar);

	top_bar->add_child(memnew(Label(String::utf8("Pantalla:"))));
	screen_opt = memnew(OptionButton);
	screen_opt->set_custom_minimum_size(Size2(118 * EDSCALE, row_h));
	screen_opt->set_clip_text(true);
	screen_opt->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	screen_opt->set_fit_to_longest_item(false);
	screen_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_screen_selected));
	top_bar->add_child(screen_opt);

	new_menu_wizard_btn = memnew(Button(String::utf8("Nuevo diseño")));
	new_menu_wizard_btn->set_custom_minimum_size(Size2(98 * EDSCALE, row_h));
	new_menu_wizard_btn->set_clip_text(false);
	new_menu_wizard_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	new_menu_wizard_btn->set_tooltip_text(String::utf8("Abre el asistente guiado (3 pasos) para crear un diseño nuevo."));
	s_make_rounded_action_btn(new_menu_wizard_btn);
	new_menu_wizard_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_open_menu_wizard));
	top_bar->add_child(new_menu_wizard_btn);

	add_submenu_btn = memnew(Button(String::utf8("+ Submenú")));
	add_submenu_btn->set_custom_minimum_size(Size2(82 * EDSCALE, row_h));
	add_submenu_btn->set_clip_text(false);
	add_submenu_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	add_submenu_btn->set_tooltip_text(String::utf8("Crea un submenú propio anidado bajo la pantalla actual."));
	s_make_rounded_action_btn(add_submenu_btn);
	add_submenu_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_add_submenu_pressed));
	top_bar->add_child(add_submenu_btn);

	templates_btn = memnew(Button(String::utf8("Plantillas…")));
	templates_btn->set_custom_minimum_size(Size2(0, row_h));
	templates_btn->set_clip_text(false);
	templates_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	templates_btn->set_tooltip_text(String::utf8("Explora plantillas de pantallas, componentes o tus diseños guardados."));
	s_make_rounded_action_btn(templates_btn);
	templates_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_templates_btn_pressed));
	top_bar->add_child(templates_btn);

	reset_btn = memnew(Button("Restaurar"));
	reset_btn->set_custom_minimum_size(Size2(row_h, row_h));
	reset_btn->set_clip_text(false);
	reset_btn->set_tooltip_text(String::utf8("Restaurar plantilla original"));
	s_make_rounded_action_btn(reset_btn);
	reset_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_reset_pressed));
	top_bar->add_child(reset_btn);

	// Fila 1 - Grupo 2: Deshacer y Rehacer (solo icono, sin "(0)")
	row1_sep_undo = make_top_sep(top_row1_flow);
	undo_hb = memnew(HBoxContainer);
	undo_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_row1_flow->add_child(undo_hb);

	undo_btn = memnew(Button("Deshacer"));
	undo_btn->set_custom_minimum_size(Size2(row_h, row_h));
	undo_btn->set_tooltip_text(String::utf8("Deshacer último cambio (Ctrl+Z)"));
	s_make_rounded_action_btn(undo_btn);
	undo_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_undo_pressed));
	undo_hb->add_child(undo_btn);

	redo_btn = memnew(Button("Rehacer"));
	redo_btn->set_custom_minimum_size(Size2(row_h, row_h));
	redo_btn->set_tooltip_text(String::utf8("Rehacer cambio deshecho (Ctrl+Y)"));
	s_make_rounded_action_btn(redo_btn);
	redo_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_redo_pressed));
	undo_hb->add_child(redo_btn);

	// Fila 1 - Grupo 3: Probar
	row1_sep_play = make_top_sep(top_row1_flow);
	play_hb = memnew(HBoxContainer);
	play_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	top_row1_flow->add_child(play_hb);

	test_btn = memnew(Button("Probar"));
	test_btn->set_custom_minimum_size(Size2(68 * EDSCALE, row_h));
	test_btn->set_tooltip_text(String::utf8("Prueba el menú actual en tiempo real con datos de ejemplo."));
	s_make_rounded_action_btn(test_btn);
	test_btn->set_clip_text(false);
	test_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	test_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_pressed));
	play_hb->add_child(test_btn);

	test_data_opt = memnew(OptionButton);
	test_data_opt->set_name("UiDesignerTestDataOpt");
	test_data_opt->set_custom_minimum_size(Size2(118 * EDSCALE, row_h));
	test_data_opt->set_fit_to_longest_item(false);
	test_data_opt->set_clip_text(true);
	test_data_opt->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	test_data_opt->set_tooltip_text(String::utf8("Cantidad de objetos de prueba para contenedores con datos en el lienzo y Probar (F5 siempre usa los datos reales de la partida)."));
	test_data_opt->add_item("Datos reales", 0);
	test_data_opt->set_item_metadata(0, -1);
	test_data_opt->add_item(String::utf8("Prueba: 0 objetos"), 1);
	test_data_opt->set_item_metadata(1, 0);
	test_data_opt->add_item(String::utf8("Prueba: 8 objetos"), 2);
	test_data_opt->set_item_metadata(2, 8);
	test_data_opt->add_item(String::utf8("Prueba: 40 objetos"), 3);
	test_data_opt->set_item_metadata(3, 40);
	test_data_opt->select(0);
	test_data_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_data_count_selected));
	play_hb->add_child(test_data_opt);

	// Fila 2 - Grupo 4: Vista (tamaño) y Zoom (Ajustar, 50 %, 100 %, 200 %, 300 %)
	wide_sep_rows = make_top_sep(top_row2_flow);
	wide_sep_rows->hide();

	res_hb = memnew(HBoxContainer);
	res_hb->add_theme_constant_override("separation", int(3 * EDSCALE));
	top_row2_flow->add_child(res_hb);
	res_hb->add_child(memnew(Label("Vista:")));
	preview_res_opt = memnew(OptionButton);
	preview_res_opt->set_custom_minimum_size(Size2(124 * EDSCALE, row_h));
	preview_res_opt->set_fit_to_longest_item(false);
	preview_res_opt->set_clip_text(true);
	preview_res_opt->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	preview_res_opt->set_tooltip_text(String::utf8("Resolución de vista previa del diseño (960x540 base 16:9)."));
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

	context_btn = memnew(Button("Contexto"));
	context_btn->set_name("UiDesignerContextBtn");
	context_btn->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
	context_btn->set_clip_text(false);
	context_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	context_btn->set_tooltip_text(String::utf8("Configura qué sub-pantallas hermanas o escena de ejemplo se ven en segundo plano y con qué opacidad."));
	s_make_rounded_action_btn(context_btn);
	context_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_context_btn_pressed));
	res_hb->add_child(context_btn);

	context_popover = memnew(PopupPanel);
	context_popover->set_name("UiDesignerContextPopover");
	context_popover->set_min_size(Size2i(int(310 * EDSCALE), int(140 * EDSCALE)));
	add_child(context_popover);

	context_pop_panel = memnew(PanelContainer);
	context_pop_panel->set_theme_type_variation("ClockGearsContainer");
	context_pop_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_pop_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	context_popover->add_child(context_pop_panel);

	MarginContainer *ctx_mc = memnew(MarginContainer);
	ctx_mc->add_theme_constant_override("margin_left", int(10 * EDSCALE));
	ctx_mc->add_theme_constant_override("margin_right", int(10 * EDSCALE));
	ctx_mc->add_theme_constant_override("margin_top", int(8 * EDSCALE));
	ctx_mc->add_theme_constant_override("margin_bottom", int(8 * EDSCALE));
	context_pop_panel->add_child(ctx_mc);

	VBoxContainer *ctx_vb = memnew(VBoxContainer);
	ctx_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ctx_vb->add_theme_constant_override("separation", int(8 * EDSCALE));
	ctx_mc->add_child(ctx_vb);

	view_context_chk = memnew(CheckBox("Mostrar contexto"));
	view_context_chk->set_name("UiDesignerViewContextChk");
	view_context_chk->set_pressed(false);
	view_context_chk->set_clip_text(false);
	view_context_chk->set_tooltip_text(String::utf8("Muestra las sub-pantallas hermanas seleccionadas atenuadas detrás del diseño activo."));
	view_context_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_context_chk_toggled));
	ctx_vb->add_child(view_context_chk);

	HBoxContainer *ctx_op_hb = memnew(HBoxContainer);
	ctx_op_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ctx_op_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	ctx_op_hb->add_child(memnew(Label("Opacidad:")));
	context_opacity_slider = memnew(HSlider);
	context_opacity_slider->set_name("UiDesignerContextOpacitySlider");
	context_opacity_slider->set_min(5);
	context_opacity_slider->set_max(100);
	context_opacity_slider->set_step(5);
	context_opacity_slider->set_value(25);
	context_opacity_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_opacity_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_context_opacity_changed));
	ctx_op_hb->add_child(context_opacity_slider);
	context_opacity_val_lbl = memnew(Label("25 %"));
	context_opacity_val_lbl->set_custom_minimum_size(Size2(44 * EDSCALE, 0));
	context_opacity_val_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	ctx_op_hb->add_child(context_opacity_val_lbl);
	ctx_vb->add_child(ctx_op_hb);

	context_siblings_section = memnew(VBoxContainer);
	context_siblings_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_siblings_section->add_theme_constant_override("separation", int(6 * EDSCALE));
	ctx_vb->add_child(context_siblings_section);
	context_siblings_section->add_child(memnew(HSeparator));

	HBoxContainer *ctx_sib_hdr = memnew(HBoxContainer);
	ctx_sib_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ctx_sib_hdr->add_theme_constant_override("separation", int(6 * EDSCALE));
	Label *sib_lbl = memnew(Label("Sub-pantallas hermanas:"));
	sib_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ctx_sib_hdr->add_child(sib_lbl);
	context_all_btn = memnew(Button("Todas"));
	context_all_btn->set_name("UiDesignerContextAllBtn");
	context_all_btn->set_clip_text(false);
	s_make_rounded_action_btn(context_all_btn);
	context_all_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_context_select_all).bind(true));
	ctx_sib_hdr->add_child(context_all_btn);
	context_none_btn = memnew(Button("Ninguna"));
	context_none_btn->set_name("UiDesignerContextNoneBtn");
	context_none_btn->set_clip_text(false);
	s_make_rounded_action_btn(context_none_btn);
	context_none_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_context_select_all).bind(false));
	ctx_sib_hdr->add_child(context_none_btn);
	context_siblings_section->add_child(ctx_sib_hdr);

	ScrollContainer *ctx_sib_sc = memnew(ScrollContainer);
	ctx_sib_sc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ctx_sib_sc->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	ctx_sib_sc->set_custom_minimum_size(Size2(0, 140 * EDSCALE));
	context_siblings_section->add_child(ctx_sib_sc);

	context_siblings_list = memnew(VBoxContainer);
	context_siblings_list->set_name("UiDesignerContextSiblingsList");
	context_siblings_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_siblings_list->add_theme_constant_override("separation", int(4 * EDSCALE));
	ctx_sib_sc->add_child(context_siblings_list);

	context_siblings_empty_lbl = memnew(Label(String::utf8("Sin sub-pantallas hermanas en esta pestaña.")));
	context_siblings_empty_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	context_siblings_empty_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	context_siblings_section->add_child(context_siblings_empty_lbl);

	context_sample_section = memnew(VBoxContainer);
	context_sample_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	context_sample_section->add_theme_constant_override("separation", int(6 * EDSCALE));
	ctx_vb->add_child(context_sample_section);
	context_sample_section->add_child(memnew(HSeparator));

	sample_scene_chk = memnew(CheckBox("Escena de ejemplo"));
	sample_scene_chk->set_name("UiDesignerSampleSceneChk");
	sample_scene_chk->set_pressed(false);
	sample_scene_chk->set_clip_text(false);
	sample_scene_chk->set_tooltip_text(String::utf8("Muestra un fondo de batalla y un enemigo de ejemplo atenuados y no editables."));
	sample_scene_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_sample_scene_chk_toggled));
	context_sample_section->add_child(sample_scene_chk);

	HBoxContainer *smp_op_hb = memnew(HBoxContainer);
	smp_op_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	smp_op_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	smp_op_hb->add_child(memnew(Label("Opacidad escena:")));
	sample_scene_opacity_slider = memnew(HSlider);
	sample_scene_opacity_slider->set_name("UiDesignerSampleOpacitySlider");
	sample_scene_opacity_slider->set_min(5);
	sample_scene_opacity_slider->set_max(100);
	sample_scene_opacity_slider->set_step(5);
	sample_scene_opacity_slider->set_value(25);
	sample_scene_opacity_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sample_scene_opacity_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_sample_scene_opacity_changed));
	smp_op_hb->add_child(sample_scene_opacity_slider);
	sample_scene_opacity_lbl = memnew(Label("25 %"));
	sample_scene_opacity_lbl->set_custom_minimum_size(Size2(44 * EDSCALE, 0));
	sample_scene_opacity_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	smp_op_hb->add_child(sample_scene_opacity_lbl);
	context_sample_section->add_child(smp_op_hb);

	zoom_hb = memnew(HBoxContainer);
	zoom_hb->add_theme_constant_override("separation", int(3 * EDSCALE));
	top_row2_flow->add_child(zoom_hb);
	static const int k_zoom_order[5] = { 0, 3, 1, 2, 4 };
	static const char *k_zoom_labels[5] = { "Ajustar", "100 %", "200 %", "50 %", "300 %" };
	static const char *k_zoom_tips[5] = {
		"Ajustar la pantalla completa 16:9 al lienzo disponible",
		"Escala real 100 % (muestra desplazamiento si no cabe)",
		"Ampliar el lienzo al 200 %",
		"Reducir el lienzo al 50 %",
		"Ampliar el lienzo al 300 %"
	};
	for (int oi = 0; oi < 5; oi++) {
		int z = k_zoom_order[oi];
		Button *zb = memnew(Button(String::utf8(k_zoom_labels[z])));
		zb->set_toggle_mode(true);
		zb->set_pressed(z == canvas_zoom_mode);
		zb->set_custom_minimum_size(Size2((z == 0 ? 56.0f : 44.0f) * EDSCALE, row_h));
		zb->set_clip_text(false);
		zb->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		zb->set_tooltip_text(String::utf8(k_zoom_tips[z]));
		s_make_rounded_action_btn(zb);
		zb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_zoom_pressed).bind(z));
		zoom_hb->add_child(zb);
		zoom_btns[z] = zb;
	}

	// Fila 2 - Grupo 5: Rejilla, tamaño, Imán, Guías
	row2_sep_grid = make_top_sep(top_row2_flow);
	grid_hb = memnew(HBoxContainer);
	grid_hb->add_theme_constant_override("separation", int(3 * EDSCALE));
	top_row2_flow->add_child(grid_hb);

	grid_chk = memnew(CheckBox("Rejilla"));
	grid_chk->set_tooltip_text(String::utf8("Mostrar u ocultar la cuadrícula de alineación sobre el lienzo."));
	grid_chk->set_pressed(false);
	grid_hb->add_child(grid_chk);

	grid_step_opt = memnew(OptionButton);
	grid_step_opt->add_item("8 px", 0);
	grid_step_opt->set_item_metadata(0, 8);
	grid_step_opt->add_item("16 px", 1);
	grid_step_opt->set_item_metadata(1, 16);
	grid_step_opt->add_item("32 px", 2);
	grid_step_opt->set_item_metadata(2, 32);
	grid_step_opt->select(1);
	grid_step_opt->set_custom_minimum_size(Size2(56 * EDSCALE, row_h));
	grid_step_opt->set_tooltip_text(String::utf8("Tamaño del paso de rejilla e imán (8, 16 o 32 px)."));
	grid_step_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_grid_step_selected));
	grid_hb->add_child(grid_step_opt);

	snap_chk = memnew(CheckBox(String::utf8("Imán")));
	snap_chk->set_tooltip_text(String::utf8("Ajustar movimiento y tamaño a la cuadrícula al arrastrar."));
	snap_chk->set_pressed(false);
	grid_hb->add_child(snap_chk);

	guides_chk = memnew(CheckBox(String::utf8("Guías")));
	guides_chk->set_tooltip_text(String::utf8("Mostrar guías inteligentes de bordes y centro mientras arrastras."));
	guides_chk->set_pressed(true);
	grid_hb->add_child(guides_chk);

	// Fila 2 - Grupo 6: Alinear (3 botones)
	row2_sep_align = make_top_sep(top_row2_flow);
	align_hb = memnew(HBoxContainer);
	align_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_row2_flow->add_child(align_hb);

	align_center_h_btn = memnew(Button("Centrar H"));
	align_center_h_btn->set_custom_minimum_size(Size2(row_h, row_h));
	align_center_h_btn->set_tooltip_text(String::utf8("Centrar elemento horizontalmente en el lienzo"));
	s_make_rounded_action_btn(align_center_h_btn);
	align_center_h_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_align_center_pressed).bind(false));
	align_hb->add_child(align_center_h_btn);

	align_center_v_btn = memnew(Button("Centrar V"));
	align_center_v_btn->set_custom_minimum_size(Size2(row_h, row_h));
	align_center_v_btn->set_tooltip_text(String::utf8("Centrar elemento verticalmente en el lienzo"));
	s_make_rounded_action_btn(align_center_v_btn);
	align_center_v_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_align_center_pressed).bind(true));
	align_hb->add_child(align_center_v_btn);

	dist_v_btn = memnew(Button("Distribuir V"));
	dist_v_btn->set_custom_minimum_size(Size2(row_h, row_h));
	dist_v_btn->set_tooltip_text(String::utf8("Distribuir los elementos verticalmente en el lienzo"));
	s_make_rounded_action_btn(dist_v_btn);
	dist_v_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_distribute_vertical_pressed));
	align_hb->add_child(dist_v_btn);

	// Fila 2 - Grupo 7: Duplicar, Copiar y Pegar
	row2_sep_clip = make_top_sep(top_row2_flow);
	clip_hb = memnew(HBoxContainer);
	clip_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_row2_flow->add_child(clip_hb);

	dup_elem_btn = memnew(Button("Duplicar"));
	dup_elem_btn->set_custom_minimum_size(Size2(row_h, row_h));
	dup_elem_btn->set_tooltip_text(String::utf8("Duplicar elemento seleccionado (Ctrl+D)"));
	s_make_rounded_action_btn(dup_elem_btn);
	dup_elem_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_duplicate_elem_pressed));
	clip_hb->add_child(dup_elem_btn);

	copy_elem_btn = memnew(Button("Copiar"));
	copy_elem_btn->set_custom_minimum_size(Size2(row_h, row_h));
	copy_elem_btn->set_tooltip_text(String::utf8("Copiar elemento seleccionado (Ctrl+C)"));
	s_make_rounded_action_btn(copy_elem_btn);
	copy_elem_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_copy_elem_pressed));
	clip_hb->add_child(copy_elem_btn);

	paste_elem_btn = memnew(Button("Pegar"));
	paste_elem_btn->set_custom_minimum_size(Size2(row_h, row_h));
	paste_elem_btn->set_tooltip_text(String::utf8("Pegar elemento del portapapeles (Ctrl+V)"));
	s_make_rounded_action_btn(paste_elem_btn);
	paste_elem_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_paste_elem_pressed));
	clip_hb->add_child(paste_elem_btn);

	// --- Contenedores de distribución en 3 columnas (Lienzo dominante >= 60%) ---
	layout_root = memnew(VBoxContainer);
	layout_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	layout_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	layout_root->add_theme_constant_override("separation", int(6 * EDSCALE));
	add_child(layout_root);

	wide_hbox = memnew(HSplitContainer);
	wide_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_hbox->add_theme_constant_override("separation", int(6 * EDSCALE));
	wide_hbox->connect("dragged", callable_mp(this, &ClockGearsUiDesignEditor::_on_wide_split_dragged).bind(0));
	if (Control *dr0 = wide_hbox->get_drag_area_control()) {
		dr0->set_tooltip_text(String::utf8("Arrastra para ajustar · Doble clic para restablecer"));
		dr0->connect("gui_input", callable_mp(this, &ClockGearsUiDesignEditor::_on_split_dragger_gui_input).bind(0));
	}
	layout_root->add_child(wide_hbox);

	left_col_scroll = memnew(ScrollContainer);
	left_col_scroll->set_custom_minimum_size(Size2(200 * EDSCALE, 0));
	left_col_scroll->set_h_size_flags(Control::SIZE_FILL);
	left_col_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	left_col_scroll->set_stretch_ratio(0.0f);
	left_col_scroll->set_clip_contents(true);
	left_col_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	left_col_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	wide_hbox->add_child(left_col_scroll);

	wide_left_col = memnew(VBoxContainer);
	wide_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_left_col->set_clip_contents(true);
	wide_left_col->add_theme_constant_override("separation", int(8 * EDSCALE));
	left_col_scroll->add_child(wide_left_col);

	wide_right_split = memnew(HSplitContainer);
	wide_right_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_right_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_right_split->add_theme_constant_override("separation", int(8 * EDSCALE));
	wide_right_split->connect("dragged", callable_mp(this, &ClockGearsUiDesignEditor::_on_wide_split_dragged).bind(1));
	if (Control *dr1 = wide_right_split->get_drag_area_control()) {
		dr1->set_tooltip_text(String::utf8("Arrastra para ajustar · Doble clic para restablecer"));
		dr1->connect("gui_input", callable_mp(this, &ClockGearsUiDesignEditor::_on_split_dragger_gui_input).bind(1));
	}
	wide_hbox->add_child(wide_right_split);

	wide_center_col = memnew(VBoxContainer);
	wide_center_col->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
	wide_center_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_center_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_center_col->set_stretch_ratio(1.0f);
	wide_right_split->add_child(wide_center_col);

	wide_right_col = memnew(VBoxContainer);
	wide_right_col->set_custom_minimum_size(Size2(280 * EDSCALE, 0));
	wide_right_col->set_h_size_flags(Control::SIZE_FILL);
	wide_right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_right_col->set_stretch_ratio(0.0f);
	wide_right_col->set_clip_contents(true);
	wide_right_col->add_theme_constant_override("separation", int(8 * EDSCALE));
	wide_right_split->add_child(wide_right_col);

	compact_scroll = memnew(ScrollContainer);
	compact_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	compact_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	compact_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	compact_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	compact_scroll->hide();
	layout_root->add_child(compact_scroll);

	compact_vbox = memnew(VBoxContainer);
	compact_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	compact_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	compact_vbox->add_theme_constant_override("separation", int(8 * EDSCALE));
	compact_scroll->add_child(compact_vbox);

	// --- Tarjeta 0 (Corrección 9B): Lista de elementos como tarjetas en "Diseños" ---
	menus_card = memnew(PanelContainer);
	menus_card->set_theme_type_variation("ClockGearsContainer");
	menus_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	menus_card->set_clip_contents(true);
	wide_left_col->add_child(menus_card);

	VBoxContainer *menus_vb = memnew(VBoxContainer);
	menus_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	menus_vb->add_theme_constant_override("separation", int(5 * EDSCALE));
	menus_card->add_child(menus_vb);

	VBoxContainer *menus_body = memnew(VBoxContainer);
	menus_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	menus_body->add_theme_constant_override("separation", int(5 * EDSCALE));
	Button *menus_hdr = _attach_collapsible_header(menus_vb, String::utf8("Elementos"), menus_body, true);
	if (menus_hdr) {
		menus_hdr->set_name("LeftElementsHeaderBtn");
		menus_hdr->set_tooltip_text(String::utf8("Elementos de la pantalla actual en orden de capas (arriba = al frente)."));
	}
	menus_vb->add_child(menus_body);

	HFlowContainer *lay_btns = memnew(HFlowContainer);
	lay_btns->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lay_btns->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	lay_btns->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	menus_body->add_child(lay_btns);

	layer_add_button_btn = memnew(Button(String::utf8("+ Botón")));
	layer_add_button_btn->set_tooltip_text(String::utf8("Crear un botón nuevo paso a paso con vista previa en el lienzo."));
	s_make_rounded_action_btn(layer_add_button_btn);
	layer_add_button_btn->set_custom_minimum_size(Size2(92 * EDSCALE, 32 * EDSCALE));
	layer_add_button_btn->set_clip_text(false);
	layer_add_button_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	layer_add_button_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_add_button_pressed));
	lay_btns->add_child(layer_add_button_btn);

	layer_add_folder_btn = memnew(Button("+ Carpeta"));
	layer_add_folder_btn->set_tooltip_text(String::utf8("Crear carpeta nueva; si hay elementos seleccionados, los agrupa dentro manteniendo su posición."));
	s_make_rounded_action_btn(layer_add_folder_btn);
	layer_add_folder_btn->set_custom_minimum_size(Size2(98 * EDSCALE, 32 * EDSCALE));
	layer_add_folder_btn->set_clip_text(false);
	layer_add_folder_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	layer_add_folder_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_add_folder_pressed));
	lay_btns->add_child(layer_add_folder_btn);

	layer_group_btn = memnew(Button("Agrupar"));
	layer_group_btn->set_tooltip_text(String::utf8("Agrupar elementos seleccionados en una carpeta (Ctrl+G)."));
	s_make_rounded_action_btn(layer_group_btn);
	layer_group_btn->set_custom_minimum_size(Size2(32 * EDSCALE, 32 * EDSCALE));
	layer_group_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_group_pressed));
	lay_btns->add_child(layer_group_btn);

	layer_ungroup_btn = memnew(Button("Desagrupar"));
	layer_ungroup_btn->set_tooltip_text(String::utf8("Sacar hijos de la carpeta seleccionada y quitarla."));
	s_make_rounded_action_btn(layer_ungroup_btn);
	layer_ungroup_btn->set_custom_minimum_size(Size2(32 * EDSCALE, 32 * EDSCALE));
	layer_ungroup_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_ungroup_pressed));
	lay_btns->add_child(layer_ungroup_btn);

	layer_up_btn = memnew(Button(String::utf8("Subir")));
	layer_up_btn->set_tooltip_text(String::utf8("Traer al frente un nivel."));
	s_make_rounded_action_btn(layer_up_btn);
	layer_up_btn->set_custom_minimum_size(Size2(32 * EDSCALE, 32 * EDSCALE));
	layer_up_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_move_pressed).bind(1));
	lay_btns->add_child(layer_up_btn);

	layer_down_btn = memnew(Button(String::utf8("Bajar")));
	layer_down_btn->set_tooltip_text(String::utf8("Enviar al fondo un nivel."));
	s_make_rounded_action_btn(layer_down_btn);
	layer_down_btn->set_custom_minimum_size(Size2(32 * EDSCALE, 32 * EDSCALE));
	layer_down_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_move_pressed).bind(-1));
	lay_btns->add_child(layer_down_btn);

	layer_del_btn = memnew(Button("Borrar"));
	layer_del_btn->set_tooltip_text(String::utf8("Eliminar elemento seleccionado (Supr)."));
	s_make_rounded_action_btn(layer_del_btn);
	layer_del_btn->set_custom_minimum_size(Size2(32 * EDSCALE, 32 * EDSCALE));
	layer_del_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_delete_pressed));
	lay_btns->add_child(layer_del_btn);

	folder_prompt_box = memnew(HBoxContainer);
	folder_prompt_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	folder_prompt_box->add_theme_constant_override("separation", int(4 * EDSCALE));
	folder_prompt_box->hide();
	menus_body->add_child(folder_prompt_box);

	folder_name_edit = memnew(LineEdit);
	folder_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	folder_name_edit->set_custom_minimum_size(Size2(90 * EDSCALE, 32 * EDSCALE));
	folder_name_edit->set_placeholder("Carpeta");
	folder_name_edit->connect("text_submitted", callable_mp(this, &ClockGearsUiDesignEditor::_on_folder_prompt_confirm));
	folder_name_edit->connect("gui_input", callable_mp(this, &ClockGearsUiDesignEditor::_on_folder_name_gui_input));
	folder_prompt_box->add_child(folder_name_edit);

	folder_confirm_btn = memnew(Button("Crear"));
	s_make_rounded_action_btn(folder_confirm_btn);
	folder_confirm_btn->set_custom_minimum_size(Size2(64 * EDSCALE, 32 * EDSCALE));
	folder_confirm_btn->set_clip_text(false);
	folder_confirm_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	folder_confirm_btn->set_tooltip_text(String::utf8("Confirmar nombre y crear carpeta."));
	folder_confirm_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_folder_prompt_confirm).bind(String()));
	folder_prompt_box->add_child(folder_confirm_btn);

	folder_cancel_btn = memnew(Button("X"));
	s_make_rounded_action_btn(folder_cancel_btn);
	folder_cancel_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
	folder_cancel_btn->set_clip_text(false);
	folder_cancel_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	folder_cancel_btn->set_tooltip_text(String::utf8("Cancelar creación de carpeta (Esc)."));
	folder_cancel_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_folder_prompt_cancel));
	folder_prompt_box->add_child(folder_cancel_btn);

	design_cards_scroll = memnew(ScrollContainer);
	design_cards_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	design_cards_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	design_cards_scroll->set_custom_minimum_size(Size2(0, 120 * EDSCALE));
	design_cards_scroll->set_clip_contents(true);
	design_cards_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	design_cards_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	design_cards_scroll->set_tooltip_text(String::utf8("Lista de elementos (arriba = al frente). Haz clic para seleccionar, doble clic para editar en ventana, clic derecho para acciones, o arrastra para reordenar / anidar."));
	menus_body->add_child(design_cards_scroll);

	design_cards_vbox = memnew(ClockGearsDesignCardsContainer);
	design_cards_vbox->set_owner_editor(this);
	design_cards_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	design_cards_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	design_cards_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	design_cards_scroll->add_child(design_cards_vbox);

	HSeparator *menu_ops_sep = memnew(HSeparator);
	menu_ops_sep->hide();
	menus_body->add_child(menu_ops_sep);

	// Controles heredados de la antigua lista de pantallas: ocultos para evitar duplicados y cortes en el panel Elementos.
	HFlowContainer *menu_ops = memnew(HFlowContainer);
	menu_ops->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_ops->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	menu_ops->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	menu_ops->hide();
	menus_body->add_child(menu_ops);

	menu_dup_btn = memnew(Button("Duplicar"));
	menu_dup_btn->set_tooltip_text(String::utf8("Duplica la pantalla o menú personalizado actual."));
	s_make_rounded_action_btn(menu_dup_btn);
	menu_dup_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_dup_pressed));
	menu_ops->add_child(menu_dup_btn);

	menu_up_btn = memnew(Button("Subir"));
	menu_up_btn->set_tooltip_text(String::utf8("Sube el menú personalizado en la lista."));
	s_make_rounded_action_btn(menu_up_btn);
	menu_up_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_move_pressed).bind(-1));
	menu_ops->add_child(menu_up_btn);

	menu_down_btn = memnew(Button("Bajar"));
	menu_down_btn->set_tooltip_text(String::utf8("Baja el menú personalizado en la lista."));
	s_make_rounded_action_btn(menu_down_btn);
	menu_down_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_move_pressed).bind(1));
	menu_ops->add_child(menu_down_btn);

	menu_del_btn = memnew(Button("Borrar"));
	menu_del_btn->set_tooltip_text(String::utf8("Elimina el menú personalizado seleccionado."));
	s_make_rounded_action_btn(menu_del_btn);
	menu_del_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_del_pressed));
	menu_ops->add_child(menu_del_btn);

	menu_restore_btn = memnew(Button("Restaurar"));
	menu_restore_btn->set_tooltip_text(String::utf8("Restaura la plantilla original en pantallas de sistema."));
	s_make_rounded_action_btn(menu_restore_btn);
	menu_restore_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_restore_pressed));
	menu_ops->add_child(menu_restore_btn);

	screens_tree = memnew(Tree);
	screens_tree->set_hide_root(true);
	screens_tree->set_select_mode(Tree::SELECT_SINGLE);
	screens_tree->hide();
	menus_body->add_child(screens_tree);

	ScrollContainer *menus_scroll = memnew(ScrollContainer);
	menus_scroll->hide();
	menus_body->add_child(menus_scroll);

	menus_list_vbox = memnew(VBoxContainer);
	menus_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_list_vbox->add_theme_constant_override("separation", int(3 * EDSCALE));
	menus_scroll->add_child(menus_list_vbox);

	// Menú contextual de tarjetas de elementos
	layer_context_menu = memnew(PopupMenu);
	add_child_submenu = memnew(PopupMenu);
	static const struct {
		const char *type;
		const char *label_utf8;
	} k_child_types[14] = {
		{ "button", "Bot\xC3\xB3n" },
		{ "text", "Texto" },
		{ "image", "Imagen" },
		{ "frame", "Marco" },
		{ "panel", "Panel" },
		{ "folder", "Carpeta" },
		{ "container", "Contenedor datos" },
		{ "bar", "Barra" },
		{ "hearts", "Corazones" },
		{ "currency", "Monedas" },
		{ "item", "Contador" },
		{ "variable", "Variable" },
		{ "widget", "Lista / Widget" },
		{ "radial", "Rueda radial" }
	};
	for (int i = 0; i < 14; i++) {
		add_child_submenu->add_item(String::utf8(k_child_types[i].label_utf8), i);
		add_child_submenu->set_item_metadata(i, String(k_child_types[i].type));
	}
	add_child_submenu->connect("id_pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_add_child_submenu_id));
	layer_context_menu->add_child(add_child_submenu);

	reparent_submenu = memnew(PopupMenu);
	reparent_submenu->connect("id_pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_reparent_submenu_id));
	layer_context_menu->add_child(reparent_submenu);

	layer_context_menu->add_item("Propiedades flotantes (Enter)", 11);
	layer_context_menu->add_separator();
	layer_context_menu->add_submenu_node_item(String::utf8("Añadir hijo"), add_child_submenu, 20);
	layer_context_menu->add_submenu_node_item("Hacer hijo de...", reparent_submenu, 22);
	layer_context_menu->add_item("Sacar del padre", 21);
	layer_context_menu->add_separator();
	layer_context_menu->add_item("Duplicar (con hijos) (Ctrl+D)", 1);
	layer_context_menu->add_item("Agrupar en carpeta (Ctrl+G)", 2);
	layer_context_menu->add_item("Desagrupar", 3);
	layer_context_menu->add_item("Renombrar (F2)", 4);
	layer_context_menu->add_separator();
	layer_context_menu->add_item("Subir", 5);
	layer_context_menu->add_item("Bajar", 6);
	layer_context_menu->add_item("Guardar como plantilla", 7);
	layer_context_menu->add_separator();
	layer_context_menu->add_item("Eliminar (Supr)", 8);
	layer_context_menu->connect("id_pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_context_menu_id));
	add_child(layer_context_menu);

	// --- Tarjeta 1: Paleta lateral (plegable) ---
	palette_card = memnew(PanelContainer);
	palette_card->set_theme_type_variation("ClockGearsContainer");
	palette_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_left_col->add_child(palette_card);

	VBoxContainer *pal_vb = memnew(VBoxContainer);
	pal_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pal_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	palette_card->add_child(pal_vb);

	VBoxContainer *pal_body = memnew(VBoxContainer);
	pal_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_attach_collapsible_header(pal_vb, "Elementos (arrastrar)", pal_body, false);
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
	static const PaletteDef k_palette[13] = {
		{ "folder", "Carpeta" },
		{ "image", "Imagen" },
		{ "text", "Texto" },
		{ "button", "+ Bot\xC3\xB3n" },
		{ "frame", "Marco" },
		{ "container", "Contenedor datos" },
		{ "bar", "Barra" },
		{ "hearts", "Corazones" },
		{ "currency", "Monedas" },
		{ "item", "Contador" },
		{ "variable", "Variable" },
		{ "widget", "Lista / Widget" },
		{ "radial", "+ Rueda radial" }
	};
	for (int i = 0; i < 13; i++) {
		ClockGearsUiPaletteButton *pb = memnew(ClockGearsUiPaletteButton);
		pb->setup(this, String(k_palette[i].type), String::utf8(k_palette[i].label_utf8));
		palette_grid->add_child(pb);
		palette_buttons.push_back(pb);
	}

	// --- Tarjeta 2: Lienzo 16:9 dominante usando ClockGearsUiDesignLayer ---
	canvas_card = memnew(PanelContainer);
	canvas_card->set_theme_type_variation("ClockGearsPreviewFrame");
	canvas_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_card->set_clip_contents(true);
	wide_center_col->add_child(canvas_card);

	VBoxContainer *can_vb = memnew(VBoxContainer);
	can_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	can_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	can_vb->add_theme_constant_override("separation", int(4 * EDSCALE));
	canvas_card->add_child(can_vb);

	canvas_scroll = memnew(ScrollContainer);
	canvas_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_scroll->set_custom_minimum_size(Size2(160 * EDSCALE, 90 * EDSCALE));
	canvas_scroll->set_clip_contents(true);
	canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	can_vb->add_child(canvas_scroll);

	canvas_aspect = memnew(AspectRatioContainer);
	canvas_aspect->set_ratio(16.0f / 9.0f);
	canvas_aspect->set_stretch_mode(AspectRatioContainer::STRETCH_FIT);
	canvas_aspect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_aspect->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_aspect->set_custom_minimum_size(Size2(160 * EDSCALE, 90 * EDSCALE));
	canvas_aspect->connect("resized", callable_mp(this, &ClockGearsUiDesignEditor::_on_canvas_fit_holder_resized), CONNECT_DEFERRED);
	canvas_scroll->add_child(canvas_aspect);
	canvas_scroll->connect("resized", callable_mp(this, &ClockGearsUiDesignEditor::_on_canvas_scroll_resized), CONNECT_DEFERRED);

	HBoxContainer *canvas_status_hb = memnew(HBoxContainer);
	canvas_status_hb->set_name("UiDesignerCanvasStatusBar");
	canvas_status_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_status_hb->set_custom_minimum_size(Size2(0, 20 * EDSCALE));
	canvas_status_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	can_vb->add_child(canvas_status_hb);

	screen_status_lbl = memnew(Label);
	screen_status_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	screen_status_lbl->set_clip_text(true);
	screen_status_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	screen_status_lbl->add_theme_font_size_override("font_size", int(11 * EDSCALE));
	screen_status_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	canvas_status_hb->add_child(screen_status_lbl);

	zoom_info_lbl = memnew(Label("960x540"));
	zoom_info_lbl->add_theme_font_size_override("font_size", int(11 * EDSCALE));
	zoom_info_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	canvas_status_hb->add_child(zoom_info_lbl);

	canvas_fit_holder = memnew(Control);
	canvas_fit_holder->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_fit_holder->set_clip_contents(true);
	canvas_fit_holder->connect("resized", callable_mp(this, &ClockGearsUiDesignEditor::_on_canvas_fit_holder_resized), CONNECT_DEFERRED);
	canvas_aspect->add_child(canvas_fit_holder);

	canvas_stage = memnew(Control);
	canvas_stage->set_name("UiDesignerCanvasStage");
	canvas_stage->set_custom_minimum_size(Size2(0, 0));
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

	ctx_battle_backdrop_node = memnew(UiBattleBackdropPreview);
	ctx_battle_backdrop_node->set_name("UiDesignerBattleBackdrop");
	ctx_battle_backdrop_node->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	ctx_battle_backdrop_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	ctx_battle_backdrop_node->set_modulate(Color(1, 1, 1, 0.25f));
	ctx_battle_backdrop_node->hide();
	canvas_stage->add_child(ctx_battle_backdrop_node);

	context_layers_root = memnew(Control);
	context_layers_root->set_name("UiDesignerContextLayers");
	context_layers_root->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	context_layers_root->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	context_layers_root->set_modulate(Color(1, 1, 1, 0.25f));
	context_layers_root->hide();
	canvas_stage->add_child(context_layers_root);

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
	canvas_context_menu->add_item("Propiedades flotantes (Enter)", 11);
	canvas_context_menu->add_separator();
	canvas_context_menu->add_item("Duplicar (Ctrl+D)", 1);
	canvas_context_menu->add_item("Copiar (Ctrl+C)", 2);
	canvas_context_menu->add_item("Pegar (Ctrl+V)", 3);
	canvas_context_menu->add_separator();
	canvas_context_menu->add_item("Agrupar en carpeta (Ctrl+G)", 9);
	canvas_context_menu->add_item("Desagrupar", 10);
	canvas_context_menu->add_separator();
	canvas_context_menu->add_item("Subir capa", 4);
	canvas_context_menu->add_item("Bajar capa", 5);
	canvas_context_menu->add_item("Centrar en pantalla", 6);
	canvas_context_menu->add_item("Guardar como mi plantilla de componente", 7);
	canvas_context_menu->add_separator();
	canvas_context_menu->add_item("Borrar (Supr)", 8);
	canvas_context_menu->connect("id_pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_canvas_context_menu_id));
	add_child(canvas_context_menu);

	// (Árbol inferior "Capas" reemplazado por la lista de tarjetas dentro de "Diseños".)

	// --- Tarjeta 4: Panel de propiedades (4C: Simple/Avanzado, Buscador, Secciones) ---
	props_card = memnew(PanelContainer);
	props_card->set_theme_type_variation("ClockGearsContainer");
	props_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_right_col->add_child(props_card);

	VBoxContainer *props_outer = memnew(VBoxContainer);
	props_outer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_outer->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_outer->set_clip_contents(true);
	props_outer->add_theme_constant_override("separation", int(6 * EDSCALE));
	props_card->add_child(props_outer);

	VBoxContainer *props_body = memnew(VBoxContainer);
	props_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_body->set_clip_contents(true);
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
	props_sel_lbl->set_clip_text(true);
	props_sel_lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	props_sel_lbl->set_custom_minimum_size(Size2(0, 0));
	props_sel_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	props_body->add_child(props_sel_lbl);

	ScrollContainer *props_scroll = memnew(ScrollContainer);
	props_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll->set_custom_minimum_size(Size2(150 * EDSCALE, 140 * EDSCALE));
	props_scroll->set_clip_contents(true);
	props_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	props_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_ALWAYS);
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

	auto make_prop_row = [&](VBoxContainer *p_parent, const String &p_lbl, const String &p_tip, const String &p_kw, bool p_adv, Control *p_ctrl, const String &p_elem_types = String()) {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
		hb->add_theme_constant_override("separation", int(8 * EDSCALE));
		Label *l = memnew(Label(p_lbl));
		l->set_custom_minimum_size(Size2(122 * EDSCALE, 32 * EDSCALE));
		l->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		l->set_clip_text(false);
		l->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		l->set_tooltip_text(p_tip);
		hb->add_child(l);
		p_ctrl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		p_ctrl->set_tooltip_text(p_tip);
		Size2 cms = p_ctrl->get_custom_minimum_size();
		if (cms.y < 32.0f * EDSCALE) {
			p_ctrl->set_custom_minimum_size(Size2(cms.x, 32.0f * EDSCALE));
		}
		hb->add_child(p_ctrl);
		p_parent->add_child(hb);
		_register_prop_filter_row(hb, p_lbl + " " + p_kw, p_adv, p_elem_types);
	};

	auto add_prop_section = [&](VBoxContainer *p_parent, const String &p_title, bool p_adv_section, VBoxContainer *&r_body) {
		r_body = memnew(VBoxContainer);
		r_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_body->add_theme_constant_override("separation", int(5 * EDSCALE));
		Button *shdr = _attach_collapsible_header(p_parent, p_title, r_body, true);
		p_parent->add_child(r_body);
		PropSectionGroup sg;
		sg.header = shdr;
		sg.body = r_body;
		sg.advanced_only = p_adv_section;
		prop_sections.push_back(sg);
	};

	prop_name_edit = memnew(LineEdit);
	prop_name_edit->set_custom_minimum_size(Size2(120 * EDSCALE, 32 * EDSCALE));
	prop_name_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(props_fields_box, "Nombre:", String::utf8("Nombre visible del elemento en capas y diseño."), "nombre texto", false, prop_name_edit);

	// Sección 1: Posición
	VBoxContainer *sec_pos_body = nullptr;
	add_prop_section(props_fields_box, String::utf8("Posición"), false, sec_pos_body);

	HBoxContainer *anc_row = memnew(HBoxContainer);
	anc_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anc_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	Label *anc_lbl = memnew(Label("Anclaje:"));
	anc_lbl->set_custom_minimum_size(Size2(122 * EDSCALE, 32 * EDSCALE));
	anc_lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	anc_lbl->set_clip_text(false);
	anc_row->add_child(anc_lbl);
	GridContainer *anc_grid = memnew(GridContainer);
	anc_grid->set_columns(3);
	anc_grid->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	anc_grid->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	anc_row->add_child(anc_grid);
	sec_pos_body->add_child(anc_row);
	_register_prop_filter_row(anc_row, "anclaje posicion esquina centro", false);

	static const char *k_anc_glyphs[9] = { "NO", "N", "NE", "O", "C", "E", "SO", "S", "SE" };
	static const char *k_anc_tips[9] = {
		"Anclar arriba a la izquierda", "Anclar arriba centrado", "Anclar arriba a la derecha",
		"Anclar centro izquierda", "Anclar en el centro", "Anclar centro derecha",
		"Anclar abajo a la izquierda", "Anclar abajo centrado", "Anclar abajo a la derecha"
	};
	for (int i = 0; i < 9; i++) {
		Button *ab = memnew(Button(k_anc_glyphs[i]));
		ab->set_toggle_mode(true);
		ab->set_custom_minimum_size(Size2(32 * EDSCALE, 30 * EDSCALE));
		ab->set_tooltip_text(String::utf8(k_anc_tips[i]));
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
		r_sp->set_custom_minimum_size(Size2(120 * EDSCALE, 32 * EDSCALE));
		r_sp->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
		make_prop_row(p_parent, p_lbl, p_tip, p_kw, p_adv, r_sp);
	};

	make_spin(sec_pos_body, "Pos X:", String::utf8("Desplazamiento horizontal desde su anclaje."), "x posicion mover", false, -960, 960, prop_ox_spin);
	make_spin(sec_pos_body, "Pos Y:", String::utf8("Desplazamiento vertical desde su anclaje."), "y posicion mover", false, -540, 540, prop_oy_spin);
	make_spin(sec_pos_body, "Ancho:", String::utf8("Anchura en píxeles base 960x540."), "ancho tamano width", false, 16, 960, prop_w_spin);
	make_spin(sec_pos_body, "Alto:", String::utf8("Altura en píxeles base 960x540."), "alto tamano height", false, 16, 540, prop_h_spin);
	make_spin(sec_pos_body, "Capa:", String::utf8("Orden de dibujo (mayor aparece encima)."), "capa orden profundidad", true, -100, 1000, prop_layer_spin);

	prop_visible_chk = memnew(CheckBox("Mostrar en juego"));
	prop_visible_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_pos_body, "Visible:", String::utf8("Muestra u oculta el elemento."), "visible ocultar mostrar", true, prop_visible_chk);

	prop_locked_chk = memnew(CheckBox(String::utf8("Bloquear posición")));
	prop_locked_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_pos_body, "Bloqueado:", String::utf8("Impide mover, redimensionar o borrar accidentalmente este elemento."), "bloquear candado locked", true, prop_locked_chk);

	prop_clip_children_chk = memnew(CheckBox(String::utf8("Recortar hijos al padre")));
	prop_clip_children_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_pos_body, "Recortar hijos:", String::utf8("Recorta los hijos que sobresalen de los límites de este elemento."), "recortar hijos padre clip mask", true, prop_clip_children_chk, "frame,image,button,text,variable,bar,currency,item,radial");

	prop_parent_layout_opt = memnew(OptionButton);
	prop_parent_layout_opt->add_item("Libre", 0); prop_parent_layout_opt->set_item_metadata(0, "free");
	prop_parent_layout_opt->add_item("Pila vertical", 1); prop_parent_layout_opt->set_item_metadata(1, "vertical_list");
	prop_parent_layout_opt->add_item("Pila horizontal", 2); prop_parent_layout_opt->set_item_metadata(2, "horizontal_list");
	prop_parent_layout_opt->add_item("Rejilla", 3); prop_parent_layout_opt->set_item_metadata(3, "grid");
	prop_parent_layout_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_pos_body, String::utf8("Distribución hijos:"), String::utf8("Ordena los hijos automáticamente al redimensionar este contenedor."), "distribucion hijos rejilla pila layout", true, prop_parent_layout_opt, "frame,folder");

	HBoxContainer *pl_hb = memnew(HBoxContainer);
	pl_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_parent_cols_spin = memnew(SpinBox);
	prop_parent_cols_spin->set_min(1); prop_parent_cols_spin->set_max(12); prop_parent_cols_spin->set_step(1);
	prop_parent_cols_spin->set_value(2);
	prop_parent_cols_spin->set_prefix("Col:");
	prop_parent_cols_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_parent_cols_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	pl_hb->add_child(prop_parent_cols_spin);
	prop_parent_space_spin = memnew(SpinBox);
	prop_parent_space_spin->set_min(0); prop_parent_space_spin->set_max(64); prop_parent_space_spin->set_step(1);
	prop_parent_space_spin->set_value(8);
	prop_parent_space_spin->set_prefix("Sep:");
	prop_parent_space_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_parent_space_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	pl_hb->add_child(prop_parent_space_spin);
	make_prop_row(sec_pos_body, "Rejilla/Sep.:", String::utf8("Columnas (rejilla) y separación en px entre hijos automáticos."), "columnas separacion rejilla", true, pl_hb, "frame,folder");

	// Sección 2: Aspecto
	VBoxContainer *shape_body = nullptr;
	add_prop_section(props_fields_box, "Aspecto", false, shape_body);

	prop_show_bg_chk = memnew(CheckBox("Fondo"));
	prop_show_bg_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Mostrar fondo:", String::utf8("Activa el fondo o caja del componente."), "fondo panel", false, prop_show_bg_chk);

	HBoxContainer *style_hb = memnew(HBoxContainer);
	style_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_style_opt = memnew(OptionButton);
	prop_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_clamp_narrow_controls(prop_style_opt, EDSCALE);
	prop_style_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	style_hb->add_child(prop_style_opt);
	prop_edit_style_btn = memnew(Button("Editar estilo"));
	prop_edit_style_btn->set_tooltip_text(String::utf8("Edita colores, bordes y relleno del estilo seleccionado sin salir de esta pestaña."));
	s_make_rounded_action_btn(prop_edit_style_btn);
	prop_edit_style_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_style_pressed));
	style_hb->add_child(prop_edit_style_btn);
	make_prop_row(shape_body, "Estilo:", String::utf8("Estilo de ventana o marco del tema."), "estilo ventana marco editar", false, style_hb);

	prop_shape_opt = memnew(OptionButton);
	prop_shape_opt->add_item(String::utf8("Rectángulo"), 0); prop_shape_opt->set_item_metadata(0, "rect");
	prop_shape_opt->add_item("Cuadrado", 1); prop_shape_opt->set_item_metadata(1, "square");
	prop_shape_opt->add_item("Redondeado", 2); prop_shape_opt->set_item_metadata(2, "rounded");
	prop_shape_opt->add_item(String::utf8("Píldora"), 3); prop_shape_opt->set_item_metadata(3, "pill");
	prop_shape_opt->add_item(String::utf8("Círculo"), 4); prop_shape_opt->set_item_metadata(4, "circle");
	prop_shape_opt->add_item("Elipse", 5); prop_shape_opt->set_item_metadata(5, "ellipse");
	prop_shape_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Forma:", String::utf8("Silueta de la caja o botón."), "forma circulo pildora esquina elipse", true, prop_shape_opt);

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
	make_prop_row(shape_body, "Radio:", String::utf8("Redondeo instantáneo de esquinas."), "radio esquina redondeo", true, rad_hb);

	prop_fill_color_btn = memnew(ColorPickerButton);
	prop_fill_color_btn->set_edit_alpha(true);
	prop_fill_color_btn->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
	prop_fill_color_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Color relleno:", String::utf8("Color principal del fondo del elemento."), "color relleno fondo", true, prop_fill_color_btn);

	HBoxContainer *op_hb = memnew(HBoxContainer);
	prop_opacity_slider = memnew(HSlider);
	prop_opacity_slider->set_min(0);
	prop_opacity_slider->set_max(100);
	prop_opacity_slider->set_step(5);
	prop_opacity_slider->set_value(100);
	prop_opacity_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_opacity_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	op_hb->add_child(prop_opacity_slider);
	prop_opacity_val_lbl = memnew(Label("100%"));
	prop_opacity_val_lbl->set_custom_minimum_size(Size2(38 * EDSCALE, 0));
	op_hb->add_child(prop_opacity_val_lbl);
	make_prop_row(shape_body, "Opacidad:", String::utf8("Transparencia general del elemento (0-100%)."), "opacidad transparencia alpha", false, op_hb);

	prop_recent_colors_box = memnew(HBoxContainer);
	prop_recent_colors_box->add_theme_constant_override("separation", int(3 * EDSCALE));
	make_prop_row(shape_body, "Recientes:", String::utf8("Últimos colores usados en el proyecto."), "paleta colores recientes", true, prop_recent_colors_box);

	prop_grad_chk = memnew(CheckBox("Degradado vertical"));
	prop_grad_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Degradado:", String::utf8("Activa degradado superior/inferior."), "degradado gradiente", true, prop_grad_chk);

	HBoxContainer *grad_cols = memnew(HBoxContainer);
	grad_cols->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_grad_top_btn = memnew(ColorPickerButton);
	prop_grad_top_btn->set_edit_alpha(true);
	prop_grad_top_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_grad_top_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	grad_cols->add_child(prop_grad_top_btn);
	prop_grad_bot_btn = memnew(ColorPickerButton);
	prop_grad_bot_btn->set_edit_alpha(true);
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
	prop_border_col_btn->set_edit_alpha(true);
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
	prop_glow_col_btn->set_edit_alpha(true);
	prop_glow_col_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 24 * EDSCALE));
	prop_glow_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	fx_hb->add_child(prop_glow_col_btn);
	make_prop_row(shape_body, "Efectos:", String::utf8("Sombra suave y brillo exterior."), "sombra brillo glow", true, fx_hb);

	prop_clip_chk = memnew(CheckBox("Recortar contenido"));
	prop_clip_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Recorte:", String::utf8("Recorta hijos o contenido a la silueta."), "recorte mascara clip", true, prop_clip_chk);

	// Sección 3: Texto y enlace
	VBoxContainer *sec_texto_body = nullptr;
	add_prop_section(props_fields_box, "Texto", false, sec_texto_body);

	prop_text_row = memnew(VBoxContainer);
	prop_text_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	sec_texto_body->add_child(prop_text_row);
	prop_text_row->add_child(memnew(Label("Texto / plantilla ({value}):")));
	prop_text_edit = memnew(LineEdit);
	prop_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_text_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	prop_text_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	prop_text_row->add_child(prop_text_edit);
	_register_prop_filter_row(prop_text_row, "texto etiqueta titulo", false, "text,button,variable,bar");

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
	make_prop_row(sec_texto_body, "Fuente:", String::utf8("Fuente importada (.ttf/.otf) para el texto."), "fuente ttf otf tipografia", true, fnt_hb, "text,button,variable,bar,currency,item");

	prop_font_size_slider = memnew(HSlider);
	prop_font_size_slider->set_min(10);
	prop_font_size_slider->set_max(64);
	prop_font_size_slider->set_step(1);
	prop_font_size_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_texto_body, String::utf8("Tamaño fuente:"), String::utf8("Tamaño de letra en píxeles."), "tamano fuente letra", true, prop_font_size_slider, "text,button,variable,bar,currency,item");

	prop_font_color_btn = memnew(ColorPickerButton);
	prop_font_color_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_texto_body, "Color texto:", String::utf8("Color del texto."), "color texto fuente", true, prop_font_color_btn, "text,button,variable,bar,currency,item");

	// Enlace de datos (desde catálogo)
	prop_binding_row = memnew(VBoxContainer);
	prop_binding_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	sec_texto_body->add_child(prop_binding_row);
	prop_binding_row->add_child(memnew(Label("Enlace de datos:")));
	prop_binding_opt = memnew(OptionButton);
	prop_binding_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_binding_opt->set_custom_minimum_size(Size2(72 * EDSCALE, row_h));
	prop_binding_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_binding_changed));
	s_clamp_narrow_controls(prop_binding_opt, EDSCALE);
	prop_binding_row->add_child(prop_binding_opt);

	prop_var_row = memnew(HBoxContainer);
	prop_var_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_binding_row->add_child(prop_var_row);
	prop_var_opt = memnew(OptionButton);
	prop_var_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_var_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_var_opt_selected));
	s_clamp_narrow_controls(prop_var_opt, EDSCALE);
	prop_var_row->add_child(prop_var_opt);
	prop_var_edit = memnew(LineEdit);
	prop_var_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_var_edit->set_placeholder("variable");
	prop_var_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	s_clamp_narrow_controls(prop_var_edit, EDSCALE);
	prop_var_row->add_child(prop_var_edit);

	prop_item_row = memnew(HBoxContainer);
	prop_item_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_binding_row->add_child(prop_item_row);
	prop_item_row->add_child(memnew(Label("Objeto:")));
	prop_item_opt = memnew(OptionButton);
	prop_item_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_item_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_item_opt_selected));
	s_clamp_narrow_controls(prop_item_opt, EDSCALE);
	prop_item_row->add_child(prop_item_opt);
	_register_prop_filter_row(prop_binding_row, "enlace datos vida mana oro nivel variable objeto", false, "text,button,variable,item,bar,image");

	prop_rotatable_chk = memnew(CheckBox(String::utf8("Rotable con ratón")));
	prop_rotatable_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_texto_body, String::utf8("Rotar vista previa:"), String::utf8("Permite girar la imagen con el ratón al inspeccionar un objeto."), "rotar vista previa icono objeto", false, prop_rotatable_chk, "image");

	// Sección 4: Imagen
	VBoxContainer *sec_imagen_body = nullptr;
	add_prop_section(props_fields_box, "Imagen", false, sec_imagen_body);

	prop_image_row = memnew(VBoxContainer);
	prop_image_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_image_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	sec_imagen_body->add_child(prop_image_row);
	prop_image_row->add_child(memnew(Label("Ruta de imagen:")));
	HBoxContainer *im_hb = memnew(HBoxContainer);
	im_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	im_hb->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	im_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	prop_image_row->add_child(im_hb);
	prop_image_edit = memnew(LineEdit);
	prop_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_image_edit->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
	prop_image_edit->set_placeholder("res://img/ui/...");
	prop_image_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	im_hb->add_child(prop_image_edit);
	prop_image_import_btn = memnew(Button(String::utf8("Elegir…")));
	prop_image_import_btn->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
	prop_image_import_btn->set_clip_text(false);
	prop_image_import_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	prop_image_import_btn->set_tooltip_text(String::utf8("Elegir o importar archivo de imagen."));
	s_make_rounded_action_btn(prop_image_import_btn);
	prop_image_import_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_import_image_pressed));
	im_hb->add_child(prop_image_import_btn);
	prop_image_clear_btn = memnew(Button("X"));
	prop_image_clear_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
	prop_image_clear_btn->set_clip_text(false);
	prop_image_clear_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	prop_image_clear_btn->set_tooltip_text(String::utf8("Quitar imagen."));
	s_make_rounded_action_btn(prop_image_clear_btn);
	prop_image_clear_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_clear_image_pressed));
	im_hb->add_child(prop_image_clear_btn);

	HFlowContainer *sm_hb = memnew(HFlowContainer);
	sm_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sm_hb->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	sm_hb->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	prop_image_scale_opt = memnew(OptionButton);
	prop_image_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_image_scale_opt->set_custom_minimum_size(Size2(130 * EDSCALE, 32 * EDSCALE));
	prop_image_scale_opt->set_fit_to_longest_item(true);
	prop_image_scale_opt->set_clip_text(false);
	prop_image_scale_opt->add_item("Estirar", 0); prop_image_scale_opt->set_item_metadata(0, "stretch");
	prop_image_scale_opt->add_item("Ajustar", 1); prop_image_scale_opt->set_item_metadata(1, "fit");
	prop_image_scale_opt->add_item("Rellenar", 2); prop_image_scale_opt->set_item_metadata(2, "fill");
	prop_image_scale_opt->add_item("Mosaico", 3); prop_image_scale_opt->set_item_metadata(3, "tile");
	prop_image_scale_opt->add_item("9-slice", 4); prop_image_scale_opt->set_item_metadata(4, "nine_slice");
	prop_image_scale_opt->add_item("Escala entera", 5); prop_image_scale_opt->set_item_metadata(5, "integer");
	prop_image_scale_opt->add_item("Manual", 6); prop_image_scale_opt->set_item_metadata(6, "manual");
	prop_image_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	sm_hb->add_child(prop_image_scale_opt);
	prop_image_adjust_manual_btn = memnew(Button("Ajustar manualmente"));
	prop_image_adjust_manual_btn->set_custom_minimum_size(Size2(156 * EDSCALE, 32 * EDSCALE));
	prop_image_adjust_manual_btn->set_clip_text(false);
	prop_image_adjust_manual_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	s_make_rounded_action_btn(prop_image_adjust_manual_btn);
	prop_image_adjust_manual_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_image_adjust_manual_pressed));
	sm_hb->add_child(prop_image_adjust_manual_btn);
	prop_nine_slice_spin = memnew(SpinBox);
	prop_nine_slice_spin->set_min(0);
	prop_nine_slice_spin->set_max(128);
	prop_nine_slice_spin->set_prefix("Margen 9-slice: ");
	prop_nine_slice_spin->set_custom_minimum_size(Size2(140 * EDSCALE, 32 * EDSCALE));
	prop_nine_slice_spin->set_tooltip_text(String::utf8("Margen en px para modo 9-slice."));
	prop_nine_slice_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	sm_hb->add_child(prop_nine_slice_spin);
	prop_image_row->add_child(sm_hb);

	prop_img_thumbs_flow = memnew(HFlowContainer);
	prop_img_thumbs_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_img_thumbs_flow->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	prop_img_thumbs_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	prop_image_row->add_child(prop_img_thumbs_flow);

	prop_btn_state_imgs_box = memnew(VBoxContainer);
	prop_btn_state_imgs_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_btn_state_imgs_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	prop_btn_state_imgs_box->hide();
	prop_image_row->add_child(prop_btn_state_imgs_box);
	static const char *k_btn_state_lbls[4] = { "Estado normal:", "Estado encima:", "Estado pulsado:", "Estado desact.:" };
	for (int si = 0; si < 4; si++) {
		HBoxContainer *shb = memnew(HBoxContainer);
		shb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		shb->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
		shb->add_theme_constant_override("separation", int(6 * EDSCALE));
		Label *sl = memnew(Label(String::utf8(k_btn_state_lbls[si])));
		sl->set_custom_minimum_size(Size2(118 * EDSCALE, 32 * EDSCALE));
		sl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		sl->set_clip_text(false);
		sl->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		shb->add_child(sl);
		prop_btn_state_img_edits[si] = memnew(LineEdit);
		prop_btn_state_img_edits[si]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		prop_btn_state_img_edits[si]->set_custom_minimum_size(Size2(90 * EDSCALE, 32 * EDSCALE));
		prop_btn_state_img_edits[si]->set_placeholder("res://...");
		prop_btn_state_img_edits[si]->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
		shb->add_child(prop_btn_state_img_edits[si]);
		Button *sb_imp = memnew(Button(String::utf8("Elegir…")));
		sb_imp->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
		sb_imp->set_clip_text(false);
		sb_imp->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		sb_imp->set_tooltip_text(String::utf8("Elegir imagen para este estado del botón."));
		s_make_rounded_action_btn(sb_imp);
		sb_imp->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_btn_state_import_pressed).bind(si));
		shb->add_child(sb_imp);
		Button *sb_clr = memnew(Button("X"));
		sb_clr->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
		sb_clr->set_clip_text(false);
		sb_clr->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		sb_clr->set_tooltip_text(String::utf8("Quitar imagen de este estado."));
		s_make_rounded_action_btn(sb_clr);
		sb_clr->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_btn_state_clear_pressed).bind(si));
		shb->add_child(sb_clr);
		prop_btn_state_imgs_box->add_child(shb);
	}
	_register_prop_filter_row(prop_image_row, "imagen textura sprite icono estado normal encima pulsado desactivado", false, "image,frame,button,currency,item");

	prop_pixel_art_chk = memnew(CheckBox("Pixelado (Nearest)"));
	prop_pixel_art_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_imagen_body, "Filtro imagen:", String::utf8("Pixelado (Nearest) o Suave interpolado (desactivado)."), "pixel art filtro imagen suave pixelado", false, prop_pixel_art_chk, "image,frame,button,currency,item,hearts");

	// Sección 4B: Datos y distribución del contenedor (UI 2/3 Punto 1)
	add_prop_section(props_fields_box, String::utf8("Datos y cuadrícula"), false, prop_container_box);

	prop_data_source_opt = memnew(OptionButton);
	prop_data_source_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_clamp_narrow_controls(prop_data_source_opt, EDSCALE);
	Array dsrcs = ClockGearsDataManager::get_ui_container_data_sources();
	for (int i = 0; i < dsrcs.size(); i++) {
		Dictionary ds = dsrcs[i];
		prop_data_source_opt->add_item(String(ds.get("label", ds.get("id", ""))), i);
		prop_data_source_opt->set_item_metadata(i, String(ds.get("id", "manual")));
	}
	prop_data_source_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, "Fuente de datos:", String::utf8("Lista real que rellena las casillas de este contenedor."), "fuente datos inventario equipo habilidades ranuras", false, prop_data_source_opt, "container,widget,radial");

	prop_data_filter_opt = memnew(OptionButton);
	prop_data_filter_opt->add_item("Todas", 0); prop_data_filter_opt->set_item_metadata(0, "");
	prop_data_filter_opt->add_item("Consumibles", 1); prop_data_filter_opt->set_item_metadata(1, "consumables");
	prop_data_filter_opt->add_item("Clave / materiales", 2); prop_data_filter_opt->set_item_metadata(2, "key_materials");
	prop_data_filter_opt->add_item("Armas", 3); prop_data_filter_opt->set_item_metadata(3, "weapons");
	prop_data_filter_opt->add_item("Armaduras", 4); prop_data_filter_opt->set_item_metadata(4, "armors");
	prop_data_filter_opt->add_item("Escudos", 5); prop_data_filter_opt->set_item_metadata(5, "shields");
	prop_data_filter_opt->add_item("Accesorios", 6); prop_data_filter_opt->set_item_metadata(6, "accessories");
	prop_data_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, String::utf8("Filtrar cat.:"), String::utf8("Limita los objetos mostrados por categoría."), "filtrar categoria tipo", false, prop_data_filter_opt, "container,widget,radial");

	prop_data_usable_chk = memnew(CheckBox("Solo usables"));
	prop_data_usable_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, String::utf8("Uso en menú:"), String::utf8("Oculta objetos que no pueden usarse en el menú."), "usables filtro", false, prop_data_usable_chk, "container,widget,radial");

	prop_data_sort_opt = memnew(OptionButton);
	prop_data_sort_opt->add_item("Por ID", 0); prop_data_sort_opt->set_item_metadata(0, "id");
	prop_data_sort_opt->add_item("Por nombre", 1); prop_data_sort_opt->set_item_metadata(1, "name");
	prop_data_sort_opt->add_item("Por cantidad", 2); prop_data_sort_opt->set_item_metadata(2, "qty");
	prop_data_sort_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, "Ordenar:", String::utf8("Criterio de ordenación de las entradas."), "ordenar id nombre cantidad", false, prop_data_sort_opt, "container,widget,radial");

	prop_data_equip_slot_spin = memnew(SpinBox);
	prop_data_equip_slot_spin->set_min(0); prop_data_equip_slot_spin->set_max(7); prop_data_equip_slot_spin->set_step(1);
	prop_data_equip_slot_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, String::utf8("Ranura equipo:"), String::utf8("Índice de ranura de equipo (0=Arma, 1=Escudo/Armadura...)."), "ranura equipo compatible", true, prop_data_equip_slot_spin, "container,widget,radial");

	prop_container_layout_opt = memnew(OptionButton);
	prop_container_layout_opt->add_item(String::utf8("Cuadrícula"), 0); prop_container_layout_opt->set_item_metadata(0, "grid");
	prop_container_layout_opt->add_item("Lista vertical", 1); prop_container_layout_opt->set_item_metadata(1, "vertical_list");
	prop_container_layout_opt->add_item("Lista horizontal", 2); prop_container_layout_opt->set_item_metadata(2, "horizontal_list");
	prop_container_layout_opt->add_item("Carrusel", 3); prop_container_layout_opt->set_item_metadata(3, "carousel");
	prop_container_layout_opt->add_item(String::utf8("Pestañas"), 4); prop_container_layout_opt->set_item_metadata(4, "tabs");
	prop_container_layout_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, String::utf8("Disposición:"), String::utf8("Forma de organizar las casillas dentro del contenedor."), "cuadricula lista carrusel pestanas layout", false, prop_container_layout_opt, "container,widget");

	HBoxContainer *cr_hb = memnew(HBoxContainer);
	cr_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_container_cols_spin = memnew(SpinBox);
	prop_container_cols_spin->set_min(1); prop_container_cols_spin->set_max(20); prop_container_cols_spin->set_prefix("Col:");
	prop_container_cols_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_container_cols_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	cr_hb->add_child(prop_container_cols_spin);
	prop_container_rows_spin = memnew(SpinBox);
	prop_container_rows_spin->set_min(0); prop_container_rows_spin->set_max(20); prop_container_rows_spin->set_prefix("Fil:");
	prop_container_rows_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_container_rows_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	cr_hb->add_child(prop_container_rows_spin);
	make_prop_row(prop_container_box, "Col. / Filas:", String::utf8("Columnas visibles y filas (0 = automático)."), "columnas filas", false, cr_hb, "container,widget");

	HBoxContainer *sp_hb = memnew(HBoxContainer);
	sp_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_container_spx_spin = memnew(SpinBox);
	prop_container_spx_spin->set_min(0); prop_container_spx_spin->set_max(64); prop_container_spx_spin->set_prefix("X:");
	prop_container_spx_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_container_spx_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	sp_hb->add_child(prop_container_spx_spin);
	prop_container_spy_spin = memnew(SpinBox);
	prop_container_spy_spin->set_min(0); prop_container_spy_spin->set_max(64); prop_container_spy_spin->set_prefix("Y:");
	prop_container_spy_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_container_spy_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	sp_hb->add_child(prop_container_spy_spin);
	make_prop_row(prop_container_box, String::utf8("Separación:"), String::utf8("Espacio en píxeles entre casillas."), "separacion espacio", false, sp_hb, "container,widget");

	prop_container_scroll_chk = memnew(CheckBox("Permitir scroll"));
	prop_container_scroll_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, "Scroll:", String::utf8("Permite desplazarse cuando hay más elementos que el área visible."), "scroll desplazamiento", false, prop_container_scroll_chk, "container,widget");

	HBoxContainer *emp_hb = memnew(HBoxContainer);
	emp_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_data_show_empty_chk = memnew(CheckBox(String::utf8("Vacías")));
	prop_data_show_empty_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	emp_hb->add_child(prop_data_show_empty_chk);
	prop_data_empty_cnt_spin = memnew(SpinBox);
	prop_data_empty_cnt_spin->set_min(0); prop_data_empty_cnt_spin->set_max(64); prop_data_empty_cnt_spin->set_prefix(String::utf8("Mín:"));
	prop_data_empty_cnt_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_data_empty_cnt_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	emp_hb->add_child(prop_data_empty_cnt_spin);
	make_prop_row(prop_container_box, String::utf8("Casillas vacías:"), String::utf8("Muestra huecos vacíos hasta alcanzar el mínimo indicado."), "vacias huecos ranuras", false, emp_hb, "container,widget,radial");

	prop_slot_action_opt = memnew(OptionButton);
	prop_slot_action_opt->add_item("Mostrar detalle", 0); prop_slot_action_opt->set_item_metadata(0, "show_detail");
	prop_slot_action_opt->add_item("Usar objeto", 1); prop_slot_action_opt->set_item_metadata(1, "item_use");
	prop_slot_action_opt->add_item("Equipar pieza", 2); prop_slot_action_opt->set_item_metadata(2, "equip_apply");
	prop_slot_action_opt->add_item("Tirar objeto", 3); prop_slot_action_opt->set_item_metadata(3, "item_discard");
	prop_slot_action_opt->add_item("Ninguna", 4); prop_slot_action_opt->set_item_metadata(4, "none");
	prop_slot_action_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_container_changed).unbind(1));
	make_prop_row(prop_container_box, "Al aceptar:", String::utf8("Acción al pulsar o aceptar una casilla."), "aceptar usar equipar tirar detalle", false, prop_slot_action_opt, "container,widget,radial");

	prop_create_item_tpl_btn = memnew(Button("Crear plantilla del elemento"));
	s_make_rounded_action_btn(prop_create_item_tpl_btn);
	prop_create_item_tpl_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_create_item_tpl_pressed));
	make_prop_row(prop_container_box, "Plantilla celda:", String::utf8("Crea una subcarpeta repetible para diseñar cada casilla (icono, nombre, cantidad)."), "plantilla elemento celda repetible", false, prop_create_item_tpl_btn, "container,widget");

	HBoxContainer *ss_hb = memnew(HBoxContainer);
	ss_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_slot_style_state_opt = memnew(OptionButton);
	prop_slot_style_state_opt->add_item("Normal", 0); prop_slot_style_state_opt->set_item_metadata(0, "normal");
	prop_slot_style_state_opt->add_item("Seleccionada", 1); prop_slot_style_state_opt->set_item_metadata(1, "selected");
	prop_slot_style_state_opt->add_item("Desactivada", 2); prop_slot_style_state_opt->set_item_metadata(2, "disabled");
	prop_slot_style_state_opt->add_item(String::utf8("Vacía"), 3); prop_slot_style_state_opt->set_item_metadata(3, "empty");
	prop_slot_style_state_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_slot_style_state_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_slot_style_state_selected));
	ss_hb->add_child(prop_slot_style_state_opt);
	prop_slot_style_fill_btn = memnew(ColorPickerButton);
	prop_slot_style_fill_btn->set_edit_alpha(true);
	prop_slot_style_fill_btn->set_custom_minimum_size(Size2(38 * EDSCALE, 28 * EDSCALE));
	prop_slot_style_fill_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_slot_style_edited).unbind(1));
	ss_hb->add_child(prop_slot_style_fill_btn);
	prop_slot_style_border_btn = memnew(ColorPickerButton);
	prop_slot_style_border_btn->set_edit_alpha(true);
	prop_slot_style_border_btn->set_custom_minimum_size(Size2(38 * EDSCALE, 28 * EDSCALE));
	prop_slot_style_border_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_slot_style_edited).unbind(1));
	ss_hb->add_child(prop_slot_style_border_btn);
	prop_slot_style_bw_spin = memnew(SpinBox);
	prop_slot_style_bw_spin->set_min(0); prop_slot_style_bw_spin->set_max(8); prop_slot_style_bw_spin->set_prefix("B:");
	prop_slot_style_bw_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_slot_style_edited).unbind(1));
	ss_hb->add_child(prop_slot_style_bw_spin);
	make_prop_row(prop_container_box, "Estilo casilla:", String::utf8("Colores y borde para los 4 estados de cada ranura."), "estado normal seleccionada desactivada vacia color", true, ss_hb, "container,widget");

	// Sección 5: Acciones
	VBoxContainer *sec_acciones_body = nullptr;
	add_prop_section(props_fields_box, "Acciones", false, sec_acciones_body);

	prop_action_row = memnew(VBoxContainer);
	prop_action_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	sec_acciones_body->add_child(prop_action_row);
	prop_action_row->add_child(memnew(Label(String::utf8("Acción principal del botón:"))));
	prop_action_opt = memnew(OptionButton);
	prop_action_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_action_opt->set_custom_minimum_size(Size2(72 * EDSCALE, row_h));
	prop_action_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_action_changed));
	s_clamp_narrow_controls(prop_action_opt, EDSCALE);
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

	HBoxContainer *sh_hb = memnew(HBoxContainer);
	sh_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	sh_hb->add_child(memnew(Label("Sonido pasar:")));
	prop_sound_hover_edit = memnew(LineEdit);
	prop_sound_hover_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_sound_hover_edit->set_placeholder("res://audio/...");
	prop_sound_hover_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	sh_hb->add_child(prop_sound_hover_edit);
	prop_action_row->add_child(sh_hb);

	HBoxContainer *sc_hb = memnew(HBoxContainer);
	sc_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	sc_hb->add_child(memnew(Label("Sonido pulsar:")));
	prop_sound_click_edit = memnew(LineEdit);
	prop_sound_click_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_sound_click_edit->set_placeholder("res://audio/...");
	prop_sound_click_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	sc_hb->add_child(prop_sound_click_edit);
	prop_action_row->add_child(sc_hb);

	HBoxContainer *ks_hb = memnew(HBoxContainer);
	ks_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	ks_hb->add_child(memnew(Label("Atajo:")));
	prop_btn_shortcut_lbl = memnew(Label("Ninguno"));
	prop_btn_shortcut_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_btn_shortcut_lbl->set_clip_text(true);
	ks_hb->add_child(prop_btn_shortcut_lbl);
	prop_btn_shortcut_capture_btn = memnew(Button("Capturar"));
	s_make_rounded_action_btn(prop_btn_shortcut_capture_btn);
	prop_btn_shortcut_capture_btn->set_tooltip_text(String::utf8("Pulsa una tecla o botón de mando para activar este botón."));
	prop_btn_shortcut_capture_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_btn_shortcut_capture_pressed));
	ks_hb->add_child(prop_btn_shortcut_capture_btn);
	prop_btn_shortcut_clear_btn = memnew(Button("X"));
	prop_btn_shortcut_clear_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
	prop_btn_shortcut_clear_btn->set_clip_text(false);
	prop_btn_shortcut_clear_btn->set_tooltip_text(String::utf8("Quitar atajo de teclado o mando."));
	s_make_rounded_action_btn(prop_btn_shortcut_clear_btn);
	prop_btn_shortcut_clear_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_btn_shortcut_clear_pressed));
	ks_hb->add_child(prop_btn_shortcut_clear_btn);
	prop_action_row->add_child(ks_hb);

	prop_multi_actions_vbox = memnew(VBoxContainer);
	prop_multi_actions_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_action_row->add_child(prop_multi_actions_vbox);

	HBoxContainer *add_act_hb = memnew(HBoxContainer);
	add_act_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_action_row->add_child(add_act_hb);
	prop_add_action_opt = memnew(OptionButton);
	prop_add_action_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_clamp_narrow_controls(prop_add_action_opt, EDSCALE);
	add_act_hb->add_child(prop_add_action_opt);
	prop_add_action_btn = memnew(Button(String::utf8("+ Añadir acción")));
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
	make_prop_row(sec_acciones_body, "Cond. visible:", String::utf8("Visibilidad según interruptor del juego."), "condicion interruptor visible", true, vis_sw_hb, "button");

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
	make_prop_row(sec_acciones_body, "Cond. activo:", String::utf8("Habilita o desactiva el botón según interruptor."), "condicion interruptor activo", true, en_sw_hb, "button");
	_register_prop_filter_row(prop_action_row, "accion boton foco secuencia evento", false, "button");

	// Sección 6: Animación del elemento
	VBoxContainer *sec_anim_body = nullptr;
	add_prop_section(props_fields_box, String::utf8("Animación"), true, sec_anim_body);
	prop_anim_opt = memnew(OptionButton);
	prop_anim_opt->add_item("Ninguna", 0); prop_anim_opt->set_item_metadata(0, "none");
	prop_anim_opt->add_item("Desvanecer", 1); prop_anim_opt->set_item_metadata(1, "fade");
	prop_anim_opt->add_item("Escala", 2); prop_anim_opt->set_item_metadata(2, "scale");
	prop_anim_opt->add_item("Rebote", 3); prop_anim_opt->set_item_metadata(3, "bounce");
	prop_anim_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(sec_anim_body, "Entrada:", String::utf8("Animación de entrada para este elemento."), "animacion entrada rebote escala", true, prop_anim_opt);

	// Sección 7: Rueda radial (solo cuando se selecciona un elemento radial)
	add_prop_section(props_fields_box, "Rueda radial", false, prop_radial_box);

	HBoxContainer *rc_hb = memnew(HBoxContainer);
	rc_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_rad_count_slider = memnew(HSlider);
	prop_rad_count_slider->set_min(1); prop_rad_count_slider->set_max(16); prop_rad_count_slider->set_step(1);
	prop_rad_count_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_rad_count_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_count_changed));
	rc_hb->add_child(prop_rad_count_slider);
	prop_rad_count_lbl = memnew(Label("6"));
	prop_rad_count_lbl->set_custom_minimum_size(Size2(28 * EDSCALE, 0));
	rc_hb->add_child(prop_rad_count_lbl);
	make_prop_row(prop_radial_box, String::utf8("Núm. ranuras:"), String::utf8("Cantidad de huecos en la rueda (1-16)."), "radial ranuras cantidad", false, rc_hb, "radial");

	prop_rad_radius_slider = memnew(HSlider);
	prop_rad_radius_slider->set_min(48); prop_rad_radius_slider->set_max(250); prop_rad_radius_slider->set_step(1);
	prop_rad_radius_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	make_prop_row(prop_radial_box, "Radio rueda:", String::utf8("Radio del anillo en píxeles."), "radial radio", false, prop_rad_radius_slider, "radial");

	prop_rad_start_slider = memnew(HSlider);
	prop_rad_start_slider->set_min(-180); prop_rad_start_slider->set_max(180); prop_rad_start_slider->set_step(1);
	prop_rad_start_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	make_prop_row(prop_radial_box, String::utf8("Ángulo inicio:"), String::utf8("Ángulo de la primera ranura (-180° a 180°)."), "radial angulo inicio", false, prop_rad_start_slider, "radial");

	HBoxContainer *sw_hb = memnew(HBoxContainer);
	sw_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_rad_full_chk = memnew(CheckBox(String::utf8("360°")));
	prop_rad_full_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	sw_hb->add_child(prop_rad_full_chk);
	prop_rad_sweep_slider = memnew(HSlider);
	prop_rad_sweep_slider->set_min(45); prop_rad_sweep_slider->set_max(360); prop_rad_sweep_slider->set_step(5);
	prop_rad_sweep_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_rad_sweep_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	sw_hb->add_child(prop_rad_sweep_slider);
	make_prop_row(prop_radial_box, "Arco:", String::utf8("Círculo completo (360°) o arco parcial."), "radial arco amplitud circulo", false, sw_hb, "radial");

	prop_rad_cw_chk = memnew(CheckBox("Sentido horario"));
	prop_rad_cw_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	make_prop_row(prop_radial_box, String::utf8("Dirección:"), String::utf8("Orden horario o antihorario."), "radial horario direccion", false, prop_rad_cw_chk, "radial");

	prop_rad_slot_size_slider = memnew(HSlider);
	prop_rad_slot_size_slider->set_min(28); prop_rad_slot_size_slider->set_max(120); prop_rad_slot_size_slider->set_step(2);
	prop_rad_slot_size_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	make_prop_row(prop_radial_box, String::utf8("Tamaño hueco:"), String::utf8("Diámetro de cada ranura en píxeles."), "radial tamano ranura", false, prop_rad_slot_size_slider, "radial");

	prop_rad_center_info_chk = memnew(CheckBox("Mostrar info en centro"));
	prop_rad_center_info_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	make_prop_row(prop_radial_box, "Info centro:", String::utf8("Muestra icono, título y descripción de la ranura activa en el centro."), "radial centro info descripcion", false, prop_rad_center_info_chk, "radial");

	prop_rad_open_anim_opt = memnew(OptionButton);
	prop_rad_open_anim_opt->add_item("Giro radial", 0); prop_rad_open_anim_opt->set_item_metadata(0, "radial_spin");
	prop_rad_open_anim_opt->add_item("Escala", 1); prop_rad_open_anim_opt->set_item_metadata(1, "scale");
	prop_rad_open_anim_opt->add_item("Fundido", 2); prop_rad_open_anim_opt->set_item_metadata(2, "fade");
	prop_rad_open_anim_opt->add_item("Ninguna", 3); prop_rad_open_anim_opt->set_item_metadata(3, "none");
	prop_rad_open_anim_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_changed).unbind(1));
	make_prop_row(prop_radial_box, "Apertura:", String::utf8("Animación de apertura de la rueda radial."), "radial animacion apertura", false, prop_rad_open_anim_opt, "radial");

	HBoxContainer *rslot_hb = memnew(HBoxContainer);
	rslot_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_rad_slot_opt = memnew(OptionButton);
	prop_rad_slot_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_rad_slot_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_slot_selected));
	rslot_hb->add_child(prop_rad_slot_opt);
	prop_rad_add_slot_btn = memnew(Button(String::utf8("+ Añadir ranura")));
	s_make_rounded_action_btn(prop_rad_add_slot_btn);
	prop_rad_add_slot_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_radial_add_slot_pressed));
	rslot_hb->add_child(prop_rad_add_slot_btn);
	prop_rad_del_slot_btn = memnew(Button("X"));
	prop_rad_del_slot_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
	prop_rad_del_slot_btn->set_clip_text(false);
	s_make_rounded_action_btn(prop_rad_del_slot_btn);
	prop_rad_del_slot_btn->set_tooltip_text("Eliminar ranura seleccionada.");
	prop_rad_del_slot_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_del_slot_pressed), CONNECT_DEFERRED);
	rslot_hb->add_child(prop_rad_del_slot_btn);
	make_prop_row(prop_radial_box, "Ranura:", String::utf8("Selecciona una ranura para configurar su icono, texto y acciones."), "radial ranura editar", false, rslot_hb, "radial");

	prop_rad_slot_name_edit = memnew(LineEdit);
	prop_rad_slot_name_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_slot_edited).unbind(1));
	make_prop_row(prop_radial_box, "Nombre ranura:", String::utf8("Nombre visible en la ranura y centro."), "radial nombre titulo", false, prop_rad_slot_name_edit, "radial");

	prop_rad_slot_desc_edit = memnew(LineEdit);
	prop_rad_slot_desc_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_slot_edited).unbind(1));
	make_prop_row(prop_radial_box, String::utf8("Descripción:"), String::utf8("Descripción mostrada en el centro."), "radial descripcion", false, prop_rad_slot_desc_edit, "radial");

	HBoxContainer *ric_hb = memnew(HBoxContainer);
	ric_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	prop_rad_slot_icon_edit = memnew(LineEdit);
	prop_rad_slot_icon_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_rad_slot_icon_edit->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
	prop_rad_slot_icon_edit->set_placeholder("res://img/...");
	prop_rad_slot_icon_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_slot_edited).unbind(1));
	ric_hb->add_child(prop_rad_slot_icon_edit);
	prop_rad_slot_icon_btn = memnew(Button(String::utf8("Elegir…")));
	prop_rad_slot_icon_btn->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
	prop_rad_slot_icon_btn->set_clip_text(false);
	s_make_rounded_action_btn(prop_rad_slot_icon_btn);
	prop_rad_slot_icon_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_radial_slot_icon_import_pressed));
	ric_hb->add_child(prop_rad_slot_icon_btn);
	make_prop_row(prop_radial_box, "Icono ranura:", String::utf8("Icono o imagen dibujada en esta ranura."), "radial icono imagen", false, ric_hb, "radial");

	VBoxContainer *ract_wrap = memnew(VBoxContainer);
	ract_wrap->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ract_wrap->add_theme_constant_override("separation", int(4 * EDSCALE));
	HBoxContainer *ract_add_hb = memnew(HBoxContainer);
	ract_add_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_rad_slot_add_act_opt = memnew(OptionButton);
	prop_rad_slot_add_act_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ract_add_hb->add_child(prop_rad_slot_add_act_opt);
	prop_rad_slot_add_act_btn = memnew(Button(String::utf8("+ Añadir acción")));
	s_make_rounded_action_btn(prop_rad_slot_add_act_btn);
	prop_rad_slot_add_act_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_add_multi_action_pressed));
	ract_add_hb->add_child(prop_rad_slot_add_act_btn);
	ract_wrap->add_child(ract_add_hb);
	prop_rad_slot_actions_vbox = memnew(VBoxContainer);
	prop_rad_slot_actions_vbox->add_theme_constant_override("separation", int(3 * EDSCALE));
	ract_wrap->add_child(prop_rad_slot_actions_vbox);
	make_prop_row(prop_radial_box, "Acciones ran.:", String::utf8("Acciones ejecutadas en orden al activar esta ranura."), "radial acciones ranura", false, ract_wrap, "radial");

	// --- Sección contextual consolidada: Ajustes de la pantalla (9B Corr Punto 5) ---
	screen_settings_box = memnew(VBoxContainer);
	screen_settings_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	screen_settings_box->add_theme_constant_override("separation", int(5 * EDSCALE));
	props_scroll_vb->add_child(screen_settings_box);

	screen_settings_body = memnew(VBoxContainer);
	screen_settings_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	screen_settings_body->add_theme_constant_override("separation", int(6 * EDSCALE));
	Button *ss_hdr = _attach_collapsible_header(screen_settings_box, String::utf8("Ajustes de la pantalla"), screen_settings_body, true);
	if (ss_hdr) {
		ss_hdr->set_tooltip_text(String::utf8("Entradas, música, vista previa, créditos, HUD y opciones propias de esta pantalla."));
	}
	screen_settings_box->add_child(screen_settings_body);

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
	mbg_im->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mbg_im->add_theme_constant_override("separation", int(6 * EDSCALE));
	menu_bg_img_edit = memnew(LineEdit);
	menu_bg_img_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_img_edit->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
	menu_bg_img_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	mbg_im->add_child(menu_bg_img_edit);
	menu_bg_img_btn = memnew(Button(String::utf8("Elegir…")));
	menu_bg_img_btn->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
	menu_bg_img_btn->set_clip_text(false);
	s_make_rounded_action_btn(menu_bg_img_btn);
	menu_bg_img_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_bg_import_pressed));
	mbg_im->add_child(menu_bg_img_btn);
	make_prop_row(bg_body, "Imagen fondo:", String::utf8("Imagen de fondo del menú."), "fondo imagen scroll", false, mbg_im);

	HFlowContainer *mbg_sc_flow = memnew(HFlowContainer);
	mbg_sc_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mbg_sc_flow->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	mbg_sc_flow->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	menu_bg_scale_opt = memnew(OptionButton);
	menu_bg_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_scale_opt->set_custom_minimum_size(Size2(120 * EDSCALE, 32 * EDSCALE));
	menu_bg_scale_opt->add_item("Estirar", 0); menu_bg_scale_opt->set_item_metadata(0, "stretch");
	menu_bg_scale_opt->add_item("Ajustar", 1); menu_bg_scale_opt->set_item_metadata(1, "fit");
	menu_bg_scale_opt->add_item("Rellenar", 2); menu_bg_scale_opt->set_item_metadata(2, "fill");
	menu_bg_scale_opt->add_item("Mosaico", 3); menu_bg_scale_opt->set_item_metadata(3, "tile");
	menu_bg_scale_opt->add_item("9-slice", 4); menu_bg_scale_opt->set_item_metadata(4, "nine_slice");
	menu_bg_scale_opt->add_item("Escala entera", 5); menu_bg_scale_opt->set_item_metadata(5, "integer");
	menu_bg_scale_opt->add_item("Manual", 6); menu_bg_scale_opt->set_item_metadata(6, "manual");
	menu_bg_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	mbg_sc_flow->add_child(menu_bg_scale_opt);
	menu_bg_adjust_manual_btn = memnew(Button("Ajustar manualmente"));
	menu_bg_adjust_manual_btn->set_custom_minimum_size(Size2(156 * EDSCALE, 32 * EDSCALE));
	menu_bg_adjust_manual_btn->set_clip_text(false);
	s_make_rounded_action_btn(menu_bg_adjust_manual_btn);
	menu_bg_adjust_manual_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_bg_adjust_manual_pressed));
	mbg_sc_flow->add_child(menu_bg_adjust_manual_btn);
	make_prop_row(bg_body, "Ajuste fondo:", String::utf8("Cómo se escala la imagen de fondo."), "ajuste escala fondo imagen manual", false, mbg_sc_flow);

	menu_bg_pixel_chk = memnew(CheckBox("Pixelado (Nearest)"));
	menu_bg_pixel_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Filtro fondo:", String::utf8("Pixelado nítido o suave para el fondo."), "pixelado suave filtro fondo", false, menu_bg_pixel_chk);

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

	menu_bg_blur_chk = memnew(CheckBox("Desenfocar el juego de fondo"));
	menu_bg_blur_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Desenfoque:", String::utf8("Desenfoca de forma eficiente el juego detrás del menú."), "desenfoque blur juego mapa", false, menu_bg_blur_chk);

	menu_bg_blur_slider = memnew(HSlider);
	menu_bg_blur_slider->set_min(0); menu_bg_blur_slider->set_max(100); menu_bg_blur_slider->set_step(1);
	menu_bg_blur_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Intens. blur:", String::utf8("Intensidad del desenfoque (0-100%)."), "intensidad desenfoque blur", false, menu_bg_blur_slider);

	menu_bg_veil_chk = memnew(CheckBox("Velo de color sobre el juego"));
	menu_bg_veil_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Velo color:", String::utf8("Tiñe suavemente el juego o desenfoque detrás del menú."), "velo oscuro color sombra mapa", false, menu_bg_veil_chk);

	HBoxContainer *veil_hb = memnew(HBoxContainer);
	veil_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	menu_bg_veil_col_btn = memnew(ColorPickerButton);
	menu_bg_veil_col_btn->set_edit_alpha(false);
	menu_bg_veil_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	veil_hb->add_child(menu_bg_veil_col_btn);
	menu_bg_veil_opacity_slider = memnew(HSlider);
	menu_bg_veil_opacity_slider->set_min(0); menu_bg_veil_opacity_slider->set_max(100); menu_bg_veil_opacity_slider->set_step(1);
	menu_bg_veil_opacity_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_veil_opacity_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	veil_hb->add_child(menu_bg_veil_opacity_slider);
	make_prop_row(bg_body, "Color/opac. velo:", String::utf8("Color y opacidad (0-100%) del velo."), "velo opacidad color", false, veil_hb);

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
	s_clamp_narrow_controls(props_card, EDSCALE);
	// Mantener Label "Tipo: ... | ID: ..." con ajuste seguro sin empujar columna
	if (props_sel_lbl) {
		props_sel_lbl->set_custom_minimum_size(Size2(0, 0));
	}

	// --- Diálogos auxiliares, Asistente 3 pasos (4A) y Galería de Plantillas (4B) ---
	import_image_dialog = memnew(FileDialog);
	import_image_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	import_image_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	import_image_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; Imagen");
	import_image_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_image_file_selected));
	add_child(import_image_dialog);

	screen_asset_dialog = memnew(FileDialog);
	screen_asset_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	screen_asset_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	screen_asset_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_asset_file_selected));
	add_child(screen_asset_dialog);

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

	delete_parent_dialog = memnew(ConfirmationDialog);
	delete_parent_dialog->set_title(String::utf8("Eliminar elemento con hijos"));
	delete_parent_dialog->set_text(String::utf8("Este elemento contiene hijos. ¿Qué quieres hacer con ellos?"));
	delete_parent_dialog->get_ok_button()->set_text(String::utf8("Sacar hijos a la raíz"));
	delete_parent_dialog->add_button(String::utf8("Borrar hijos también"), false, "delete_children");
	delete_parent_dialog->get_cancel_button()->set_text("Cancelar");
	delete_parent_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_delete_parent_keep_confirmed));
	delete_parent_dialog->connect("custom_action", callable_mp(this, &ClockGearsUiDesignEditor::_on_delete_parent_custom_action));
	add_child(delete_parent_dialog);

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
	menu_wizard_dialog->set_name("UiMenuWizardDialog");
	menu_wizard_dialog->set_title(String::utf8("Asistente: Nuevo menú"));
	menu_wizard_dialog->set_min_size(Size2i(int(760 * EDSCALE), int(560 * EDSCALE)));
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
	wiz_sc1->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wiz_sc1->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wiz_sc1->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	wizard_step_panels[0] = memnew(VBoxContainer);
	wizard_step_panels[0]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
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
	wizard_step_panels[1]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[1]->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[1]->add_theme_constant_override("separation", int(6 * EDSCALE));
	wizard_step_panels[1]->add_child(memnew(Label(String::utf8("Nombre del menú:"))));
	wizard_name_edit = memnew(LineEdit);
	wizard_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_name_edit->set_text(String::utf8("Mi Menú"));
	wizard_step_panels[1]->add_child(wizard_name_edit);
	wizard_step_panels[1]->add_child(memnew(Label("Botones iniciales:")));
	GridContainer *wiz_chk_grid = memnew(GridContainer);
	wiz_chk_grid->set_columns(3);
	wiz_chk_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[1]->add_child(wiz_chk_grid);
	struct WizBtnDef { const char *label; const char *act; bool def_on; };
	static const WizBtnDef k_wbtns[9] = {
		{ "Inventario", "items", true },
		{ "Estado", "status", true },
		{ "Equipo", "equip", false },
		{ "Guardar", "save", true },
		{ "Opciones", "options", true },
		{ "Cerrar", "close", true },
		{ "Continuar", "continue", false },
		{ "Nueva partida", "new_game", false },
		{ "Salir al t\xC3\xADtulo", "to_title", false }
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
	wizard_step_panels[2]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[2]->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[2]->add_theme_constant_override("separation", int(8 * EDSCALE));
	HBoxContainer *wctx_hb = memnew(HBoxContainer);
	wctx_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wctx_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	wctx_hb->add_child(memnew(Label("Contexto:")));
	wizard_ctx_opt = memnew(OptionButton);
	wizard_ctx_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_ctx_opt->set_fit_to_longest_item(false);
	wizard_ctx_opt->set_clip_text(true);
	wizard_ctx_opt->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
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
	wkey_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wkey_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	wizard_capture_key_btn = memnew(Button("Capturar tecla / mando"));
	wizard_capture_key_btn->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
	wizard_capture_key_btn->set_clip_text(false);
	s_make_rounded_action_btn(wizard_capture_key_btn);
	wizard_capture_key_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_capture_key_pressed));
	wkey_hb->add_child(wizard_capture_key_btn);
	wizard_triggers_lbl = memnew(Label("Sin tecla asignada (opcional)"));
	wizard_triggers_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_triggers_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
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
	template_gallery_dialog->set_name("UiTemplateGalleryDialog");
	template_gallery_dialog->set_title("Biblioteca de plantillas");
	template_gallery_dialog->set_min_size(Size2i(int(760 * EDSCALE), int(560 * EDSCALE)));
	template_gallery_dialog->set_flag(Window::FLAG_POPUP, false);
	template_gallery_dialog->set_transient(true);
	template_gallery_dialog->set_exclusive(false);
	template_gallery_dialog->set_close_on_escape(true);
	template_gallery_dialog->get_ok_button()->hide();
	template_gallery_dialog->connect("close_requested", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_close_requested));
	template_gallery_dialog->connect("canceled", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_close_requested));
	template_gallery_dialog->connect("window_input", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_window_input));
	add_child(template_gallery_dialog);

	VBoxContainer *gal_vb = memnew(VBoxContainer);
	gal_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gal_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	gal_vb->add_theme_constant_override("separation", int(8 * EDSCALE));
	template_gallery_dialog->add_child(gal_vb);

	HBoxContainer *gal_hdr = memnew(HBoxContainer);
	gal_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gal_hdr->add_theme_constant_override("separation", int(8 * EDSCALE));
	gal_vb->add_child(gal_hdr);

	Label *gal_hdr_lbl = memnew(Label(String::utf8("Elige una plantilla o componente para aplicar al diseño actual:")));
	gal_hdr_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gal_hdr_lbl->set_clip_text(true);
	gal_hdr->add_child(gal_hdr_lbl);

	tpl_close_x_btn = memnew(Button(String::utf8("×")));
	tpl_close_x_btn->set_name("TplCloseXBtn");
	tpl_close_x_btn->set_theme_type_variation("ClockGearsCircleCloseButton");
	tpl_close_x_btn->set_custom_minimum_size(Size2(int(28 * EDSCALE), int(28 * EDSCALE)));
	tpl_close_x_btn->set_tooltip_text(String::utf8("Cerrar sin aplicar nada (Esc)"));
	tpl_close_x_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_close_requested));
	gal_hdr->add_child(tpl_close_x_btn);

	template_tabs = memnew(TabContainer);
	template_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	template_tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	gal_vb->add_child(template_tabs);

	auto make_tpl_tab = [&](const String &p_tab_name, GridContainer *&r_grid) {
		ScrollContainer *sc = memnew(ScrollContainer);
		sc->set_name(p_tab_name);
		sc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		sc->set_clip_contents(true);
		sc->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		sc->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
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

	// Ayudas largas (tras cursor quieto) en botones de iconos, acciones y controles del lienzo
	ClockGearsTheme::set_control_long_help(new_menu_wizard_btn, String::utf8("Nuevo diseño\nAbre el asistente de 3 pasos para crear un menú o ventana nueva con su distribución, estilo y botones base."));
	ClockGearsTheme::set_control_long_help(add_submenu_btn, String::utf8("Añadir submenú\nCrea una sub-pantalla anidada dentro del módulo actual para añadir ventanas auxiliares o listados propios."));
	ClockGearsTheme::set_control_long_help(templates_btn, String::utf8("Galería de plantillas\nExplora plantillas completas de pantalla, componentes listos o tus propios diseños guardados."));
	ClockGearsTheme::set_control_long_help(reset_btn, String::utf8("Restaurar plantilla original\nDevuelve la pantalla actual a su configuración predeterminada (pide confirmación antes de sobrescribir)."));
	ClockGearsTheme::set_control_long_help(undo_btn, String::utf8("Deshacer (Ctrl+Z)\nRevierte el último cambio de posición, tamaño, texto o estilo realizado en esta pantalla."));
	ClockGearsTheme::set_control_long_help(redo_btn, String::utf8("Rehacer (Ctrl+Y)\nVuelve a aplicar el último cambio que acabas de deshacer."));
	ClockGearsTheme::set_control_long_help(test_btn, String::utf8("Probar en vivo\nAbre una vista previa interactiva con datos de ejemplo para comprobar el menú o la batalla al completo."));
	ClockGearsTheme::set_control_long_help(preview_res_opt, String::utf8("Resolución de vista previa\nSimula cómo se adapta el diseño en pantallas 16:9 de 960x540, 1366x768, Full HD o QHD."));
	ClockGearsTheme::set_control_long_help(zoom_btns[0], String::utf8("Zoom Ajustar\nEncaja toda la pantalla 16:9 completa dentro del espacio disponible del lienzo."));
	ClockGearsTheme::set_control_long_help(zoom_btns[1], String::utf8("Zoom 200 %\nAmplía el lienzo al doble de su tamaño para alinear detalles píxel a píxel."));
	ClockGearsTheme::set_control_long_help(zoom_btns[2], String::utf8("Zoom 50 %\nReduce el lienzo a la mitad para ver diseños amplios con comodidad."));
	ClockGearsTheme::set_control_long_help(zoom_btns[3], String::utf8("Zoom 100 %\nMuestra el lienzo a escala real 1:1 según la resolución elegida en Vista."));
	ClockGearsTheme::set_control_long_help(zoom_btns[4], String::utf8("Zoom 300 %\nAmplía el lienzo al triple para ajustar iconos, marcos 9-slice y tipografía fina."));
	ClockGearsTheme::set_control_long_help(grid_chk, String::utf8("Mostrar rejilla\nDibuja líneas guía sobre el lienzo para ayudarte a alinear cajas y botones de forma uniforme."));
	ClockGearsTheme::set_control_long_help(grid_step_opt, String::utf8("Tamaño de rejilla\nDefine cada cuántos píxeles se dibujan las celdas de la rejilla y se imantan los elementos (8, 16 o 32 px)."));
	ClockGearsTheme::set_control_long_help(snap_chk, String::utf8("Imán a la rejilla\nAjusta automáticamente la posición y tamaño de cada elemento al paso de rejilla al arrastrar."));
	ClockGearsTheme::set_control_long_help(guides_chk, String::utf8("Guías inteligentes\nMuestra líneas de alineación respecto a bordes, centros y elementos vecinos mientras mueves o escalas."));
	ClockGearsTheme::set_control_long_help(align_center_h_btn, String::utf8("Centrar horizontalmente\nColoca el elemento seleccionado justo en el centro horizontal del lienzo."));
	ClockGearsTheme::set_control_long_help(align_center_v_btn, String::utf8("Centrar verticalmente\nColoca el elemento seleccionado justo en el centro vertical del lienzo."));
	ClockGearsTheme::set_control_long_help(dist_v_btn, String::utf8("Distribuir en vertical\nReparte uniformemente el espacio vertical entre los elementos visibles o seleccionados."));
	ClockGearsTheme::set_control_long_help(dup_elem_btn, String::utf8("Duplicar elemento (Ctrl+D)\nCrea una copia exacta del elemento seleccionado ligeramente desplazada en la misma pantalla."));
	ClockGearsTheme::set_control_long_help(copy_elem_btn, String::utf8("Copiar elemento (Ctrl+C)\nGuarda en el portapapeles el elemento seleccionado para pegarlo aquí o en otra pantalla."));
	ClockGearsTheme::set_control_long_help(paste_elem_btn, String::utf8("Pegar elemento (Ctrl+V)\nInserta una copia del elemento previamente copiado en el diseño activo."));
	ClockGearsTheme::set_control_long_help(context_btn, String::utf8("Contexto\nAbre las opciones de contexto en segundo plano: visibilidad, opacidad, sub-pantallas hermanas y escena de ejemplo en Batalla."));
	ClockGearsTheme::set_control_long_help(view_context_chk, String::utf8("Mostrar contexto\nMuestra las sub-pantallas hermanas elegidas de esta pestaña atenuadas detrás del diseño activo."));
	ClockGearsTheme::set_control_long_help(sample_scene_chk, String::utf8("Escena de ejemplo\nMuestra un campo de batalla y enemigos de muestra detrás de los paneles para comprobar la legibilidad."));
	ClockGearsTheme::set_control_long_help(layer_add_button_btn, String::utf8("+ Botón\nAbre la ventana rápida para crear un botón nuevo con su texto, posición y acción."));
	ClockGearsTheme::set_control_long_help(layer_add_folder_btn, String::utf8("+ Carpeta\nCrea una carpeta organizadora en el árbol de capas para agrupar elementos relacionados."));
	ClockGearsTheme::set_control_long_help(layer_group_btn, String::utf8("Agrupar en carpeta\nMete los elementos seleccionados dentro de una carpeta común para moverlos u ocultarlos juntos."));
	ClockGearsTheme::set_control_long_help(layer_ungroup_btn, String::utf8("Desagrupar carpeta\nSaca los elementos hijos fuera de la carpeta seleccionada manteniendo sus propiedades."));
	ClockGearsTheme::set_control_long_help(layer_up_btn, String::utf8("Subir capa\nMueve el elemento seleccionado un nivel hacia delante en el orden de dibujado."));
	ClockGearsTheme::set_control_long_help(layer_down_btn, String::utf8("Bajar capa\nMueve el elemento seleccionado un nivel hacia atrás en el orden de dibujado."));
	ClockGearsTheme::set_control_long_help(layer_del_btn, String::utf8("Borrar elemento\nElimina del diseño el elemento seleccionado (se puede revertir con Deshacer)."));
	for (int pi = 0; pi < palette_buttons.size(); pi++) {
		if (ClockGearsUiPaletteButton *pb = palette_buttons[pi]) {
			const String tname = pb->get_text();
			const String et = pb->get_element_type();
			ClockGearsTheme::set_control_long_help(pb, tname + String::utf8("\nArrastra este componente al lienzo (o haz clic para añadirlo) para crear un elemento de tipo «") + et + String::utf8("»."));
		}
	}
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

void ClockGearsUiDesignEditor::_relayout_top_toolbar(int p_avail_w) {
	if (!top_row1_flow || !top_row2_flow) {
		return;
	}
	int eff_w = p_avail_w > 0 ? p_avail_w : (get_size().x > 200.0f ? int(get_size().x) : 1366);
	float single_need = 28.0f * EDSCALE;
	const int flow_sep = int(4 * EDSCALE);
	Control *single_items[] = {
		top_bar, row1_sep_undo, undo_hb, wide_sep_rows, res_hb, zoom_hb,
		row2_sep_grid, grid_hb, row2_sep_align, align_hb, row2_sep_clip, clip_hb,
		row1_sep_play, play_hb
	};
	int item_cnt = 0;
	for (Control *c : single_items) {
		if (!c) {
			continue;
		}
		single_need += c->get_combined_minimum_size().x;
		item_cnt++;
	}
	if (item_cnt > 1) {
		single_need += float(item_cnt - 1) * float(flow_sep);
	}
	bool want_single_row = (float(eff_w) >= single_need);
	top_bar_single_row_mode = want_single_row;

	auto move_to = [](Control *p_ctrl, Control *p_target) {
		if (!p_ctrl || !p_target) {
			return;
		}
		if (p_ctrl->get_parent() != p_target) {
			if (p_ctrl->get_parent()) {
				p_ctrl->get_parent()->remove_child(p_ctrl);
			}
			p_target->add_child(p_ctrl);
		}
	};

	if (want_single_row) {
		top_row2_flow->hide();
		move_to(top_bar, top_row1_flow);
		move_to(row1_sep_undo, top_row1_flow);
		move_to(undo_hb, top_row1_flow);
		move_to(wide_sep_rows, top_row1_flow);
		if (wide_sep_rows) wide_sep_rows->show();
		move_to(res_hb, top_row1_flow);
		move_to(zoom_hb, top_row1_flow);
		move_to(row2_sep_grid, top_row1_flow);
		move_to(grid_hb, top_row1_flow);
		move_to(row2_sep_align, top_row1_flow);
		move_to(align_hb, top_row1_flow);
		move_to(row2_sep_clip, top_row1_flow);
		move_to(clip_hb, top_row1_flow);
		move_to(row1_sep_play, top_row1_flow);
		move_to(play_hb, top_row1_flow);
	} else {
		move_to(top_bar, top_row1_flow);
		move_to(row1_sep_undo, top_row1_flow);
		move_to(undo_hb, top_row1_flow);
		move_to(row1_sep_play, top_row1_flow);
		move_to(play_hb, top_row1_flow);

		if (wide_sep_rows) {
			wide_sep_rows->hide();
		}
		move_to(res_hb, top_row2_flow);
		move_to(zoom_hb, top_row2_flow);
		move_to(row2_sep_grid, top_row2_flow);
		move_to(grid_hb, top_row2_flow);
		move_to(row2_sep_align, top_row2_flow);
		move_to(align_hb, top_row2_flow);
		move_to(row2_sep_clip, top_row2_flow);
		move_to(clip_hb, top_row2_flow);
		top_row2_flow->show();
	}
}

void ClockGearsUiDesignEditor::_rebalance_responsive_layout(int p_avail_w) {
	int actual_w = (get_size().x > 200.0f) ? int(Math::round(get_size().x)) : p_avail_w;
	last_adapt_width = actual_w;
	_relayout_top_toolbar(actual_w);
	bool want_compact = (actual_w > 0 && actual_w < int(780 * EDSCALE));
	compact_two_row_layout = want_compact;

	if (want_compact) {
		wide_hbox->hide();
		if (compact_scroll) compact_scroll->show();
		compact_vbox->show();
		s_reparent_ctrl(canvas_card, compact_vbox);
		s_reparent_ctrl(menus_card, compact_vbox);
		s_reparent_ctrl(palette_card, compact_vbox);
		s_reparent_ctrl(props_card, compact_vbox);
		menus_card->set_v_size_flags(Control::SIZE_FILL);
		menus_card->set_custom_minimum_size(Size2(0, 260 * EDSCALE));
		palette_card->set_custom_minimum_size(Size2(0, 0));
		props_card->set_custom_minimum_size(Size2(0, 0));
		canvas_card->set_custom_minimum_size(Size2(0, 280 * EDSCALE));
		if (palette_grid) {
			palette_grid->set_columns(3);
		}
	} else {
		if (compact_scroll) compact_scroll->hide();
		compact_vbox->hide();
		wide_hbox->show();
		float eff_w = float(actual_w > 0 ? actual_w : 1280);
		if (left_col_scroll) {
			left_col_scroll->set_h_size_flags(Control::SIZE_FILL);
			left_col_scroll->set_stretch_ratio(0.0f);
			left_col_scroll->set_clip_contents(true);
			left_col_scroll->show();
		}
		if (wide_right_split) {
			wide_right_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			wide_right_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		}
		if (wide_center_col) {
			wide_center_col->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
			wide_center_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			wide_center_col->set_stretch_ratio(1.0f);
			wide_center_col->set_clip_contents(true);
		}
		if (wide_right_col) {
			wide_right_col->set_h_size_flags(Control::SIZE_FILL);
			wide_right_col->set_stretch_ratio(0.0f);
			wide_right_col->set_clip_contents(true);
		}
		s_reparent_ctrl(menus_card, wide_left_col);
		s_reparent_ctrl(palette_card, wide_left_col);
		s_reparent_ctrl(canvas_card, wide_center_col);
		s_reparent_ctrl(props_card, wide_right_col);
		menus_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		menus_card->set_custom_minimum_size(Size2(0, 0));
		palette_card->set_custom_minimum_size(Size2(0, 0));
		props_card->set_custom_minimum_size(Size2(0, 0));
		props_card->set_clip_contents(true);
		canvas_card->set_custom_minimum_size(Size2(0, 0));
		_apply_wide_split_widths(eff_w);
		callable_mp(this, &ClockGearsUiDesignEditor::_apply_wide_split_widths).bind(eff_w).call_deferred();
	}
	fit_recalc_dirty = true;
	callable_mp(this, &ClockGearsUiDesignEditor::_apply_canvas_zoom_layout).bind(true).call_deferred();
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
	fit_recalc_dirty = true;
	_apply_canvas_zoom_layout(true);
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
	_refresh_screen_settings_panel();
	_refresh_menu_global_properties();
	_refresh_undo_redo_buttons();
}

void ClockGearsUiDesignEditor::refresh_geometry_fast(bool p_refresh_props) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!design_layer || !dm) {
		return;
	}
	design_layer->set_use_editor_test_data(dm->get_editor_test_data_count() >= 0);
	bool is_custom_menu = dm->has_menu(current_screen_id) && !ClockGearsDataManager::is_system_menu_id(current_screen_id);
	bool has_custom = dm->has_custom_ui_design(current_screen_id) || is_custom_menu;
	if (current_screen_id == "hud" && !has_custom) {
		design_layer->apply_prebuilt_screen_design(current_screen_id, dm->get_default_ui_screen_template("hud"));
	} else if (has_custom || current_screen_id == "hud") {
		design_layer->apply_prebuilt_screen_design(current_screen_id, dm->get_ui_screen_design(current_screen_id));
	} else {
		design_layer->apply_prebuilt_screen_design(current_screen_id, dm->get_default_ui_screen_template(current_screen_id));
	}
	if (!design_layer->is_visible()) {
		design_layer->show();
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
			_refresh_background_and_default_preview(true);
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
	_sync_open_floating_windows();
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
		Dictionary d_cur = dm->get_ui_screen_design(current_screen_id);
		Dictionary bg = d_cur.get("background", Dictionary());
		String bt_cur = String(bg.get("type", "default")).to_lower();
		if (bt_cur == "default" || bg.is_empty()) {
			if (current_screen_id == "title") {
				bg = dm->get_title_menu_config().get("background", Dictionary());
			} else if (current_screen_id == "credits") {
				bg = dm->get_credits_config().get("background", Dictionary());
			} else if (current_screen_id == "game_over") {
				bg = dm->get_game_over_config().get("background", Dictionary());
			} else if (current_screen_id.begins_with("game_menu_")) {
				String sub = current_screen_id.trim_prefix("game_menu_");
				if (sub == "main") sub = "menu";
				Dictionary bgs = dm->get_game_menu_config().get("backgrounds", Dictionary());
				if (bgs.has(sub)) {
					bg = bgs[sub];
				}
			}
		}
		ClockGearsTitleScreen::apply_screen_background(canvas_bg_color, canvas_bg_tex, bg, current_screen_id == "game_over" ? "#0c070cff" : "#12151e");
	}

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
	(void)p_has_custom;
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
	// Nota 9B P3: _refresh_canvas_stage solo redibuja el contenido; NUNCA altera zoom, escala ni scroll del lienzo.
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

	float zf = _cg_zoom_factor_for_mode(canvas_zoom_mode);
	Size2 stage_sz(
			MAX(160.0f, float(int(Math::round(float(preview_resolution.x) * zf)))),
			MAX(90.0f, float(int(Math::round(float(preview_resolution.y) * zf)))));
	design_layer->set_viewport_size_override(stage_sz);
	design_layer->set_size(stage_sz);
	design_layer->set_use_editor_test_data(dm->get_editor_test_data_count() >= 0);
	if (current_screen_id == "hud" && !has_custom) {
		Dictionary hud_preview = dm->get_default_ui_screen_template("hud");
		design_layer->apply_prebuilt_screen_design(current_screen_id, hud_preview);
		design_layer->show();
	} else if (!has_custom) {
		Dictionary sys_preview = dm->get_default_ui_screen_template(current_screen_id);
		design_layer->apply_prebuilt_screen_design(current_screen_id, sys_preview);
		design_layer->show();
	} else {
		design_layer->apply_prebuilt_screen_design(current_screen_id, design);
		design_layer->show();
	}
	if (canvas_overlay) {
		canvas_overlay->set_size(stage_sz);
		canvas_overlay->queue_redraw();
	}
	_refresh_context_preview();
}

static Ref<Texture2D> _cg_icon_for_screen_id(const String &p_id, Control *p_ref) {
	ClockGearsIcons *ci = ClockGearsIcons::get_singleton();
	if (!ci) return Ref<Texture2D>();
	String iname = "ui_frame";
	if (p_id == "messages" || p_id == "battle_notice") iname = "cmd_message";
	else if (p_id == "messages_choices" || p_id == "battle") iname = "ui_button";
	else if (p_id == "messages_number" || p_id == "battle_party") iname = "ui_bar";
	else if (p_id == "messages_history" || p_id == "battle_submenu") iname = "ui_templates";
	else if (p_id == "messages_gold") iname = "ui_item";
	else if (p_id == "battle_enemies") iname = "ui_image";
	else if (p_id == "battle_target") iname = "ui_widget";
	else if (p_id == "title") iname = "cmd_scene";
	return ci->get_icon_with_fallback(iname, "db_system", 1.0f);
}

static void s_set_subtree_mouse_ignore(Node *p_node) {
	if (!p_node) {
		return;
	}
	if (Control *c = Object::cast_to<Control>(p_node)) {
		c->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		c->set_focus_mode(Control::FOCUS_NONE);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		s_set_subtree_mouse_ignore(p_node->get_child(i));
	}
}

HashMap<String, ClockGearsUiDesignEditor::TabContextPrefs> &ClockGearsUiDesignEditor::s_get_context_prefs_store() {
	static HashMap<String, TabContextPrefs> store;
	return store;
}

String ClockGearsUiDesignEditor::_get_active_context_tab_key() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String scope = dm ? dm->get_menu_system_group(current_screen_id) : system_scope_root;
	if (scope.is_empty()) {
		scope = system_scope_root.is_empty() ? current_screen_id : system_scope_root;
	}
	if (scope.is_empty()) {
		scope = "title";
	}
	String proj_key = "";
	if (ProjectSettings::get_singleton()) {
		proj_key = ProjectSettings::get_singleton()->globalize_path("res://").simplify_path().to_lower();
	}
	return proj_key + "::" + scope.to_lower();
}

ClockGearsUiDesignEditor::TabContextPrefs &ClockGearsUiDesignEditor::_get_tab_context_prefs(const String &p_tab_key) {
	HashMap<String, TabContextPrefs> &store = s_get_context_prefs_store();
	if (!store.has(p_tab_key)) {
		store[p_tab_key] = TabContextPrefs();
	}
	return store[p_tab_key];
}

void ClockGearsUiDesignEditor::_rebuild_context_popover_contents(bool p_force_rebuild) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String scope = dm ? dm->get_menu_system_group(current_screen_id) : system_scope_root;
	if (scope.is_empty()) {
		scope = system_scope_root;
	}
	bool has_subscreen_scope = (scope == "messages" || scope == "battle");
	bool is_battle_scope = (scope == "battle");
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	String pop_key = scope + "::" + current_screen_id;
	bool need_rebuild = p_force_rebuild || (last_context_popover_key != pop_key);

	bool prev = context_ui_updating;
	context_ui_updating = true;
	if (view_context_chk) {
		view_context_chk->set_visible(true);
		view_context_chk->set_pressed(prefs.show_context);
	}
	if (context_opacity_slider) {
		context_opacity_slider->set_value(CLAMP(prefs.opacity_pct, 5, 100));
	}
	if (context_opacity_val_lbl) {
		context_opacity_val_lbl->set_text(itos(CLAMP(prefs.opacity_pct, 5, 100)) + " %");
	}
	if (context_siblings_list) {
		int sib_count = 0;
		if (need_rebuild) {
			last_context_popover_key = pop_key;
			while (context_siblings_list->get_child_count() > 0) {
				Node *ch = context_siblings_list->get_child(0);
				context_siblings_list->remove_child(ch);
				ch->queue_free();
			}
			if (has_subscreen_scope && dm) {
				Array menus = dm->get_all_menus_for_selector();
				for (int i = 0; i < menus.size(); i++) {
					Dictionary sd = menus[i];
					String sid = String(sd.get("id", ""));
					if (sid.is_empty() || sid == current_screen_id) {
						continue;
					}
					String sg = String(sd.get("system_group", dm->get_menu_system_group(sid)));
					if (sid != scope && sg != scope) {
						continue;
					}
					bool on = prefs.sibling_enabled.has(sid) ? prefs.sibling_enabled[sid] : true;
					String label_txt = String(sd.get("name", sid));
					CheckBox *cb = memnew(CheckBox(label_txt));
					cb->set_name("CtxSiblingChk_" + sid);
					cb->set_meta("sibling_id", sid);
					cb->set_pressed(on);
					cb->set_clip_text(false);
					cb->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_context_sibling_toggled).bind(sid));
					context_siblings_list->add_child(cb);
					sib_count++;
				}
			}
		} else {
			for (int i = 0; i < context_siblings_list->get_child_count(); i++) {
				CheckBox *cb = Object::cast_to<CheckBox>(context_siblings_list->get_child(i));
				if (cb) {
					String sid = String(cb->get_meta("sibling_id", ""));
					bool on = prefs.sibling_enabled.has(sid) ? prefs.sibling_enabled[sid] : true;
					cb->set_pressed(on);
					sib_count++;
				}
			}
		}
		if (context_siblings_section) {
			context_siblings_section->set_visible(has_subscreen_scope);
		}
		if (context_siblings_empty_lbl) {
			context_siblings_empty_lbl->set_visible(has_subscreen_scope && sib_count == 0);
		}
		if (context_all_btn) {
			context_all_btn->set_visible(sib_count > 0);
		}
		if (context_none_btn) {
			context_none_btn->set_visible(sib_count > 0);
		}
	}
	if (context_sample_section) {
		context_sample_section->set_visible(is_battle_scope);
	}
	if (sample_scene_chk) {
		sample_scene_chk->set_visible(is_battle_scope);
		sample_scene_chk->set_pressed(prefs.sample_scene_enabled);
	}
	if (sample_scene_opacity_slider) {
		sample_scene_opacity_slider->set_value(CLAMP(prefs.sample_opacity_pct, 5, 100));
	}
	if (sample_scene_opacity_lbl) {
		sample_scene_opacity_lbl->set_text(itos(CLAMP(prefs.sample_opacity_pct, 5, 100)) + " %");
	}
	context_ui_updating = prev;
}

void ClockGearsUiDesignEditor::_on_context_btn_pressed() {
	if (!context_popover || !context_btn) {
		return;
	}
	_rebuild_context_popover_contents(true);
	if (ClockGearsTheme *ct = ClockGearsTheme::get_singleton()) {
		ct->apply_to_window(context_popover);
		if (context_pop_panel) {
			ct->apply_to_control(context_pop_panel);
		}
	}
	Rect2 btn_rect = context_btn->get_screen_rect();
	if (Window *pw = context_btn->get_window()) {
		if (pw->is_embedding_subwindows()) {
			btn_rect = context_btn->get_global_rect();
		}
	}
	Size2 min_sz = context_pop_panel ? context_pop_panel->get_combined_minimum_size() : Size2(310 * EDSCALE, 220 * EDSCALE);
	Size2i pop_sz(MAX(int(320 * EDSCALE), int(Math::ceil(min_sz.x)) + int(8 * EDSCALE)),
			MAX(int(140 * EDSCALE), int(Math::ceil(min_sz.y)) + int(8 * EDSCALE)));
	Rect2i usable = ClockGearsTheme::get_window_usable_rect(context_popover);
	Point2i desired_pos(int(Math::round(btn_rect.position.x)), int(Math::round(btn_rect.position.y + btn_rect.size.y + 4.0 * EDSCALE)));
	Rect2i pop_rect = ClockGearsTheme::compute_fitted_popup_rect(usable, pop_sz, Size2i(260, 120), pop_sz, desired_pos, true, pop_sz);
	context_popover->set_size(pop_rect.size);
	context_popover->set_position(pop_rect.position);
	context_popover->popup(pop_rect);
}

void ClockGearsUiDesignEditor::_on_context_chk_toggled(bool p_pressed) {
	if (context_ui_updating) {
		return;
	}
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	prefs.show_context = p_pressed;
	_refresh_context_preview();
}

void ClockGearsUiDesignEditor::_on_context_opacity_changed(double p_val) {
	if (context_ui_updating) {
		return;
	}
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	prefs.opacity_pct = CLAMP(int(Math::round(p_val)), 5, 100);
	if (context_opacity_val_lbl) {
		context_opacity_val_lbl->set_text(itos(prefs.opacity_pct) + " %");
	}
	_refresh_context_preview();
}

void ClockGearsUiDesignEditor::_on_context_sibling_toggled(bool p_pressed, const String &p_sid) {
	if (context_ui_updating || p_sid.is_empty()) {
		return;
	}
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	prefs.sibling_enabled[p_sid] = p_pressed;
	_refresh_context_preview();
}

void ClockGearsUiDesignEditor::_on_context_select_all(bool p_all) {
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	if (context_siblings_list) {
		bool prev = context_ui_updating;
		context_ui_updating = true;
		for (int i = 0; i < context_siblings_list->get_child_count(); i++) {
			CheckBox *cb = Object::cast_to<CheckBox>(context_siblings_list->get_child(i));
			if (cb) {
				String sid = String(cb->get_meta("sibling_id", ""));
				if (!sid.is_empty()) {
					prefs.sibling_enabled[sid] = p_all;
				}
				cb->set_pressed(p_all);
			}
		}
		context_ui_updating = prev;
	}
	_refresh_context_preview();
}

void ClockGearsUiDesignEditor::_on_sample_scene_chk_toggled(bool p_pressed) {
	if (context_ui_updating) {
		return;
	}
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	prefs.sample_scene_enabled = p_pressed;
	_refresh_context_preview();
}

void ClockGearsUiDesignEditor::_on_sample_scene_opacity_changed(double p_val) {
	if (context_ui_updating) {
		return;
	}
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());
	prefs.sample_opacity_pct = CLAMP(int(Math::round(p_val)), 5, 100);
	if (sample_scene_opacity_lbl) {
		sample_scene_opacity_lbl->set_text(itos(prefs.sample_opacity_pct) + " %");
	}
	_refresh_context_preview();
}

void ClockGearsUiDesignEditor::_refresh_context_preview() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String scope = dm ? dm->get_menu_system_group(current_screen_id) : system_scope_root;
	if (scope.is_empty()) scope = system_scope_root;
	bool has_subscreen_scope = (scope == "messages" || scope == "battle");
	bool is_battle_scope = (scope == "battle");
	TabContextPrefs &prefs = _get_tab_context_prefs(_get_active_context_tab_key());

	_rebuild_context_popover_contents(false);

	if (ctx_battle_backdrop_node) {
		bool show_bd = is_battle_scope && prefs.sample_scene_enabled;
		ctx_battle_backdrop_node->set_visible(show_bd);
		ctx_battle_backdrop_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		ctx_battle_backdrop_node->set_modulate(Color(1.0f, 1.0f, 1.0f, CLAMP(prefs.sample_opacity_pct, 5, 100) / 100.0f));
		ctx_battle_backdrop_node->set_size(Vector2(BASE_W, BASE_H));
		if (show_bd) {
			ctx_battle_backdrop_node->queue_redraw();
		}
	}
	if (!context_layers_root) {
		return;
	}
	while (context_layers_root->get_child_count() > 0) {
		Node *ch = context_layers_root->get_child(0);
		context_layers_root->remove_child(ch);
		memdelete(ch);
	}
	bool show_ctx = has_subscreen_scope && prefs.show_context;
	context_layers_root->set_visible(show_ctx);
	context_layers_root->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	context_layers_root->set_modulate(Color(1.0f, 1.0f, 1.0f, CLAMP(prefs.opacity_pct, 5, 100) / 100.0f));
	if (!show_ctx || !dm) {
		return;
	}
	Array menus = dm->get_all_menus_for_selector();
	for (int i = 0; i < menus.size(); i++) {
		Dictionary sd = menus[i];
		String sid = String(sd.get("id", ""));
		if (sid.is_empty() || sid == current_screen_id) continue;
		String sg = String(sd.get("system_group", dm->get_menu_system_group(sid)));
		if (sid != scope && sg != scope) continue;
		bool sib_on = prefs.sibling_enabled.has(sid) ? prefs.sibling_enabled[sid] : true;
		if (!sib_on) continue;
		Dictionary d = dm->has_custom_ui_design(sid) ? dm->get_ui_screen_design(sid) : dm->get_default_ui_screen_template(sid);
		ClockGearsUiDesignLayer *ctx_l = memnew(ClockGearsUiDesignLayer);
		ctx_l->set_name("CtxLayer_" + sid);
		ctx_l->set_preview_mode(true);
		ctx_l->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		ctx_l->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		ctx_l->set_viewport_size_override(Vector2(BASE_W, BASE_H));
		context_layers_root->add_child(ctx_l);
		ctx_l->apply_prebuilt_screen_design(sid, d);
		s_set_subtree_mouse_ignore(ctx_l);
	}
}

void ClockGearsUiDesignEditor::_refresh_screen_dropdown() {
	if (!screen_opt) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array menus = dm ? dm->get_all_menus_for_selector() : ClockGearsDataManager::get_ui_design_screens();
	auto belongs_to_scope = [&](const Dictionary &p_sd) -> bool {
		if (system_scope_root.is_empty()) {
			return true;
		}
		String id = String(p_sd.get("id", ""));
		if (id == system_scope_root) {
			return true;
		}
		String sg = String(p_sd.get("system_group", ""));
		if (sg == system_scope_root) {
			return true;
		}
		if (dm && dm->get_menu_system_group(id) == system_scope_root) {
			return true;
		}
		return false;
	};
	bool prev = updating_ui;
	updating_ui = true;
	screen_opt->clear();
	int sel_idx = 0;
	for (int i = 0; i < menus.size(); i++) {
		Dictionary sd = menus[i];
		if (!belongs_to_scope(sd)) {
			continue;
		}
		String id = String(sd.get("id", ""));
		String lbl = String(sd.get("label", sd.get("name", id)));
		String pid = String(sd.get("parent_id", ""));
		if (!pid.is_empty()) {
			lbl = String::utf8("  ↳ ") + lbl;
		}
		int item_i = screen_opt->get_item_count();
		screen_opt->add_item(lbl, item_i);
		Ref<Texture2D> ic = _cg_icon_for_screen_id(id, screen_opt);
		if (ic.is_valid()) {
			screen_opt->set_item_icon(item_i, ic);
		}
		screen_opt->set_item_metadata(item_i, id);
		if (id == current_screen_id) {
			sel_idx = item_i;
		}
	}
	if (screen_opt->get_item_count() > 0) {
		screen_opt->select(sel_idx);
		const String cur_lbl = screen_opt->get_item_text(sel_idx).trim_prefix(String::utf8("  ↳ "));
		const String cur_id = String(screen_opt->get_item_metadata(sel_idx));
		screen_opt->set_tooltip_text(cur_lbl);
		ClockGearsTheme::set_control_long_help(screen_opt, get_screen_long_help(cur_id, cur_lbl));
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

	if (screens_tree) {
		Vector<Dictionary> custom_list;
		HashMap<String, Dictionary> custom_by_id;
		String sig = "scope=" + system_scope_root;
		for (int i = 0; i < menus.size(); i++) {
			Dictionary md = menus[i];
			String mid = String(md.get("id", ""));
			if (!mid.is_empty() && !ClockGearsDataManager::is_system_menu_id(mid)) {
				custom_list.push_back(md);
				custom_by_id[mid] = md;
				sig += "|" + mid + ":" + String(md.get("name", md.get("label", mid))) + "@" + String(md.get("parent_id", "")) + "#" + String(md.get("system_group", ""));
			}
		}

		bool prev_upd = updating_ui;
		updating_ui = true;
		TreeItem *to_select = nullptr;
		if (screens_tree->get_root() == nullptr || sig != cached_screens_tree_sig) {
			cached_screens_tree_sig = sig;
			screens_tree->clear();
			TreeItem *root_item = screens_tree->create_item();
			if (root_item) {
				HashMap<String, TreeItem *> items_by_id;
				auto add_tree_screen = [&](TreeItem *p_parent, const String &p_label, const String &p_id, const String &p_tip) -> TreeItem * {
					TreeItem *it = screens_tree->create_item(p_parent);
					if (!it) {
						return nullptr;
					}
					it->set_text(0, p_label);
					it->set_metadata(0, p_id);
					it->set_tooltip_text(0, p_tip);
					it->set_collapsed(false);
					items_by_id[p_id] = it;
					if (p_id == current_screen_id) {
						to_select = it;
					}
					return it;
				};
				if (system_scope_root.is_empty()) {
					add_tree_screen(root_item, String::utf8("Menú de inicio"), "title", String::utf8("Pantalla de título del juego, logo, música y entradas."));
					add_tree_screen(root_item, "Opciones", "game_menu_options", String::utf8("Pantalla de volumen, pantalla completa y controles."));
					add_tree_screen(root_item, "Partidas", "game_menu_save", String::utf8("Ranuras para guardar y cargar partidas."));

					TreeItem *gm_branch = screens_tree->create_item(root_item);
					if (gm_branch) {
						gm_branch->set_text(0, String::utf8("Menú del juego"));
						gm_branch->set_selectable(0, false);
						gm_branch->set_tooltip_text(0, String::utf8("Menú de pausa y sus subpantallas principales."));
						gm_branch->set_collapsed(false);
						add_tree_screen(gm_branch, "Principal", "game_menu_main", String::utf8("Entradas y diseño principal del menú de pausa."));
						add_tree_screen(gm_branch, "Inventario", "game_menu_items", String::utf8("Inventario del grupo y vista previa de objetos."));
						add_tree_screen(gm_branch, "Equipo", "game_menu_equip", String::utf8("Ranuras de equipo y estadísticas del personaje."));
						add_tree_screen(gm_branch, "Estado", "game_menu_status", String::utf8("Atributos, nivel y estado detallado del grupo."));
					}

					add_tree_screen(root_item, String::utf8("Créditos"), "credits", String::utf8("Lista de créditos, desplazamiento y música."));
					add_tree_screen(root_item, "Game Over", "game_over", String::utf8("Pantalla de Game Over, música y botones de reintento."));
					add_tree_screen(root_item, "HUD", "hud", String::utf8("Indicadores en pantalla durante el juego (PV, oro, widgets)."));
					add_tree_screen(root_item, "Mensajes", "messages", String::utf8("Cuadro de diálogo, caja de nombre, retrato, elecciones, número e historial."));
					add_tree_screen(root_item, "Batalla", "battle", String::utf8("Comandos de combate, panel de grupo, enemigos y aviso de batalla."));
				} else {
					Dictionary root_meta = dm ? dm->get_menu_metadata(system_scope_root) : Dictionary();
					String root_lbl = String(root_meta.get("name", system_scope_root));
					add_tree_screen(root_item, root_lbl, system_scope_root, String::utf8("Diseño principal de este sistema."));
				}

				// Adjuntar submenús propios recursivamente bajo su padre o grupo de sistema
				HashSet<String> attached;
				for (int pass = 0; pass < 8; pass++) {
					bool progress = false;
					for (int i = 0; i < custom_list.size(); i++) {
						Dictionary md = custom_list[i];
						String mid = String(md.get("id", ""));
						if (attached.has(mid)) {
							continue;
						}
						String sg = String(md.get("system_group", ""));
						if (sg.is_empty() && dm) {
							sg = dm->get_menu_system_group(mid);
						}
						if (!system_scope_root.is_empty() && sg != system_scope_root && String(md.get("parent_id", "")) != system_scope_root) {
							continue;
						}
						String pid = String(md.get("parent_id", ""));
						if (!pid.is_empty() && items_by_id.has(pid)) {
							String mname = String(md.get("name", md.get("label", mid)));
							add_tree_screen(items_by_id[pid], mname, mid, String::utf8("Submenú o variante personalizada anidada."));
							attached.insert(mid);
							progress = true;
						}
					}
					if (!progress) {
						break;
					}
				}

				Vector<Dictionary> leftover_custom;
				for (int i = 0; i < custom_list.size(); i++) {
					Dictionary md = custom_list[i];
					String mid = String(md.get("id", ""));
					if (attached.has(mid)) {
						continue;
					}
					String sg = String(md.get("system_group", ""));
					if (sg.is_empty() && dm) {
						sg = dm->get_menu_system_group(mid);
					}
					if (!system_scope_root.is_empty()) {
						if (sg == system_scope_root) {
							if (TreeItem *rp = items_by_id.has(system_scope_root) ? items_by_id[system_scope_root] : root_item) {
								add_tree_screen(rp, String(md.get("name", md.get("label", mid))), mid, String::utf8("Diseño propio dentro de este sistema."));
							}
						}
					} else {
						leftover_custom.push_back(md);
					}
				}
				if (system_scope_root.is_empty() && !leftover_custom.is_empty()) {
					TreeItem *cust_branch = screens_tree->create_item(root_item);
					if (cust_branch) {
						cust_branch->set_text(0, String::utf8("Menús propios"));
						cust_branch->set_selectable(0, false);
						cust_branch->set_collapsed(false);
						for (int i = 0; i < leftover_custom.size(); i++) {
							String mid = String(leftover_custom[i].get("id", ""));
							String mname = String(leftover_custom[i].get("name", leftover_custom[i].get("label", mid)));
							add_tree_screen(cust_branch, mname, mid, String::utf8("Menú personalizado creado en el proyecto."));
						}
					}
				}
			}
		} else {
			Vector<TreeItem *> q;
			q.push_back(screens_tree->get_root());
			while (!q.is_empty()) {
				TreeItem *cur = q[q.size() - 1];
				q.remove_at(q.size() - 1);
				if (cur != screens_tree->get_root() && cur->is_selectable(0) && String(cur->get_metadata(0)) == current_screen_id) {
					to_select = cur;
					break;
				}
				for (TreeItem *ch = cur->get_first_child(); ch != nullptr; ch = ch->get_next()) {
					q.push_back(ch);
				}
			}
		}
		if (to_select && screens_tree->get_selected() != to_select) {
			to_select->select(0);
		}
		updating_ui = prev_upd;
	}
}

static Ref<Texture2D> s_resolve_ui_tree_icon(Control *p_ctx, const String &p_ed_icon, const String &p_cg_icon) {
	Ref<Texture2D> ic;
	if (p_ctx && p_ctx->is_inside_tree() && !p_ed_icon.is_empty() && p_ctx->has_theme_icon(StringName(p_ed_icon), SNAME("EditorIcons"))) {
		ic = p_ctx->get_theme_icon(StringName(p_ed_icon), SNAME("EditorIcons"));
	}
	if (ic.is_null() && ClockGearsIcons::get_singleton() && !p_cg_icon.is_empty()) {
		ic = ClockGearsIcons::get_singleton()->get_icon(p_cg_icon, float(ClockGearsIcons::get_editor_icon_scale(p_ctx)));
	}
	return ic;
}

static Ref<Texture2D> s_resolve_layer_type_icon(Control *p_ctx, const String &p_type) {
	String t = p_type.strip_edges().to_lower();
	if (t == "folder") return s_resolve_ui_tree_icon(p_ctx, "Folder", "folder");
	if (t == "frame") return s_resolve_ui_tree_icon(p_ctx, "PanelContainer", "ui_frame");
	if (t == "button") return s_resolve_ui_tree_icon(p_ctx, "Button", "ui_button");
	if (t == "text" || t == "variable") return s_resolve_ui_tree_icon(p_ctx, "Label", "ui_text");
	if (t == "image") return s_resolve_ui_tree_icon(p_ctx, "TextureRect", "ui_image");
	if (t == "bar" || t == "hearts") return s_resolve_ui_tree_icon(p_ctx, "ProgressBar", "ui_bar");
	if (t == "currency" || t == "item") return s_resolve_ui_tree_icon(p_ctx, "TextureButton", "ui_item");
	if (t == "radial") return s_resolve_ui_tree_icon(p_ctx, "ViewportTexture", "ui_wizard");
	if (t == "widget" || t == "container") return s_resolve_ui_tree_icon(p_ctx, "Tree", "ui_widget");
	return s_resolve_ui_tree_icon(p_ctx, "Node", "ui_text");
}

static void s_collect_editor_tree_sorted(const Array &p_elems, HashMap<String, Dictionary> &r_by_id, HashMap<String, Vector<String>> &r_children, HashMap<String, int> &r_orig_idx) {
	r_by_id.clear();
	r_children.clear();
	r_orig_idx.clear();
	HashMap<String, String> canon_id;
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = Dictionary(p_elems[i]).duplicate(true);
		String id = String(e.get("id", ""));
		if (id.is_empty()) continue;
		r_by_id[id] = e;
		r_orig_idx[id] = i;
		canon_id[id.to_lower()] = id;
		r_children[id] = Vector<String>();
	}
	r_children[String("")] = Vector<String>();
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		String id = String(Dictionary(p_elems[i]).get("id", ""));
		if (!r_by_id.has(id)) continue;
		String raw_pid = String(r_by_id[id].get("parent_id", "")).strip_edges().to_lower();
		String pid = (!raw_pid.is_empty() && canon_id.has(raw_pid) && canon_id[raw_pid] != id) ? canon_id[raw_pid] : String("");
		r_by_id[id]["parent_id"] = pid;
		r_children[pid].push_back(id);
	}
	HashMap<String, int> sub_max_z;
	for (const KeyValue<String, Dictionary> &kv : r_by_id) {
		sub_max_z[kv.key] = int(kv.value.get("layer_order", 0));
	}
	bool changed = true;
	for (int pass = 0; pass < 32 && changed; pass++) {
		changed = false;
		for (const KeyValue<String, Vector<String>> &kv : r_children) {
			if (kv.key.is_empty() || !sub_max_z.has(kv.key)) continue;
			int mz = sub_max_z[kv.key];
			for (int c = 0; c < kv.value.size(); c++) {
				int cz = sub_max_z[kv.value[c]];
				if (cz > mz) mz = cz;
			}
			if (mz != sub_max_z[kv.key]) {
				sub_max_z[kv.key] = mz;
				changed = true;
			}
		}
	}
	struct SiblingCmp {
		const HashMap<String, Dictionary> *by_id = nullptr;
		const HashMap<String, int> *sub_max = nullptr;
		const HashMap<String, int> *orig_idx = nullptr;
		bool operator()(const String &a, const String &b) const {
			int sa = sub_max->get(a);
			int sb = sub_max->get(b);
			if (sa != sb) return sa > sb;
			int za = int(by_id->get(a).get("layer_order", 0));
			int zb = int(by_id->get(b).get("layer_order", 0));
			if (za != zb) return za > zb;
			return orig_idx->get(a) > orig_idx->get(b);
		}
	};
	SiblingCmp cmp{ &r_by_id, &sub_max_z, &r_orig_idx };
	for (KeyValue<String, Vector<String>> &kv : r_children) {
		if (kv.value.size() > 1) {
			kv.value.sort_custom<SiblingCmp>(cmp);
		}
	}
}

static bool s_parse_virtual_slot_id(const String &p_id, String &r_parent_id, int &r_slot_idx) {
	if (!p_id.begins_with("__slot__::")) {
		return false;
	}
	int sep = p_id.rfind("::");
	if (sep <= 10) {
		return false;
	}
	String num_str = p_id.substr(sep + 2).strip_edges();
	if (!num_str.is_valid_int()) {
		return false;
	}
	r_parent_id = p_id.substr(10, sep - 10);
	r_slot_idx = num_str.to_int();
	return !r_parent_id.is_empty() && r_slot_idx >= 0;
}

static Array s_collect_virtual_slot_dicts(ClockGearsDataManager *p_dm, const String &p_screen_id, const Dictionary &p_parent_elem) {
	Array out;
	String eid = String(p_parent_elem.get("id", ""));
	String etype = String(p_parent_elem.get("type", "")).strip_edges().to_lower();
	Dictionary lay = (p_parent_elem.has("layout") && p_parent_elem["layout"].get_type() == Variant::DICTIONARY)
			? Dictionary(p_parent_elem["layout"])
			: Dictionary();
	String ds = ClockGearsDataManager::normalize_ui_data_source(p_parent_elem.get("data_source", "manual"));
	if (etype == "radial") {
		Array slots = (lay.has("radial_slots") && lay["radial_slots"].get_type() == Variant::ARRAY)
				? Array(lay["radial_slots"])
				: Array();
		if (slots.is_empty() && ds == "manual") {
			slots = ClockGearsDataManager::make_default_radial_slots(6, p_screen_id);
		} else if (slots.is_empty() && ds != "manual" && p_dm) {
			Array entries = p_dm->resolve_ui_container_entries(p_parent_elem, -1, 0, true);
			int cnt = CLAMP(int(entries.size()), 1, 24);
			for (int i = 0; i < cnt; i++) {
				Dictionary d;
				Dictionary ed = entries[i];
				d["id"] = String(ed.get("id", vformat("slot_%d", i + 1)));
				d["title"] = String(ed.get("name", vformat("Ranura %d", i + 1)));
				d["description"] = String(ed.get("description", ""));
				d["icon_path"] = String(ed.get("icon_path", ""));
				slots.push_back(d);
			}
		}
		for (int i = 0; i < slots.size(); i++) {
			if (slots[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary sd = slots[i];
			Dictionary vs;
			String title = String(sd.get("title", sd.get("name", vformat("Ranura %d", i + 1))));
			vs["id"] = "__slot__::" + eid + "::" + itos(i);
			vs["parent_id"] = eid;
			vs["type"] = "slot";
			vs["_is_virtual_slot"] = true;
			vs["_slot_index"] = i;
			vs["_slot_parent_type"] = String("radial");
			vs["name"] = String(sd.get("name", title));
			vs["text"] = title;
			vs["description"] = String(sd.get("description", ""));
			String vmode = String(sd.get("visibility_mode", "always")).strip_edges().to_lower();
			vs["visible"] = (vmode != "hidden" && vmode != "never");
			vs["visibility_mode"] = vmode;
			vs["visibility_switch"] = String(sd.get("visibility_switch", ""));
			vs["enabled_mode"] = String(sd.get("enabled_mode", "always"));
			vs["enabled_switch"] = String(sd.get("enabled_switch", ""));
			vs["locked"] = false;
			vs["layer_order"] = int(p_parent_elem.get("layer_order", 0));
			vs["image_normal"] = String(sd.get("image_normal", ""));
			vs["image_hover"] = String(sd.get("image_hover", ""));
			vs["image_pressed"] = String(sd.get("image_pressed", ""));
			vs["image_disabled"] = String(sd.get("image_disabled", ""));
			vs["image_path"] = String(sd.get("icon_path", sd.get("image_normal", "")));
			vs["icon_path"] = String(sd.get("icon_path", ""));
			vs["action"] = String(sd.get("action", ""));
			vs["actions"] = sd.has("actions") ? Array(sd["actions"]).duplicate(true) : Array();
			Dictionary rlay;
			rlay["radial_slot_shape"] = String(sd.get("shape", lay.get("radial_slot_shape", "circle")));
			vs["layout"] = rlay;
			Dictionary sh;
			sh["fill_color"] = String(sd.get("fill_color", "#1c1f29f0"));
			sh["border_color"] = String(sd.get("border_color", "#f5bc54ff"));
			vs["shape"] = sh;
			vs["font_color"] = String(sd.get("font_color", "#f5f2eaff"));
			vs["slot_scale"] = sd.has("slot_scale") ? float(sd["slot_scale"]) : 1.0f;
			out.push_back(vs);
		}
	} else if (etype == "container") {
		int cols = MAX(1, int(lay.get("columns", 4)));
		int rows_cnt = MAX(1, int(lay.get("rows", 2)));
		int cnt = (ds == "manual") ? MAX(1, MIN(32, int(p_parent_elem.get("data_empty_count", cols * rows_cnt)))) : 0;
		Array entries;
		if (p_dm && ds != "manual") {
			entries = p_dm->resolve_ui_container_entries(p_parent_elem, -1, 0, true);
			cnt = CLAMP(int(entries.size()), 1, 24);
		}
		Dictionary ov_map = (p_parent_elem.has("element_overrides") && p_parent_elem["element_overrides"].get_type() == Variant::DICTIONARY)
				? Dictionary(p_parent_elem["element_overrides"])
				: Dictionary();
		for (int i = 0; i < cnt; i++) {
			Dictionary ov = (ov_map.has(itos(i)) && ov_map[itos(i)].get_type() == Variant::DICTIONARY) ? Dictionary(ov_map[itos(i)]) : Dictionary();
			Dictionary ed = (i < entries.size() && entries[i].get_type() == Variant::DICTIONARY) ? Dictionary(entries[i]) : Dictionary();
			String def_title = !ed.is_empty() ? String(ed.get("name", vformat("Ranura %d", i + 1))) : vformat("Ranura %d", i + 1);
			String title = String(ov.get("name", ov.get("text", def_title)));
			Dictionary vs;
			vs["id"] = "__slot__::" + eid + "::" + itos(i);
			vs["parent_id"] = eid;
			vs["type"] = "slot";
			vs["_is_virtual_slot"] = true;
			vs["_slot_index"] = i;
			vs["_slot_parent_type"] = String("container");
			vs["name"] = title;
			vs["text"] = String(ov.get("text", title));
			vs["description"] = String(ov.get("description", ed.get("description", "")));
			String vmode = String(ov.get("visibility_mode", "always")).strip_edges().to_lower();
			bool vbool = ov.has("visible") ? bool(ov["visible"]) : true;
			vs["visible"] = vbool && (vmode != "hidden" && vmode != "never");
			vs["visibility_mode"] = vmode;
			vs["visibility_switch"] = String(ov.get("visibility_switch", ""));
			vs["enabled_mode"] = String(ov.get("enabled_mode", "always"));
			vs["enabled_switch"] = String(ov.get("enabled_switch", ""));
			vs["locked"] = false;
			vs["layer_order"] = int(p_parent_elem.get("layer_order", 0));
			vs["image_normal"] = String(ov.get("image_normal", ""));
			vs["image_hover"] = String(ov.get("image_hover", ""));
			vs["image_pressed"] = String(ov.get("image_pressed", ""));
			vs["image_disabled"] = String(ov.get("image_disabled", ""));
			vs["image_path"] = String(ov.get("image_normal", ed.get("icon_path", "")));
			vs["action"] = String(ov.get("action", p_parent_elem.get("slot_action", "")));
			vs["actions"] = ov.has("actions") ? Array(ov["actions"]).duplicate(true) : Array();
			Dictionary sh;
			sh["fill_color"] = String(ov.get("fill_color", "#1c1f29f0"));
			sh["border_color"] = String(ov.get("border_color", "#d8a040ff"));
			vs["shape"] = sh;
			vs["font_color"] = String(ov.get("font_color", "#f5f2eaff"));
			out.push_back(vs);
		}
	}
	return out;
}

static bool s_write_virtual_slot_into_parent(const String &p_screen_id, Dictionary &r_parent, int p_slot_idx, const Dictionary &p_slot_elem) {
	if (p_slot_idx < 0 || r_parent.is_empty()) {
		return false;
	}
	String ptype = String(r_parent.get("type", "")).strip_edges().to_lower();
	String s_name = String(p_slot_elem.get("name", "")).strip_edges();
	String s_text = String(p_slot_elem.get("text", s_name)).strip_edges();
	if (s_text.is_empty()) s_text = s_name;
	if (s_name.is_empty()) s_name = s_text;
	bool s_vis = p_slot_elem.has("visible") ? bool(p_slot_elem["visible"]) : true;
	String vmode = String(p_slot_elem.get("visibility_mode", "always")).strip_edges().to_lower();
	if (!s_vis) {
		vmode = "never";
	} else if (vmode == "never" || vmode == "hidden") {
		vmode = "always";
	}
	Dictionary sh = (p_slot_elem.has("shape") && p_slot_elem["shape"].get_type() == Variant::DICTIONARY) ? Dictionary(p_slot_elem["shape"]) : Dictionary();

	if (ptype == "radial") {
		Dictionary lay = (r_parent.has("layout") && r_parent["layout"].get_type() == Variant::DICTIONARY)
				? Dictionary(r_parent["layout"]).duplicate(true)
				: Dictionary();
		Array slots = (lay.has("radial_slots") && lay["radial_slots"].get_type() == Variant::ARRAY)
				? Array(lay["radial_slots"]).duplicate(true)
				: Array();
		if (slots.is_empty()) {
			slots = ClockGearsDataManager::make_default_radial_slots(MAX(6, p_slot_idx + 1), p_screen_id);
		}
		if (p_slot_idx >= slots.size()) {
			return false;
		}
		Dictionary sd = (slots[p_slot_idx].get_type() == Variant::DICTIONARY) ? Dictionary(slots[p_slot_idx]).duplicate(true) : Dictionary();
		sd["name"] = s_name;
		sd["title"] = s_text;
		sd["description"] = String(p_slot_elem.get("description", sd.get("description", "")));
		sd["visibility_mode"] = vmode;
		sd["visibility_switch"] = String(p_slot_elem.get("visibility_switch", sd.get("visibility_switch", "")));
		sd["enabled_mode"] = String(p_slot_elem.get("enabled_mode", sd.get("enabled_mode", "always")));
		sd["enabled_switch"] = String(p_slot_elem.get("enabled_switch", sd.get("enabled_switch", "")));
		sd["image_normal"] = String(p_slot_elem.get("image_normal", sd.get("image_normal", ""))).strip_edges();
		sd["image_hover"] = String(p_slot_elem.get("image_hover", sd.get("image_hover", ""))).strip_edges();
		sd["image_pressed"] = String(p_slot_elem.get("image_pressed", sd.get("image_pressed", ""))).strip_edges();
		sd["image_disabled"] = String(p_slot_elem.get("image_disabled", sd.get("image_disabled", ""))).strip_edges();
		if (p_slot_elem.has("image_path")) {
			sd["icon_path"] = String(p_slot_elem["image_path"]).strip_edges();
		}
		if (p_slot_elem.has("layout") && p_slot_elem["layout"].get_type() == Variant::DICTIONARY) {
			Dictionary rlay = p_slot_elem["layout"];
			if (rlay.has("radial_slot_shape")) {
				sd["shape"] = String(rlay["radial_slot_shape"]).strip_edges().to_lower();
			}
		}
		if (sh.has("fill_color")) sd["fill_color"] = String(sh["fill_color"]);
		if (sh.has("border_color")) sd["border_color"] = String(sh["border_color"]);
		if (p_slot_elem.has("font_color")) sd["font_color"] = String(p_slot_elem["font_color"]);
		if (p_slot_elem.has("action")) sd["action"] = String(p_slot_elem["action"]);
		if (p_slot_elem.has("actions") && p_slot_elem["actions"].get_type() == Variant::ARRAY) {
			sd["actions"] = Array(p_slot_elem["actions"]).duplicate(true);
		}
		slots[p_slot_idx] = sd;
		lay["radial_slots"] = slots;
		r_parent["layout"] = lay;
		return true;
	} else if (ptype == "container") {
		Dictionary ov_map = (r_parent.has("element_overrides") && r_parent["element_overrides"].get_type() == Variant::DICTIONARY)
				? Dictionary(r_parent["element_overrides"]).duplicate(true)
				: Dictionary();
		Dictionary ov = (ov_map.has(itos(p_slot_idx)) && ov_map[itos(p_slot_idx)].get_type() == Variant::DICTIONARY)
				? Dictionary(ov_map[itos(p_slot_idx)]).duplicate(true)
				: Dictionary();
		ov["name"] = s_name;
		ov["text"] = s_text;
		ov["description"] = String(p_slot_elem.get("description", ov.get("description", "")));
		ov["visible"] = s_vis;
		ov["visibility_mode"] = vmode;
		ov["visibility_switch"] = String(p_slot_elem.get("visibility_switch", ov.get("visibility_switch", "")));
		ov["enabled_mode"] = String(p_slot_elem.get("enabled_mode", ov.get("enabled_mode", "always")));
		ov["enabled_switch"] = String(p_slot_elem.get("enabled_switch", ov.get("enabled_switch", "")));
		ov["image_normal"] = String(p_slot_elem.get("image_normal", ov.get("image_normal", ""))).strip_edges();
		ov["image_hover"] = String(p_slot_elem.get("image_hover", ov.get("image_hover", ""))).strip_edges();
		ov["image_pressed"] = String(p_slot_elem.get("image_pressed", ov.get("image_pressed", ""))).strip_edges();
		ov["image_disabled"] = String(p_slot_elem.get("image_disabled", ov.get("image_disabled", ""))).strip_edges();
		if (sh.has("fill_color")) ov["fill_color"] = String(sh["fill_color"]);
		if (sh.has("border_color")) ov["border_color"] = String(sh["border_color"]);
		if (p_slot_elem.has("font_color")) ov["font_color"] = String(p_slot_elem["font_color"]);
		if (p_slot_elem.has("action")) ov["action"] = String(p_slot_elem["action"]);
		if (p_slot_elem.has("actions") && p_slot_elem["actions"].get_type() == Variant::ARRAY) {
			ov["actions"] = Array(p_slot_elem["actions"]).duplicate(true);
		}
		ov_map[itos(p_slot_idx)] = ov;
		r_parent["element_overrides"] = ov_map;
		return true;
	}
	return false;
}

void ClockGearsUiDesignEditor::_refresh_layers_list() {
	if (!design_cards_vbox) {
		return;
	}
	Array elems = get_effective_screen_elements();
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	s_collect_editor_tree_sorted(elems, by_id, children, orig_idx);

	if (!selected_elem_id.is_empty() && !by_id.has(selected_elem_id)) {
		String v_pid;
		int v_idx = -1;
		if (s_parse_virtual_slot_id(selected_elem_id, v_pid, v_idx) && !get_effective_element(selected_elem_id).is_empty()) {
			// Mantener selección de ranura virtual hija válida
		} else {
			String sel_low = selected_elem_id.strip_edges().to_lower();
			selected_elem_id = "";
			for (const KeyValue<String, Dictionary> &kv : by_id) {
				if (kv.key.to_lower() == sel_low) {
					selected_elem_id = kv.key;
					break;
				}
			}
		}
	}
	PackedStringArray clean_sel;
	for (int i = 0; i < selected_elem_ids.size(); i++) {
		bool is_vslot = selected_elem_ids[i].begins_with("__slot__::") && !get_effective_element(selected_elem_ids[i]).is_empty();
		if ((by_id.has(selected_elem_ids[i]) || is_vslot) && !clean_sel.has(selected_elem_ids[i])) {
			clean_sel.push_back(selected_elem_ids[i]);
		}
	}
	if (!selected_elem_id.is_empty() && !clean_sel.has(selected_elem_id)) {
		clean_sel.push_back(selected_elem_id);
	}
	selected_elem_ids = clean_sel;

	design_cards_vbox->clear_drop_preview();
	design_card_rows.clear();
	design_cards_order.clear();
	while (design_cards_vbox->get_child_count() > 0) {
		Node *ch = design_cards_vbox->get_child(design_cards_vbox->get_child_count() - 1);
		design_cards_vbox->remove_child(ch);
		ch->queue_free();
	}

	Ref<Texture2D> eye_on_ic = s_resolve_ui_tree_icon(design_cards_vbox, "GuiVisibilityVisible", "ui_eye_on");
	Ref<Texture2D> eye_off_ic = s_resolve_ui_tree_icon(design_cards_vbox, "GuiVisibilityHidden", "ui_eye_off");
	Ref<Texture2D> lock_on_ic = s_resolve_ui_tree_icon(design_cards_vbox, "Lock", "ui_lock");
	Ref<Texture2D> lock_off_ic = s_resolve_ui_tree_icon(design_cards_vbox, "Unlock", "ui_unlock");

	struct BuildCards {
		static void run(ClockGearsUiDesignEditor *ed, ClockGearsDesignCardsContainer *box, const String &parent_id, int depth,
				const HashMap<String, Dictionary> &by_id, const HashMap<String, Vector<String>> &children,
				HashMap<String, ClockGearsDesignCardRow *> &r_rows, Vector<String> &r_order,
				const Ref<Texture2D> &eye_on, const Ref<Texture2D> &eye_off,
				const Ref<Texture2D> &lock_on, const Ref<Texture2D> &lock_off) {
			if (!children.has(parent_id)) return;
			const Vector<String> &kids = children[parent_id];
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			String sid = ed ? ed->get_current_screen() : String();
			String parent_name;
			if (!parent_id.is_empty() && by_id.has(parent_id)) {
				parent_name = String(by_id[parent_id].get("name", parent_id));
			}
			for (int i = 0; i < kids.size(); i++) {
				const String &eid = kids[i];
				const Dictionary &e = by_id[eid];
				String etype = String(e.get("type", "text")).strip_edges().to_lower();
				bool vis = bool(e.get("visible", true));
				bool locked = bool(e.get("locked", false));
				bool collapsed = bool(e.get("collapsed", false));
				Array vslots;
				bool has_real_children = (children.has(eid) && !children[eid].is_empty());
				if (etype == "container" || (etype == "radial" && !has_real_children)) {
					vslots = s_collect_virtual_slot_dicts(dm, sid, e);
				} else if (etype == "radial") {
					String rad_ds = ClockGearsDataManager::normalize_ui_data_source(e.get("data_source", "manual"));
					if (rad_ds != "manual") {
						vslots = s_collect_virtual_slot_dicts(dm, sid, e);
					}
				}
				bool has_kids = has_real_children || !vslots.is_empty();

				ClockGearsDesignCardRow *row = memnew(ClockGearsDesignCardRow);
				box->add_child(row);
				Ref<Texture2D> type_ic = s_resolve_layer_type_icon(box, etype);
				row->setup(ed, box, e, depth, has_kids, type_ic, vis ? eye_on : eye_off, locked ? lock_on : lock_off, ed->is_element_selected(eid), parent_name);
				r_rows[eid] = row;
				r_order.push_back(eid);

				if (!collapsed) {
					if (has_real_children && !(etype == "radial" && ClockGearsDataManager::normalize_ui_data_source(e.get("data_source", "manual")) != "manual")) {
						run(ed, box, eid, depth + 1, by_id, children, r_rows, r_order, eye_on, eye_off, lock_on, lock_off);
					}
					if (!vslots.is_empty()) {
						Ref<Texture2D> slot_ic = s_resolve_layer_type_icon(box, "button");
						String pname = String(e.get("name", eid));
						for (int si = 0; si < vslots.size(); si++) {
							Dictionary vs = vslots[si];
							String vsid = String(vs.get("id", ""));
							bool vvis = bool(vs.get("visible", true));
							ClockGearsDesignCardRow *srow = memnew(ClockGearsDesignCardRow);
							box->add_child(srow);
							srow->setup(ed, box, vs, depth + 1, false, slot_ic, vvis ? eye_on : eye_off, lock_off, ed->is_element_selected(vsid), pname);
							r_rows[vsid] = srow;
							r_order.push_back(vsid);
						}
					}
				}
			}
		}
	};
	BuildCards::run(this, design_cards_vbox, String(""), 0, by_id, children, design_card_rows, design_cards_order, eye_on_ic, eye_off_ic, lock_on_ic, lock_off_ic);

	if (design_cards_order.is_empty()) {
		Label *empty_lbl = memnew(Label(String::utf8("(Sin elementos en esta pantalla)")));
		empty_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		empty_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
		empty_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		design_cards_vbox->add_child(empty_lbl);
	}

	bool has_sel = !selected_elem_id.is_empty();
	bool sel_is_folder = false;
	if (has_sel && by_id.has(selected_elem_id)) {
		sel_is_folder = (String(by_id[selected_elem_id].get("type", "")) == "folder");
	}
	if (layer_group_btn) layer_group_btn->set_disabled(!has_sel);
	if (layer_ungroup_btn) layer_ungroup_btn->set_disabled(!sel_is_folder);
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
		prop_style_opt->add_item("(Del tema)", 0);
		prop_style_opt->set_item_metadata(0, String(""));
		for (int i = 0; i < styles.size(); i++) {
			if (styles[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary st = styles[i];
			String sid = String(st.get("id", ""));
			String sname = String(st.get("name", sid));
			String fhex = String(st.get("fill_color", "#141824ff"));
			String bhex = String(st.get("border_color", "#e0a040ff"));
			Color fc = Color::html_is_valid(fhex) ? Color::html(fhex) : Color(0.1f, 0.12f, 0.18f, 1.0f);
			Color bc = Color::html_is_valid(bhex) ? Color::html(bhex) : Color(0.88f, 0.62f, 0.25f, 1.0f);
			String ckey = sid + "|" + fhex + "|" + bhex;
			Ref<Texture2D> thumb;
			if (style_thumb_cache.has(ckey)) {
				thumb = style_thumb_cache[ckey];
			} else {
				Ref<Image> tim = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
				for (int y = 0; y < 16; y++) {
					for (int x = 0; x < 16; x++) {
						bool border = (x <= 1 || y <= 1 || x >= 14 || y >= 14);
						tim->set_pixel(x, y, border ? bc : fc);
					}
				}
				thumb = ImageTexture::create_from_image(tim);
				style_thumb_cache[ckey] = thumb;
			}
			int idx = prop_style_opt->get_item_count();
			prop_style_opt->add_icon_item(thumb, sname, idx);
			prop_style_opt->set_item_metadata(idx, sid);
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
	if (prop_rad_slot_add_act_opt && prop_rad_slot_add_act_opt->get_item_count() == 0) {
		prop_rad_slot_add_act_opt->clear();
		Array all_acts = ClockGearsDataManager::get_all_ui_button_action_types();
		for (int i = 0; i < all_acts.size(); i++) {
			Dictionary ad = all_acts[i];
			prop_rad_slot_add_act_opt->add_item(String(ad.get("label", ad.get("id", ""))), i);
			prop_rad_slot_add_act_opt->set_item_metadata(i, String(ad.get("id", "")));
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
	if (selected_elem_id.is_empty()) {
		return Dictionary();
	}
	return get_effective_element(selected_elem_id);
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
		cb->set_tooltip_text(String::utf8("Aplicar color reciente: ") + hex);
		Color sw_col = Color::html(hex);
		Ref<Image> sw_img = Image::create_empty(12, 12, false, Image::FORMAT_RGBA8);
		sw_img->fill(sw_col);
		cb->set_button_icon(ImageTexture::create_from_image(sw_img));
		cb->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		Ref<StyleBoxFlat> sb = memnew(StyleBoxFlat);
		sb->set_bg_color(sw_col);
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
	_populate_multi_actions_list(prop_multi_actions_vbox, p_elem.get("actions", Array()));
}

void ClockGearsUiDesignEditor::_populate_multi_actions_list(VBoxContainer *p_vbox, const Array &p_acts) {
	if (!p_vbox) {
		return;
	}
	s_clear_container_children(p_vbox);
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array all_types = ClockGearsDataManager::get_all_ui_button_action_types();
	auto action_label = [&](const String &p_t) -> String {
		for (int ai = 0; ai < all_types.size(); ai++) {
			Dictionary ad = all_types[ai];
			if (String(ad.get("id", "")) == p_t) {
				return String(ad.get("label", p_t));
			}
		}
		return p_t;
	};
	Array menus = dm ? dm->get_all_menus_for_selector() : Array();
	Array acts = p_acts;
	for (int i = 0; i < acts.size(); i++) {
		if (acts[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary a = acts[i];
		String atype = String(a.get("type", "")).strip_edges().to_lower();
		VBoxContainer *card = memnew(VBoxContainer);
		card->add_theme_constant_override("separation", int(2 * EDSCALE));
		p_vbox->add_child(card);

		HBoxContainer *hb = memnew(HBoxContainer);
		hb->add_theme_constant_override("separation", int(3 * EDSCALE));
		Label *l = memnew(Label(itos(i + 1) + ". " + action_label(atype)));
		l->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		l->set_clip_text(true);
		hb->add_child(l);

		Button *ub = memnew(Button(String::utf8("▲")));
		ub->set_tooltip_text(String::utf8("Subir acción en el orden."));
		ub->set_disabled(i == 0);
		s_make_rounded_action_btn(ub);
		ub->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_move_multi_action_pressed).bind(i, -1), CONNECT_DEFERRED);
		hb->add_child(ub);

		Button *db = memnew(Button(String::utf8("▼")));
		db->set_tooltip_text(String::utf8("Bajar acción en el orden."));
		db->set_disabled(i + 1 >= acts.size());
		s_make_rounded_action_btn(db);
		db->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_move_multi_action_pressed).bind(i, 1), CONNECT_DEFERRED);
		hb->add_child(db);

		Button *rb = memnew(Button("x"));
		rb->set_tooltip_text(String::utf8("Quitar esta acción de la lista."));
		s_make_rounded_action_btn(rb);
		rb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_remove_multi_action_pressed).bind(i), CONNECT_DEFERRED);
		hb->add_child(rb);
		card->add_child(hb);

		if (atype == "open_menu" || atype == "open_system_screen") {
			OptionButton *m_opt = memnew(OptionButton);
			m_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			s_clamp_narrow_controls(m_opt, EDSCALE);
			String cur_m = String(a.get("menu_id", a.get("target", a.get("screen_id", ""))));
			for (int mi = 0; mi < menus.size(); mi++) {
				Dictionary md = menus[mi];
				String mid = String(md.get("id", ""));
				m_opt->add_item(String(md.get("name", mid)), mi);
				m_opt->set_item_metadata(mi, mid);
			}
			s_select_option_meta(m_opt, cur_m);
			String pk = (atype == "open_system_screen") ? "screen_id" : "menu_id";
			m_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_menu).bind(m_opt->get_instance_id(), i, pk));
			card->add_child(m_opt);
		} else if (atype == "play_sound" || atype == "play_music") {
			LineEdit *se = memnew(LineEdit);
			se->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			se->set_placeholder("res://audio/...");
			se->set_text(String(a.get("sound_path", a.get("path", ""))));
			se->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step).bind(i, String("sound_path")));
			card->add_child(se);
		} else if (atype == "common_event") {
			SpinBox *ces = memnew(SpinBox);
			ces->set_min(1);
			ces->set_max(999);
			ces->set_prefix("EC #");
			ces->set_value(int(a.get("common_event_id", 1)));
			ces->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step).bind(i, String("common_event_id")));
			card->add_child(ces);
		} else if (atype == "set_switch") {
			HBoxContainer *swb = memnew(HBoxContainer);
			LineEdit *swe = memnew(LineEdit);
			swe->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			swe->set_placeholder("interruptor");
			swe->set_text(String(a.get("switch_name", "")));
			swe->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step).bind(i, String("switch_name")));
			swb->add_child(swe);
			CheckBox *swc = memnew(CheckBox("ON"));
			swc->set_pressed(bool(a.get("value", true)));
			swc->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step).bind(i, String("value")));
			swb->add_child(swc);
			card->add_child(swb);
		} else if (atype == "set_variable") {
			HBoxContainer *vb = memnew(HBoxContainer);
			LineEdit *ve = memnew(LineEdit);
			ve->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			ve->set_placeholder("variable");
			ve->set_text(String(a.get("variable_name", "")));
			ve->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step).bind(i, String("variable_name")));
			vb->add_child(ve);
			SpinBox *vs = memnew(SpinBox);
			vs->set_min(-99999);
			vs->set_max(99999);
			vs->set_value(int(a.get("value", 0)));
			vs->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step).bind(i, String("value")));
			vb->add_child(vs);
			card->add_child(vb);
		} else if (atype == "hud_visibility") {
			OptionButton *h_opt = memnew(OptionButton);
			h_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			h_opt->add_item("Mostrar", 0); h_opt->set_item_metadata(0, true);
			h_opt->add_item("Ocultar", 1); h_opt->set_item_metadata(1, false);
			h_opt->add_item("Alternar", 2); h_opt->set_item_metadata(2, String("toggle"));
			Variant hv = a.get("visible", true);
			if (hv.get_type() == Variant::STRING && String(hv) == "toggle") {
				h_opt->select(2);
			} else {
				h_opt->select(bool(hv) ? 0 : 1);
			}
			h_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_edit_multi_action_menu).bind(h_opt->get_instance_id(), i, String("visible")));
			card->add_child(h_opt);
		}
	}
}

static Dictionary s_extract_elem_shape_ui_state(ClockGearsDataManager *p_dm, const Dictionary &p_el) {
	Dictionary sh;
	if (p_el.has("shape") && p_el["shape"].get_type() == Variant::DICTIONARY) {
		sh = Dictionary(p_el["shape"]).duplicate(true);
	}
	String ws_id = String(p_el.get("window_style_id", "")).strip_edges().to_lower();
	Dictionary ws;
	if (p_dm) {
		if (!ws_id.is_empty() && ws_id != "default") {
			ws = p_dm->get_window_style(ws_id);
		} else {
			ws = p_dm->get_window_style("rounded");
		}
	}
	String def_shape = String(ws.get("shape_type", ws.get("border_type", "rounded"))).strip_edges().to_lower();
	if (def_shape.is_empty() || def_shape == "none") {
		def_shape = "rect";
	}
	Dictionary out;
	out["shape_type"] = sh.has("shape_type") ? String(sh["shape_type"]) : (p_el.get("shape", Variant()).get_type() == Variant::STRING ? String(p_el["shape"]) : def_shape);
	out["corner_radius"] = sh.has("corner_radius") ? int(sh["corner_radius"]) : int(p_el.get("corner_radius", ws.get("corner_radius", 8)));
	out["fill_color"] = sh.has("fill_color") ? String(sh["fill_color"]) : String(p_el.get("fill_color", ws.get("fill_color", "#14171ff0")));
	out["gradient_enabled"] = sh.has("gradient_enabled") ? bool(sh["gradient_enabled"]) : bool(p_el.get("use_gradient", ws.get("gradient_enabled", false)));
	out["gradient_top_color"] = sh.has("gradient_top_color") ? String(sh["gradient_top_color"]) : String(p_el.get("gradient_top", ws.get("gradient_top_color", "#2b3348ff")));
	out["gradient_bottom_color"] = sh.has("gradient_bottom_color") ? String(sh["gradient_bottom_color"]) : String(p_el.get("gradient_bottom", ws.get("gradient_bottom_color", "#181d2aff")));
	out["border_width"] = sh.has("border_width") ? int(sh["border_width"]) : int(p_el.get("border_width", ws.get("border_width", 2)));
	out["border_color"] = sh.has("border_color") ? String(sh["border_color"]) : String(p_el.get("border_color", ws.get("border_color", "#e0a040ff")));
	out["shadow_enabled"] = sh.has("shadow_enabled") ? bool(sh["shadow_enabled"]) : bool(p_el.get("shadow", ws.get("shadow_enabled", false)));
	out["glow_enabled"] = sh.has("glow_enabled") ? bool(sh["glow_enabled"]) : bool(p_el.get("glow", ws.get("glow_enabled", false)));
	out["glow_color"] = sh.has("glow_color") ? String(sh["glow_color"]) : String(p_el.get("glow_color", ws.get("glow_color", "#f0b44cff")));
	String eff_st = String(out["shape_type"]).strip_edges().to_lower();
	bool def_clip = (eff_st == "circle" || eff_st == "ellipse" || eff_st == "pill" || eff_st == "rounded");
	out["clip_content"] = sh.has("clip_content") ? bool(sh["clip_content"]) : (p_el.has("clip_content") ? bool(p_el["clip_content"]) : def_clip);
	return out;
}

void ClockGearsUiDesignEditor::_apply_properties_filter() {
	String q = props_search_query.strip_edges().to_lower();
	String et = current_selected_elem_type.strip_edges().to_lower();
	for (int i = 0; i < prop_filter_entries.size(); i++) {
		const PropRowFilterEntry &ent = prop_filter_entries[i];
		if (!ent.row) {
			continue;
		}
		bool ok_type = ent.elem_types.is_empty() || et.is_empty() || ent.elem_types.split(",").has(et);
		bool ok_mode = props_advanced_mode || !ent.advanced_only || !q.is_empty();
		bool ok_query = q.is_empty() || ent.search_text.contains(q);
		ent.row->set_visible(ok_type && ok_mode && ok_query);
	}
	for (int i = 0; i < prop_sections.size(); i++) {
		const PropSectionGroup &sg = prop_sections[i];
		if (!sg.header || !sg.body) {
			continue;
		}
		bool any_child_vis = false;
		for (int c = 0; c < sg.body->get_child_count(); c++) {
			Control *cc = Object::cast_to<Control>(sg.body->get_child(c));
			if (cc && cc->is_visible()) {
				any_child_vis = true;
				break;
			}
		}
		bool allow_section = any_child_vis && (props_advanced_mode || !sg.advanced_only || !q.is_empty());
		sg.header->set_visible(allow_section);
		sg.body->set_visible(allow_section && sg.header->is_pressed());
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
	updating_props_panel = true;
	Dictionary el = _get_selected_element_copy();
	if (el.is_empty()) {
		current_selected_elem_type = "";
		if (props_sel_lbl) {
			props_sel_lbl->set_text(String::utf8("Selecciona un elemento o ajusta el menú abajo."));
		}
		if (props_fields_box) {
			props_fields_box->hide();
		}
		_apply_properties_filter();
		updating_props_panel = false;
		return;
	}

	bool prev_upd = updating_ui;
	updating_ui = true;

	String etype = String(el.get("type", "text"));
	current_selected_elem_type = etype;
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
	if (prop_locked_chk) {
		prop_locked_chk->set_pressed(bool(el.get("locked", false)));
	}
	if (prop_clip_children_chk) {
		prop_clip_children_chk->set_pressed(bool(el.get("clip_children", false)));
	}
	if (prop_parent_layout_opt) {
		s_select_option_meta(prop_parent_layout_opt, String(el.get("layout_mode", "free")));
	}
	if (prop_parent_cols_spin) {
		prop_parent_cols_spin->set_value(CLAMP(int(el.get("layout_columns", 2)), 1, 12));
	}
	if (prop_parent_space_spin) {
		prop_parent_space_spin->set_value(CLAMP(int(el.get("layout_spacing", 8)), 0, 64));
	}
	if (prop_show_bg_chk) {
		prop_show_bg_chk->set_pressed(bool(el.get("show_background", false)));
	}
	s_select_option_meta(prop_style_opt, String(el.get("window_style_id", "")));

	Dictionary st_ui = s_extract_elem_shape_ui_state(ClockGearsDataManager::get_singleton(), el);
	s_select_option_meta(prop_shape_opt, String(st_ui.get("shape_type", "rounded")));
	int crad = int(st_ui.get("corner_radius", 8));
	if (prop_radius_slider) prop_radius_slider->set_value(crad);
	if (prop_radius_val_lbl) prop_radius_val_lbl->set_text(itos(crad) + "px");
	int op_pct = int(Math::round(CLAMP(float(el.get("opacity", 1.0)), 0.0f, 1.0f) * 100.0f));
	if (prop_opacity_slider) prop_opacity_slider->set_value(op_pct);
	if (prop_opacity_val_lbl) prop_opacity_val_lbl->set_text(itos(op_pct) + "%");
	String fcol = String(st_ui.get("fill_color", "#232a3aff"));
	if (prop_fill_color_btn && Color::html_is_valid(fcol)) prop_fill_color_btn->set_pick_color(Color::html(fcol));
	if (prop_grad_chk) prop_grad_chk->set_pressed(bool(st_ui.get("gradient_enabled", false)));
	String gt = String(st_ui.get("gradient_top_color", "#2b3348ff"));
	String gb = String(st_ui.get("gradient_bottom_color", "#181d2aff"));
	if (prop_grad_top_btn && Color::html_is_valid(gt)) prop_grad_top_btn->set_pick_color(Color::html(gt));
	if (prop_grad_bot_btn && Color::html_is_valid(gb)) prop_grad_bot_btn->set_pick_color(Color::html(gb));
	if (prop_border_w_slider) prop_border_w_slider->set_value(int(st_ui.get("border_width", 2)));
	String bcol = String(st_ui.get("border_color", "#d8a040ff"));
	if (prop_border_col_btn && Color::html_is_valid(bcol)) prop_border_col_btn->set_pick_color(Color::html(bcol));
	if (prop_shadow_chk) prop_shadow_chk->set_pressed(bool(st_ui.get("shadow_enabled", false)));
	if (prop_glow_chk) prop_glow_chk->set_pressed(bool(st_ui.get("glow_enabled", false)));
	String gcol = String(st_ui.get("glow_color", "#f0b44cff"));
	if (prop_glow_col_btn && Color::html_is_valid(gcol)) prop_glow_col_btn->set_pick_color(Color::html(gcol));
	if (prop_clip_chk) prop_clip_chk->set_pressed(bool(st_ui.get("clip_content", false)));
	if (prop_pixel_art_chk) prop_pixel_art_chk->set_pressed(bool(el.get("pixel_art", true)));
	if (prop_anim_opt) s_select_option_meta(prop_anim_opt, String(el.get("in_anim", "none")));
	_refresh_recent_colors_palette();

	bool has_text = (etype == "text" || etype == "button" || etype == "variable" || etype == "bar");
	bool has_font = (has_text || etype == "currency" || etype == "item");
	prop_text_row->set_visible(has_text);
	if (has_text) {
		prop_text_edit->set_text(etype == "bar" ? String(el.get("label_text", "")) : String(el.get("text", "")));
	}
	if (has_font) {
		s_select_option_meta(prop_font_opt, String(el.get("font_path", "")));
		if (prop_font_size_slider) prop_font_size_slider->set_value(int(el.get("font_size", 16)));
		String tcol = String(el.get("font_color", "#f5f2eaff"));
		if (prop_font_color_btn && Color::html_is_valid(tcol)) prop_font_color_btn->set_pick_color(Color::html(tcol));
	}

	bool has_img = (etype == "image" || etype == "frame" || etype == "button" || etype == "currency" || etype == "item");
	prop_image_row->set_visible(has_img);
	if (has_img) {
		String ikey = (etype == "currency" || etype == "item") ? "icon_path" : "image_path";
		String ip = String(el.get(ikey, ""));
		if (ip.is_empty() && etype == "button") {
			ip = String(el.get("icon_path", ""));
		}
		prop_image_edit->set_text(ip);
		bool has_scale_mode = (etype == "image" || etype == "frame" || etype == "button");
		if (prop_image_scale_opt) {
			prop_image_scale_opt->set_visible(has_scale_mode);
			s_select_option_meta(prop_image_scale_opt, String(el.get("scale_mode", "stretch")));
		}
		if (prop_image_adjust_manual_btn) {
			prop_image_adjust_manual_btn->set_visible(has_scale_mode);
			s_sync_manual_adjust_btn_state(prop_image_adjust_manual_btn, String(el.get("scale_mode", "stretch")), s_elem_has_any_image(el));
		}
		if (prop_nine_slice_spin) {
			prop_nine_slice_spin->set_visible(has_scale_mode);
			prop_nine_slice_spin->set_value(int(el.get("nine_slice_margin", 12)));
		}
		_refresh_image_thumbs_gallery();
	}
	if (prop_btn_state_imgs_box) {
		prop_btn_state_imgs_box->set_visible(etype == "button");
		if (etype == "button") {
			static const char *k_sk[4] = { "image_normal", "image_hover", "image_pressed", "image_disabled" };
			for (int si = 0; si < 4; si++) {
				if (prop_btn_state_img_edits[si]) {
					prop_btn_state_img_edits[si]->set_text(String(el.get(k_sk[si], "")));
				}
			}
		}
	}

	bool has_binding = (etype == "text" || etype == "button" || etype == "variable" || etype == "image");
	prop_binding_row->set_visible(has_binding || etype == "item" || etype == "bar");
	if (prop_rotatable_chk) {
		prop_rotatable_chk->set_visible(etype == "image");
		prop_rotatable_chk->set_pressed(bool(el.get("rotatable", false)));
	}
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
		if (prop_sound_hover_edit) prop_sound_hover_edit->set_text(String(el.get("sound_hover", "")));
		if (prop_sound_click_edit) prop_sound_click_edit->set_text(String(el.get("sound_click", "")));
		if (prop_btn_shortcut_lbl) {
			Dictionary sc = el.get("shortcut", Dictionary());
			prop_btn_shortcut_lbl->set_text(sc.is_empty() ? "Ninguno" : s_format_trigger_text(sc));
		}
		String vmode_flat = String(el.get("visibility_mode", "always")).strip_edges().to_lower();
		String v_ui = "always";
		String v_sw = String(el.get("visibility_switch", "")).strip_edges();
		if (vmode_flat == "switch") {
			v_ui = bool(el.get("visibility_invert", false)) ? "switch_off" : "switch_on";
		} else if (el.has("visible_condition") && el["visible_condition"].get_type() == Variant::DICTIONARY) {
			Dictionary vc = el["visible_condition"];
			v_ui = String(vc.get("mode", "always"));
			if (v_sw.is_empty()) v_sw = String(vc.get("switch_name", ""));
		}
		s_select_option_meta(prop_vis_mode_opt, v_ui);
		if (prop_vis_sw_edit) prop_vis_sw_edit->set_text(v_sw);

		String emode_flat = String(el.get("enabled_mode", "always")).strip_edges().to_lower();
		String e_ui = "always";
		String e_sw = String(el.get("enabled_switch", "")).strip_edges();
		if (emode_flat == "switch") {
			e_ui = bool(el.get("enabled_invert", false)) ? "switch_off" : "switch_on";
		} else if (el.has("enabled_condition") && el["enabled_condition"].get_type() == Variant::DICTIONARY) {
			Dictionary ec = el["enabled_condition"];
			e_ui = String(ec.get("mode", "always"));
			if (e_sw.is_empty()) e_sw = String(ec.get("switch_name", ""));
		}
		s_select_option_meta(prop_en_mode_opt, e_ui);
		if (prop_en_sw_edit) prop_en_sw_edit->set_text(e_sw);
		_refresh_multi_actions_ui(el);
	}
	if (etype == "radial") {
		_refresh_radial_inspector();
	}
	if (etype == "container" || etype == "widget" || etype == "radial") {
		_refresh_container_inspector(el);
	}

	updating_ui = prev_upd;
	_apply_properties_filter();
	updating_props_panel = false;
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
	if (menu_bg_scale_opt) s_select_option_meta(menu_bg_scale_opt, String(bg.get("scale_mode", "stretch")));
	s_sync_manual_adjust_btn_state(menu_bg_adjust_manual_btn, String(bg.get("scale_mode", "stretch")), !String(bg.get("image_path", "")).strip_edges().is_empty());
	if (menu_bg_pixel_chk) menu_bg_pixel_chk->set_pressed(bg.has("pixel_art") ? bool(bg["pixel_art"]) : true);
	if (menu_bg_scroll_x_spin) menu_bg_scroll_x_spin->set_value(int(bg.get("scroll_speed_x", 0)));
	if (menu_bg_scroll_y_spin) menu_bg_scroll_y_spin->set_value(int(bg.get("scroll_speed_y", 0)));
	if (menu_bg_blur_chk) menu_bg_blur_chk->set_pressed(bool(bg.get("blur_enabled", false)));
	if (menu_bg_blur_slider) menu_bg_blur_slider->set_value(int(bg.get("blur_intensity", 45)));
	if (menu_bg_veil_chk) menu_bg_veil_chk->set_pressed(bool(bg.get("veil_enabled", bg.get("dark_veil", false))));
	String vcol_hex = String(bg.get("veil_color", "#05070ca6"));
	Color vcol = Color::html_is_valid(vcol_hex) ? Color::html(vcol_hex) : Color(0.02f, 0.03f, 0.05f, 0.65f);
	if (menu_bg_veil_col_btn) menu_bg_veil_col_btn->set_pick_color(Color(vcol.r, vcol.g, vcol.b, 1.0f));
	if (menu_bg_veil_opacity_slider) menu_bg_veil_opacity_slider->set_value(int(Math::round(CLAMP(vcol.a, 0.0f, 1.0f) * 100.0f)));

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
	if (menu_bgm_edit) {
		String bgm_p = String(d.get("bgm_path", ""));
		if (bgm_p.is_empty()) {
			if (current_screen_id == "title") bgm_p = String(dm->get_title_menu_config().get("bgm_path", ""));
			else if (current_screen_id == "credits") bgm_p = String(dm->get_credits_config().get("bgm_path", ""));
			else if (current_screen_id == "game_over") bgm_p = String(dm->get_game_over_config().get("bgm_path", ""));
		}
		menu_bgm_edit->set_text(bgm_p);
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
		undo_btn->set_tooltip_text(String::utf8("Deshacer último cambio (Ctrl+Z) (") + itos(uc) + ")");
		if (undo_btn->get_button_icon().is_valid()) {
			undo_btn->set_text(String());
		} else {
			undo_btn->set_text("Deshacer");
		}
	}
	if (redo_btn) {
		redo_btn->set_disabled(rc <= 0);
		redo_btn->set_tooltip_text(String::utf8("Rehacer cambio deshecho (Ctrl+Y) (") + itos(rc) + ")");
		if (redo_btn->get_button_icon().is_valid()) {
			redo_btn->set_text(String());
		} else {
			redo_btn->set_text("Rehacer");
		}
	}
}

void ClockGearsUiDesignEditor::_notify_data_mutated(bool p_mark_dirty) {
	refresh_from_data_manager();
	_sync_open_floating_windows();
	if (p_mark_dirty) {
		emit_signal("design_modified");
	}
}

void ClockGearsUiDesignEditor::_trace_log(const String &p_event, const String &p_detail) const {
	static bool s_trace_opened = false;
	static Ref<FileAccess> s_trace_file;
	if (!s_trace_opened) {
		s_trace_opened = true;
		s_trace_file = FileAccess::open("user://ui_editor_trace.log", FileAccess::WRITE);
	}
	if (s_trace_file.is_valid()) {
		String ts = Time::get_singleton() ? Time::get_singleton()->get_time_string_from_system() : String("00:00:00");
		s_trace_file->store_line("[" + ts + "] " + p_event + (p_detail.is_empty() ? String("") : String(" | ") + p_detail));
		s_trace_file->flush();
	}
}

void ClockGearsUiDesignEditor::_on_screens_tree_selected() {
	if (updating_ui || !screens_tree) {
		return;
	}
	TreeItem *sel = screens_tree->get_selected();
	if (!sel) {
		return;
	}
	String mid = String(sel->get_metadata(0));
	_trace_log("tree_selected", "meta=" + mid + " current=" + current_screen_id);
	if (!mid.is_empty() && mid != current_screen_id) {
		select_screen(mid);
	}
}

void ClockGearsUiDesignEditor::set_system_scope(const String &p_root_screen_id) {
	String trimmed = p_root_screen_id.strip_edges().to_lower();
	String norm_root = trimmed.is_empty() ? String("") : ClockGearsDataManager::normalize_menu_id(trimmed);
	system_scope_root = norm_root;
	_trace_log("system_scope", "root=" + (norm_root.is_empty() ? String("all") : norm_root));
	if (!system_scope_root.is_empty()) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		bool same_sys = (current_screen_id == system_scope_root);
		if (!same_sys && dm) {
			same_sys = (dm->get_menu_system_group(current_screen_id) == system_scope_root);
		}
		if (!same_sys) {
			select_screen(system_scope_root);
			return;
		}
	}
	refresh_from_data_manager();
}

String ClockGearsUiDesignEditor::add_submenu_to_current(const String &p_name) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	String parent = current_screen_id;
	if (parent.is_empty()) {
		parent = system_scope_root.is_empty() ? String("game_menu_main") : system_scope_root;
	}
	String sid = dm->create_submenu_design(parent, p_name);
	if (!sid.is_empty()) {
		_trace_log("add_submenu", "parent=" + parent + " new=" + sid);
		select_screen(sid);
		_notify_data_mutated(true);
	}
	return sid;
}

void ClockGearsUiDesignEditor::_on_add_submenu_pressed() {
	(void)add_submenu_to_current("");
}

String ClockGearsUiDesignEditor::quick_add_button_element(const String &p_text) {
	String bid = add_element_at("button", Vector2i(BASE_W / 2, BASE_H / 2));
	if (!bid.is_empty() && !p_text.strip_edges().is_empty()) {
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Dictionary el = dm->get_ui_element(current_screen_id, bid);
			if (!el.is_empty()) {
				el["name"] = p_text.strip_edges();
				el["text"] = p_text.strip_edges();
				dm->update_ui_element(current_screen_id, bid, el, false);
				selected_elem_id = bid;
				selected_elem_ids.clear();
				selected_elem_ids.push_back(bid);
				refresh_from_data_manager();
			}
		}
	}
	return bid;
}

void ClockGearsUiDesignEditor::_on_add_button_bar_pressed() {
	(void)open_create_button_window();
}

void ClockGearsUiDesignEditor::_on_layer_add_button_pressed() {
	(void)open_create_button_window();
}

void ClockGearsUiDesignEditor::select_screen(const String &p_screen_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String norm_sid = (p_screen_id.strip_edges().to_lower() == "game_over") ? String("game_over") : ClockGearsDataManager::normalize_menu_id(p_screen_id);
	_trace_log("screen_open_begin", "req=" + p_screen_id + " norm=" + norm_sid);
	if (dm && norm_sid != "game_over" && !dm->has_menu(norm_sid)) {
		Array menus = dm->get_all_menus_for_selector();
		if (!menus.is_empty()) {
			norm_sid = String(Dictionary(menus[0]).get("id", "title"));
		}
	}
	if (!system_scope_root.is_empty() && dm) {
		String sg = dm->get_menu_system_group(norm_sid);
		if (norm_sid != system_scope_root && sg != system_scope_root) {
			system_scope_root = sg;
		}
	}
	if (!current_screen_id.is_empty() && norm_sid != current_screen_id) {
		ScreenZoomState cur_zs;
		cur_zs.zoom_mode = canvas_zoom_mode;
		if (canvas_scroll) {
			cur_zs.scroll_pos = Vector2i(canvas_scroll->get_h_scroll(), canvas_scroll->get_v_scroll());
		}
		per_screen_zoom_state[current_screen_id] = cur_zs;
	}
	cancel_create_button_window();
	cancel_inline_folder_create();
	current_screen_id = norm_sid;
	selected_elem_id = "";
	selected_elem_ids.clear();
	Array elems = get_effective_screen_elements();
	if (!elems.is_empty() && elems[0].get_type() == Variant::DICTIONARY) {
		selected_elem_id = String(Dictionary(elems[0]).get("id", ""));
		if (!selected_elem_id.is_empty()) {
			selected_elem_ids.push_back(selected_elem_id);
		}
	}
	ScreenZoomState target_zs;
	if (per_screen_zoom_state.has(current_screen_id)) {
		target_zs = per_screen_zoom_state[current_screen_id];
	}
	int prev_mode = canvas_zoom_mode;
	canvas_zoom_mode = CLAMP(target_zs.zoom_mode, 0, 4);
	canvas_zoom = (canvas_zoom_mode == 2) ? 2 : ((canvas_zoom_mode == 4) ? 3 : 1);
	for (int i = 0; i < 5; i++) {
		if (zoom_btns[i]) {
			zoom_btns[i]->set_pressed(i == canvas_zoom_mode);
		}
	}
	bool need_fit = (canvas_zoom_mode == 0) && (prev_mode != 0 || last_canvas_viewport_sz.x < 16.0f);
	if (need_fit) {
		fit_recalc_dirty = true;
	}
	_apply_canvas_zoom_layout(need_fit);
	refresh_from_data_manager();
	if (canvas_zoom_mode != 0) {
		callable_mp(this, &ClockGearsUiDesignEditor::_restore_canvas_scroll_pos).bind(target_zs.scroll_pos).call_deferred();
	}
	_trace_log("screen_open_done", "screen=" + current_screen_id + " elems=" + itos(elems.size()) + " sel=" + selected_elem_id);
}

float ClockGearsUiDesignEditor::get_canvas_stage_scale() const {
	float base_ratio = float(MAX(320, preview_resolution.x)) / float(BASE_W);
	return base_ratio * _cg_zoom_factor_for_mode(canvas_zoom_mode);
}

Array ClockGearsUiDesignEditor::get_effective_screen_elements() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return Array();
	}
	Array elems = dm->get_ui_screen_elements(current_screen_id);
	if (!elems.is_empty()) {
		return elems;
	}
	bool is_custom_menu = dm->has_menu(current_screen_id) && !ClockGearsDataManager::is_system_menu_id(current_screen_id);
	if (is_custom_menu || (current_screen_id != "hud" && dm->has_custom_ui_design(current_screen_id))) {
		return elems;
	}
	Dictionary def_tpl = dm->get_default_ui_screen_template(current_screen_id);
	return Array(def_tpl.get("elements", Array())).duplicate(true);
}

Dictionary ClockGearsUiDesignEditor::get_effective_element(const String &p_elem_id) const {
	String v_pid;
	int v_idx = -1;
	if (s_parse_virtual_slot_id(p_elem_id, v_pid, v_idx)) {
		Dictionary parent_el = get_effective_element(v_pid);
		if (parent_el.is_empty()) {
			return Dictionary();
		}
		Array vslots = s_collect_virtual_slot_dicts(ClockGearsDataManager::get_singleton(), current_screen_id, parent_el);
		if (v_idx >= 0 && v_idx < vslots.size() && vslots[v_idx].get_type() == Variant::DICTIONARY) {
			return Dictionary(vslots[v_idx]).duplicate(true);
		}
		return Dictionary();
	}
	String target = p_elem_id.strip_edges().to_lower();
	if (target.is_empty()) {
		return Dictionary();
	}
	Array elems = get_effective_screen_elements();
	for (int i = 0; i < elems.size(); i++) {
		if (elems[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary el = elems[i];
		if (String(el.get("id", "")).to_lower() == target || String(el.get("name", "")).strip_edges().to_lower() == target) {
			return el.duplicate(true);
		}
	}
	return Dictionary();
}

bool ClockGearsUiDesignEditor::ensure_screen_customizable_for_edit(bool p_record_undo) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	bool has_cust = dm->has_custom_ui_design(current_screen_id);
	if (current_screen_id == "hud") {
		if (!dm->get_hud_widgets().is_empty()) {
			return false;
		}
	} else if (has_cust || !dm->get_ui_screen_elements(current_screen_id).is_empty()) {
		return false;
	}
	Dictionary tpl = dm->get_default_ui_screen_template(current_screen_id);
	tpl["custom_enabled"] = true;
	dm->set_ui_screen_design(current_screen_id, tpl, p_record_undo);
	return true;
}

void ClockGearsUiDesignEditor::commit_canvas_drag_undo(const Dictionary &p_initial_snapshot) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !p_initial_snapshot.is_empty()) {
		dm->push_ui_design_undo_state(current_screen_id, p_initial_snapshot);
	}
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::set_canvas_zoom_mode(int p_mode) {
	canvas_zoom_mode = CLAMP(p_mode, 0, 4);
	canvas_zoom = (canvas_zoom_mode == 2) ? 2 : ((canvas_zoom_mode == 4) ? 3 : 1);
	for (int i = 0; i < 5; i++) {
		if (zoom_btns[i]) {
			zoom_btns[i]->set_pressed(i == canvas_zoom_mode);
		}
	}
	ScreenZoomState st = per_screen_zoom_state.has(current_screen_id) ? per_screen_zoom_state[current_screen_id] : ScreenZoomState();
	st.zoom_mode = canvas_zoom_mode;
	if (canvas_scroll) {
		st.scroll_pos = Vector2i(canvas_scroll->get_h_scroll(), canvas_scroll->get_v_scroll());
	}
	per_screen_zoom_state[current_screen_id] = st;
	if (canvas_zoom_mode == 0) {
		fit_recalc_dirty = true;
	}
	_apply_canvas_zoom_layout(canvas_zoom_mode == 0);
	_refresh_canvas_stage();
}

void ClockGearsUiDesignEditor::set_canvas_zoom(int p_zoom) {
	if (p_zoom <= 0) {
		set_canvas_zoom_mode(0);
	} else if (p_zoom == 1) {
		set_canvas_zoom_mode(1);
	} else if (p_zoom == 2) {
		set_canvas_zoom_mode(2);
	} else {
		set_canvas_zoom_mode(4);
	}
}

String ClockGearsUiDesignEditor::add_element_at(const String &p_type, const Vector2i &p_canvas_pos) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	Dictionary el = dm->make_default_ui_element(p_type, current_screen_id);
	if (p_type != "frame" && p_type != "folder") {
		Array cur_els = dm->get_ui_screen_elements(current_screen_id);
		int top_l = 1;
		for (int i = 0; i < cur_els.size(); i++) {
			if (cur_els[i].get_type() == Variant::DICTIONARY) {
				top_l = MAX(top_l, int(Dictionary(cur_els[i]).get("layer_order", 0)));
			}
		}
		el["layer_order"] = MIN(1000, top_l + 1);
	}
	el["anchor"] = "top_left";
	int w = int(el.get("width", 120));
	int h = int(el.get("height", 36));
	if (p_canvas_pos.x >= 0 && p_canvas_pos.y >= 0) {
		int cx = CLAMP(p_canvas_pos.x - w / 2, 0, BASE_W - w);
		int cy = CLAMP(p_canvas_pos.y - h / 2, 0, BASE_H - h);
		el["offset_x"] = cx;
		el["offset_y"] = cy;
	} else if (p_type == "radial") {
		el["offset_x"] = (BASE_W - w) / 2;
		el["offset_y"] = (BASE_H - h) / 2;
	} else {
		Array existing = dm->get_ui_screen_elements(current_screen_id);
		el["offset_x"] = CLAMP(36 + existing.size() * 14, 0, BASE_W - w);
		el["offset_y"] = CLAMP(36 + existing.size() * 14, 0, BASE_H - h);
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	String new_id = dm->add_ui_element(current_screen_id, el, prev_custom);
	if (!new_id.is_empty()) {
		if (!prev_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		selected_elem_id = new_id;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(new_id);
		_notify_data_mutated(true);
	}
	return new_id;
}

void ClockGearsUiDesignEditor::update_multi_dragged_elements_fast(const Array &p_pairs) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || p_pairs.is_empty()) {
		return;
	}
	for (int i = 0; i < p_pairs.size(); i++) {
		if (p_pairs[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary pr = p_pairs[i];
		String eid = String(pr.get("id", ""));
		Dictionary el = pr.get("elem", Dictionary());
		if (eid.is_empty() || el.is_empty()) continue;
		dm->update_ui_element(current_screen_id, eid, el, false);
		if (design_layer && design_layer->is_visible()) {
			design_layer->update_element_geometry_fast(eid, el);
		}
		if (eid == selected_elem_id) {
			bool prev_upd = updating_ui;
			updating_ui = true;
			if (prop_ox_spin) prop_ox_spin->set_value(int(el.get("offset_x", 0)));
			if (prop_oy_spin) prop_oy_spin->set_value(int(el.get("offset_y", 0)));
			updating_ui = prev_upd;
		}
	}
	if (design_layer && !design_layer->is_visible()) {
		_refresh_background_and_default_preview(true);
		design_layer->apply_screen_design(current_screen_id, dm->get_ui_screen_design(current_screen_id));
		design_layer->show();
	}
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
}

void ClockGearsUiDesignEditor::set_grid_step(int p_step) {
	int clamped = (p_step <= 10) ? 8 : ((p_step >= 24) ? 32 : 16);
	if (canvas_overlay) {
		canvas_overlay->set_grid_step(clamped);
	}
	if (grid_step_opt) {
		for (int i = 0; i < grid_step_opt->get_item_count(); i++) {
			if (int(grid_step_opt->get_item_metadata(i)) == clamped) {
				grid_step_opt->select(i);
				break;
			}
		}
	}
}

int ClockGearsUiDesignEditor::get_grid_step() const {
	return canvas_overlay ? canvas_overlay->get_grid_step() : 16;
}

void ClockGearsUiDesignEditor::_on_grid_step_selected(int p_idx) {
	if (!grid_step_opt || p_idx < 0) {
		return;
	}
	set_grid_step(int(grid_step_opt->get_item_metadata(p_idx)));
}

bool ClockGearsUiDesignEditor::is_element_selected(const String &p_elem_id) const {
	if (p_elem_id.is_empty()) {
		return false;
	}
	if (p_elem_id == selected_elem_id) {
		return true;
	}
	return selected_elem_ids.has(p_elem_id);
}

bool ClockGearsUiDesignEditor::select_elements(const PackedStringArray &p_ids, const String &p_primary_id) {
	PackedStringArray next;
	for (int i = 0; i < p_ids.size(); i++) {
		Dictionary el = get_effective_element(p_ids[i]);
		if (!el.is_empty()) {
			String eid = String(el.get("id", p_ids[i]));
			if (!next.has(eid)) {
				next.push_back(eid);
			}
		}
	}
	selected_elem_ids = next;
	if (next.is_empty()) {
		selected_elem_id = "";
	} else if (!p_primary_id.is_empty() && next.has(p_primary_id)) {
		selected_elem_id = p_primary_id;
	} else {
		selected_elem_id = next[next.size() - 1];
	}
	_trace_log("element_multi_select", "screen=" + current_screen_id + " count=" + itos(selected_elem_ids.size()));
	_sync_selection_from_layers_tree();
	_refresh_properties_panel();
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
	return true;
}

bool ClockGearsUiDesignEditor::toggle_element_in_selection(const String &p_elem_id) {
	Dictionary el = get_effective_element(p_elem_id);
	if (el.is_empty()) {
		return false;
	}
	String eid = String(el.get("id", p_elem_id));
	PackedStringArray next = selected_elem_ids;
	if (!selected_elem_id.is_empty() && !next.has(selected_elem_id)) {
		next.push_back(selected_elem_id);
	}
	int idx = next.find(eid);
	if (idx >= 0) {
		next.remove_at(idx);
	} else {
		next.push_back(eid);
	}
	String prim = (idx >= 0) ? (next.is_empty() ? String() : next[next.size() - 1]) : eid;
	return select_elements(next, prim);
}

Dictionary ClockGearsUiDesignEditor::get_canvas_draw_stats() const {
	Dictionary out;
	bool grid_en = canvas_overlay ? canvas_overlay->is_grid_overlay_enabled() : false;
	int step = canvas_overlay ? canvas_overlay->get_grid_step() : 16;
	int grid_lines = 0;
	if (grid_en && step >= 4) {
		grid_lines = ((BASE_W - 1) / step) + ((BASE_H - 1) / step);
	}
	out["grid_enabled"] = grid_en;
	out["grid_step"] = step;
	out["grid_line_count"] = grid_lines;
	out["hud_grid_visible"] = (canvas_hud_grid_bg && canvas_hud_grid_bg->is_visible());
	out["smart_guides_enabled"] = canvas_overlay ? canvas_overlay->is_smart_guides_enabled() : false;
	out["dragging"] = canvas_overlay ? canvas_overlay->is_dragging_canvas() : false;
	out["active_guide_lines"] = (canvas_overlay && canvas_overlay->is_dragging_canvas() && canvas_overlay->is_smart_guides_enabled()) ? canvas_overlay->get_active_guide_line_count() : 0;
	out["selected_count"] = selected_elem_ids.size();
	return out;
}

bool ClockGearsUiDesignEditor::select_element(const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	if (p_elem_id.is_empty()) {
		selected_elem_id = "";
		selected_elem_ids.clear();
		_trace_log("element_select", "screen=" + current_screen_id + " elem=<none>");
		_sync_selection_from_layers_tree();
		_refresh_properties_panel();
		if (canvas_overlay) canvas_overlay->queue_redraw();
		return true;
	}
	Dictionary el = get_effective_element(p_elem_id);
	if (el.is_empty()) {
		return false;
	}
	selected_elem_id = String(el.get("id", p_elem_id));
	selected_elem_ids.clear();
	selected_elem_ids.push_back(selected_elem_id);
	_trace_log("element_select", "screen=" + current_screen_id + " elem=" + selected_elem_id);
	_sync_selection_from_layers_tree();
	_refresh_properties_panel();
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
	return true;
}

void ClockGearsUiDesignEditor::_on_delete_parent_keep_confirmed() {
	delete_selected_element_with_mode(0);
}

void ClockGearsUiDesignEditor::_on_delete_parent_custom_action(const String &p_action) {
	if (p_action == "delete_children") {
		if (delete_parent_dialog) {
			delete_parent_dialog->hide();
		}
		delete_selected_element_with_mode(1);
	}
}

bool ClockGearsUiDesignEditor::delete_selected_element() {
	return delete_selected_element_with_mode(-1);
}

bool ClockGearsUiDesignEditor::delete_selected_element_with_mode(int p_mode) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || (selected_elem_id.is_empty() && selected_elem_ids.is_empty())) {
		return false;
	}
	PackedStringArray targets = selected_elem_ids;
	if (!selected_elem_id.is_empty() && !targets.has(selected_elem_id)) {
		targets.push_back(selected_elem_id);
	}
	Array eff_elems = get_effective_screen_elements();
	PackedStringArray unlocked;
	for (int i = 0; i < targets.size(); i++) {
		Dictionary el = get_effective_element(targets[i]);
		String cid = String(el.get("id", targets[i]));
		if (!el.is_empty() && !ClockGearsUiDesignLayer::is_element_effectively_locked(cid, eff_elems)) {
			unlocked.push_back(cid);
		}
	}
	if (unlocked.is_empty()) {
		return false;
	}
	if (p_mode < 0) {
		bool has_children = false;
		HashSet<String> target_low;
		for (int i = 0; i < unlocked.size(); i++) {
			target_low.insert(unlocked[i].to_lower());
		}
		for (int i = 0; i < eff_elems.size(); i++) {
			if (eff_elems[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary e = eff_elems[i];
			String pid = String(e.get("parent_id", "")).strip_edges().to_lower();
			String eid = String(e.get("id", "")).to_lower();
			if (!pid.is_empty() && target_low.has(pid) && !target_low.has(eid)) {
				has_children = true;
				break;
			}
		}
		if (has_children && delete_parent_dialog) {
			pending_delete_parent_ids = unlocked;
			delete_parent_dialog->popup_centered();
			return false;
		}
		p_mode = 0;
	}
	pending_delete_parent_ids.clear();
	bool delete_children = (p_mode == 1);
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	bool any_del = false;
	for (int i = 0; i < unlocked.size(); i++) {
		if (dm->delete_ui_element_with_mode(current_screen_id, unlocked[i], delete_children, false)) {
			any_del = true;
		}
	}
	if (!any_del) {
		return false;
	}
	dm->push_ui_design_undo_state(current_screen_id, snap);
	_trace_log("delete_elem", "screen=" + current_screen_id + " mode=" + itos(p_mode) + " count=" + itos(unlocked.size()));
	selected_elem_id = "";
	selected_elem_ids.clear();
	Array rem = get_effective_screen_elements();
	if (!rem.is_empty() && rem[0].get_type() == Variant::DICTIONARY) {
		selected_elem_id = String(Dictionary(rem[0]).get("id", ""));
		if (!selected_elem_id.is_empty()) {
			selected_elem_ids.push_back(selected_elem_id);
		}
	}
	_notify_data_mutated(true);
	return true;
}

bool ClockGearsUiDesignEditor::toggle_element_visibility(const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	Dictionary el = get_effective_element(p_elem_id);
	if (el.is_empty()) {
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	bool next_vis = !bool(el.get("visible", true));
	el["visible"] = next_vis;
	String v_pid;
	int v_idx = -1;
	bool ok = false;
	if (s_parse_virtual_slot_id(p_elem_id, v_pid, v_idx)) {
		Dictionary parent_el = dm->get_ui_element(current_screen_id, v_pid);
		if (parent_el.is_empty()) parent_el = get_effective_element(v_pid);
		if (s_write_virtual_slot_into_parent(current_screen_id, parent_el, v_idx, el)) {
			ok = dm->update_ui_element(current_screen_id, v_pid, parent_el, was_custom);
		}
	} else {
		ok = dm->update_ui_element(current_screen_id, p_elem_id, el, was_custom);
	}
	if (ok) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_trace_log("layer_visibility", "screen=" + current_screen_id + " elem=" + p_elem_id + " vis=" + (next_vis ? "true" : "false"));
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::toggle_element_locked(const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	Dictionary el = get_effective_element(p_elem_id);
	if (el.is_empty()) {
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	bool next_lock = !bool(el.get("locked", false));
	el["locked"] = next_lock;
	if (dm->update_ui_element(current_screen_id, p_elem_id, el, was_custom)) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_trace_log("layer_lock", "screen=" + current_screen_id + " elem=" + p_elem_id + " locked=" + (next_lock ? "true" : "false"));
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::rename_element_inline(const String &p_elem_id, const String &p_new_name) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String clean = p_new_name.strip_edges();
	if (!dm || clean.is_empty()) {
		_refresh_layers_list();
		return false;
	}
	Dictionary el = get_effective_element(p_elem_id);
	if (el.is_empty() || String(el.get("name", "")) == clean) {
		_refresh_layers_list();
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	el["name"] = clean;
	el["text"] = clean;
	String v_pid;
	int v_idx = -1;
	bool ok = false;
	if (s_parse_virtual_slot_id(p_elem_id, v_pid, v_idx)) {
		Dictionary parent_el = dm->get_ui_element(current_screen_id, v_pid);
		if (parent_el.is_empty()) parent_el = get_effective_element(v_pid);
		if (s_write_virtual_slot_into_parent(current_screen_id, parent_el, v_idx, el)) {
			ok = dm->update_ui_element(current_screen_id, v_pid, parent_el, was_custom);
		}
	} else {
		ok = dm->update_ui_element(current_screen_id, p_elem_id, el, was_custom);
	}
	if (ok) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_trace_log("layer_rename", "screen=" + current_screen_id + " elem=" + p_elem_id + " name=" + clean);
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::begin_selected_tree_rename() {
	if (selected_elem_id.is_empty() || !design_card_rows.has(selected_elem_id)) {
		return false;
	}
	ClockGearsDesignCardRow *row = design_card_rows[selected_elem_id];
	if (!row) {
		return false;
	}
	_ensure_design_card_visible(selected_elem_id);
	row->begin_inline_rename();
	return true;
}

String ClockGearsUiDesignEditor::create_folder_element(const String &p_name, const String &p_parent_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	Dictionary f = dm->make_default_ui_element("folder", current_screen_id);
	if (!p_name.strip_edges().is_empty()) {
		f["name"] = p_name.strip_edges();
	}
	if (!p_parent_id.strip_edges().is_empty()) {
		f["parent_id"] = p_parent_id.strip_edges();
	}
	String fid = dm->add_ui_element(current_screen_id, f, was_custom);
	if (!fid.is_empty()) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		selected_elem_id = fid;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(fid);
		_trace_log("folder_create", "screen=" + current_screen_id + " folder=" + fid);
		_notify_data_mutated(true);
	}
	return fid;
}

String ClockGearsUiDesignEditor::group_selected_into_folder(const String &p_name) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || (selected_elem_id.is_empty() && selected_elem_ids.is_empty())) {
		return String();
	}
	PackedStringArray ids = selected_elem_ids;
	if (!selected_elem_id.is_empty() && !ids.has(selected_elem_id)) {
		ids.push_back(selected_elem_id);
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	String fid = dm->group_ui_elements_into_folder(current_screen_id, ids, p_name, was_custom);
	if (!fid.is_empty()) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		selected_elem_id = fid;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(fid);
		_trace_log("folder_group", "screen=" + current_screen_id + " folder=" + fid + " count=" + itos(ids.size()));
		_notify_data_mutated(true);
	}
	return fid;
}

bool ClockGearsUiDesignEditor::ungroup_selected_folder() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return false;
	}
	Dictionary sel = get_effective_element(selected_elem_id);
	if (sel.is_empty() || String(sel.get("type", "")) != "folder") {
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	if (dm->ungroup_ui_folder(current_screen_id, selected_elem_id, was_custom)) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_trace_log("folder_ungroup", "screen=" + current_screen_id + " folder=" + selected_elem_id);
		selected_elem_id = "";
		selected_elem_ids.clear();
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::can_drop_in_layer_tree(const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section) const {
	if (p_elem_ids.is_empty()) {
		return false;
	}
	Array elems = get_effective_screen_elements();
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	s_collect_editor_tree_sorted(elems, by_id, children, orig_idx);

	HashSet<String> dragging;
	for (int i = 0; i < p_elem_ids.size(); i++) {
		if (by_id.has(p_elem_ids[i])) {
			dragging.insert(p_elem_ids[i]);
		}
	}
	if (dragging.is_empty()) {
		return false;
	}
	if (p_target_id.is_empty() || p_drop_section == -100) {
		return true;
	}
	if (!by_id.has(p_target_id)) {
		return false;
	}
	String check_ancestor = (p_drop_section == 0) ? p_target_id : String(by_id[p_target_id].get("parent_id", ""));
	if ((p_drop_section == 0 || p_drop_section == -1 || p_drop_section == 1) && dragging.has(p_target_id)) {
		return false;
	}
	HashSet<String> vis;
	while (!check_ancestor.is_empty() && by_id.has(check_ancestor) && !vis.has(check_ancestor)) {
		if (dragging.has(check_ancestor)) {
			return false;
		}
		vis.insert(check_ancestor);
		check_ancestor = String(by_id[check_ancestor].get("parent_id", ""));
	}
	return true;
}

bool ClockGearsUiDesignEditor::reorder_layer_tree(const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !can_drop_in_layer_tree(p_elem_ids, p_target_id, p_drop_section)) {
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	if (dm->reorder_ui_element_tree(current_screen_id, p_elem_ids, p_target_id, p_drop_section, was_custom)) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_trace_log("layer_tree_reorder", "screen=" + current_screen_id + " target=" + p_target_id + " sec=" + itos(p_drop_section));
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::set_element_collapsed(const String &p_elem_id, bool p_collapsed) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || p_elem_id.is_empty()) {
		return false;
	}
	Dictionary el = dm->get_ui_element(current_screen_id, p_elem_id);
	if (el.is_empty()) {
		el = get_effective_element(p_elem_id);
		if (el.is_empty()) {
			return false;
		}
		ensure_screen_customizable_for_edit(false);
	}
	el["collapsed"] = p_collapsed;
	return dm->update_ui_element(current_screen_id, p_elem_id, el, false);
}

Array ClockGearsUiDesignEditor::get_layer_tree_info() const {
	Array out;
	Array elems = get_effective_screen_elements();
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	s_collect_editor_tree_sorted(elems, by_id, children, orig_idx);

	struct Walk {
		static void run(const String &parent_id, int depth, const HashMap<String, Dictionary> &by_id,
				const HashMap<String, Vector<String>> &children, const ClockGearsUiDesignEditor *ed, Array &out) {
			if (!children.has(parent_id)) return;
			const Vector<String> &kids = children[parent_id];
			for (int i = 0; i < kids.size(); i++) {
				const String &eid = kids[i];
				const Dictionary &e = by_id[eid];
				Dictionary row;
				row["id"] = eid;
				row["name"] = String(e.get("name", eid));
				row["parent_id"] = String(e.get("parent_id", ""));
				row["type"] = String(e.get("type", ""));
				row["depth"] = depth;
				row["layer_order"] = int(e.get("layer_order", 0));
				row["visible"] = bool(e.get("visible", true));
				row["locked"] = bool(e.get("locked", false));
				row["collapsed"] = bool(e.get("collapsed", false));
				row["selected"] = ed->is_element_selected(eid);
				row["editor_selected"] = ed->is_element_selected(eid);
				out.push_back(row);
				run(eid, depth + 1, by_id, children, ed, out);
			}
		}
	};
	Walk::run(String(""), 0, by_id, children, this, out);
	return out;
}

Array ClockGearsUiDesignEditor::get_design_cards_info() const {
	Array out;
	for (int i = 0; i < design_cards_order.size(); i++) {
		const String &eid = design_cards_order[i];
		ClockGearsDesignCardRow *row_ctl = design_card_rows.has(eid) ? design_card_rows[eid] : nullptr;
		Dictionary e = get_effective_element(eid);
		Dictionary info;
		info["id"] = eid;
		info["name"] = String(e.get("name", eid));
		info["parent_id"] = String(e.get("parent_id", ""));
		info["type"] = String(e.get("type", ""));
		info["depth"] = row_ctl ? row_ctl->get_depth() : 0;
		info["layer_order"] = int(e.get("layer_order", 0));
		info["visible"] = bool(e.get("visible", true));
		info["locked"] = bool(e.get("locked", false));
		info["collapsed"] = bool(e.get("collapsed", false));
		info["selected"] = is_element_selected(eid);
		info["card_control"] = row_ctl;
		out.push_back(info);
	}
	return out;
}

Control *ClockGearsUiDesignEditor::get_design_card_control(const String &p_elem_id) const {
	if (design_card_rows.has(p_elem_id)) {
		return design_card_rows[p_elem_id];
	}
	String low = p_elem_id.strip_edges().to_lower();
	for (const KeyValue<String, ClockGearsDesignCardRow *> &kv : design_card_rows) {
		if (kv.key.to_lower() == low) {
			return kv.value;
		}
	}
	return nullptr;
}

String ClockGearsUiDesignEditor::add_child_element_of_type(const String &p_parent_id, const String &p_type) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	Dictionary parent_el = get_effective_element(p_parent_id);
	if (parent_el.is_empty()) {
		return String();
	}
	String canon_pid = String(parent_el.get("id", p_parent_id));
	Dictionary el = dm->make_default_ui_element(p_type, current_screen_id);
	if (p_type != "frame" && p_type != "folder") {
		Array cur_els = get_effective_screen_elements();
		int top_l = 1;
		for (int i = 0; i < cur_els.size(); i++) {
			if (cur_els[i].get_type() == Variant::DICTIONARY) {
				top_l = MAX(top_l, int(Dictionary(cur_els[i]).get("layer_order", 0)));
			}
		}
		el["layer_order"] = MIN(1000, top_l + 1);
	}
	el["parent_id"] = canon_pid;
	el["anchor"] = "top_left";
	Array existing = get_effective_screen_elements();
	int child_cnt = 0;
	for (int i = 0; i < existing.size(); i++) {
		if (existing[i].get_type() == Variant::DICTIONARY &&
				String(Dictionary(existing[i]).get("parent_id", "")).to_lower() == canon_pid.to_lower()) {
			child_cnt++;
		}
	}
	el["offset_x"] = 12 + child_cnt * 10;
	el["offset_y"] = 12 + child_cnt * 10;

	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	if (bool(parent_el.get("collapsed", false))) {
		parent_el["collapsed"] = false;
		dm->update_ui_element(current_screen_id, canon_pid, parent_el, false);
	}
	String new_id = dm->add_ui_element(current_screen_id, el, was_custom);
	if (!new_id.is_empty()) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		selected_elem_id = new_id;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(new_id);
		_trace_log("card_add_child", "screen=" + current_screen_id + " parent=" + canon_pid + " child=" + new_id + " type=" + p_type);
		_notify_data_mutated(true);
		_ensure_design_card_visible(new_id);
	}
	return new_id;
}

bool ClockGearsUiDesignEditor::reparent_element_to(const String &p_elem_id, const String &p_new_parent_id) {
	if (p_elem_id.is_empty()) {
		return false;
	}
	PackedStringArray ids;
	ids.push_back(p_elem_id);
	if (p_new_parent_id.strip_edges().is_empty()) {
		return unparent_element(p_elem_id);
	}
	if (!reorder_layer_tree(ids, p_new_parent_id, 0)) {
		return false;
	}
	set_element_collapsed(p_new_parent_id, false);
	_refresh_layers_list();
	_ensure_design_card_visible(p_elem_id);
	return true;
}

bool ClockGearsUiDesignEditor::unparent_element(const String &p_elem_id) {
	if (p_elem_id.is_empty()) {
		return false;
	}
	PackedStringArray ids;
	ids.push_back(p_elem_id);
	bool ok = reorder_layer_tree(ids, String(""), -100);
	if (ok) {
		_ensure_design_card_visible(p_elem_id);
	}
	return ok;
}

bool ClockGearsUiDesignEditor::drop_card_on_target(const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section) {
	bool ok = reorder_layer_tree(p_elem_ids, p_target_id, p_drop_section);
	if (ok && p_drop_section == 0 && !p_target_id.is_empty()) {
		set_element_collapsed(p_target_id, false);
		_refresh_layers_list();
	}
	if (ok && !p_elem_ids.is_empty()) {
		_ensure_design_card_visible(p_elem_ids[0]);
	}
	return ok;
}

bool ClockGearsUiDesignEditor::simulate_card_drop_at(const String &p_target_id, const Vector2 &p_local_pos, const PackedStringArray &p_drag_ids) {
	if (!design_card_rows.has(p_target_id)) {
		return false;
	}
	ClockGearsDesignCardRow *row = design_card_rows[p_target_id];
	if (!row) {
		return false;
	}
	Dictionary payload;
	payload["type"] = String("clockgears_layer_tree");
	payload["ids"] = p_drag_ids;
	if (!row->can_drop_data(p_local_pos, payload)) {
		return false;
	}
	row->drop_data(p_local_pos, payload);
	return true;
}

void ClockGearsUiDesignEditor::_ensure_design_card_visible(const String &p_elem_id) {
	if (!design_cards_scroll || p_elem_id.is_empty() || !design_card_rows.has(p_elem_id)) {
		return;
	}
	ClockGearsDesignCardRow *row = design_card_rows[p_elem_id];
	if (row && is_inside_tree() && design_cards_scroll->is_inside_tree() && row->is_inside_tree()) {
		design_cards_scroll->ensure_control_visible(row);
	}
}

void ClockGearsUiDesignEditor::_sync_selection_from_layers_tree() {
	if (design_card_rows.is_empty()) {
		_refresh_layers_list();
		return;
	}
	// Si el elemento seleccionado está oculto dentro de una carpeta/padre contraído, expandir ancestros.
	if (!selected_elem_id.is_empty() && !design_card_rows.has(selected_elem_id)) {
		Array elems = get_effective_screen_elements();
		HashMap<String, Dictionary> by_id;
		for (int i = 0; i < elems.size(); i++) {
			if (elems[i].get_type() == Variant::DICTIONARY) {
				Dictionary e = elems[i];
				by_id[String(e.get("id", ""))] = e;
			}
		}
		String cur = selected_elem_id;
		bool uncollapsed_any = false;
		HashSet<String> vis;
		while (!cur.is_empty() && by_id.has(cur) && !vis.has(cur)) {
			vis.insert(cur);
			String pid = String(by_id[cur].get("parent_id", ""));
			if (!pid.is_empty() && by_id.has(pid) && bool(by_id[pid].get("collapsed", false))) {
				set_element_collapsed(pid, false);
				uncollapsed_any = true;
			}
			cur = pid;
		}
		if (uncollapsed_any) {
			_refresh_layers_list();
		}
	}
	for (const KeyValue<String, ClockGearsDesignCardRow *> &kv : design_card_rows) {
		if (kv.value) {
			kv.value->update_visual_state(is_element_selected(kv.key));
		}
	}
	if (!selected_elem_id.is_empty()) {
		_ensure_design_card_visible(selected_elem_id);
	}

	bool has_sel = !selected_elem_id.is_empty();
	bool sel_is_folder = false;
	if (has_sel) {
		Dictionary el = get_effective_element(selected_elem_id);
		sel_is_folder = (String(el.get("type", "")) == "folder");
	}
	if (layer_group_btn) layer_group_btn->set_disabled(!has_sel);
	if (layer_ungroup_btn) layer_ungroup_btn->set_disabled(!sel_is_folder);
	if (layer_up_btn) layer_up_btn->set_disabled(!has_sel);
	if (layer_down_btn) layer_down_btn->set_disabled(!has_sel);
	if (layer_del_btn) layer_del_btn->set_disabled(!has_sel);
}

void ClockGearsUiDesignEditor::_on_layers_tree_multi_selected(Object *p_item, int p_col, bool p_selected) {
	if (updating_ui || !layers_tree) {
		return;
	}
	PackedStringArray next;
	String primary;
	TreeItem *clicked = Object::cast_to<TreeItem>(p_item);
	for (TreeItem *ti = layers_tree->get_next_selected(nullptr); ti != nullptr; ti = layers_tree->get_next_selected(ti)) {
		String eid = String(ti->get_metadata(0));
		if (!eid.is_empty() && !next.has(eid)) {
			next.push_back(eid);
		}
	}
	if (clicked && p_selected) {
		String cid = String(clicked->get_metadata(0));
		if (!cid.is_empty() && next.has(cid)) {
			primary = cid;
		}
	}
	if (primary.is_empty() && !next.is_empty()) {
		primary = next[next.size() - 1];
	}
	selected_elem_ids = next;
	selected_elem_id = primary;
	_trace_log("layer_tree_select", "screen=" + current_screen_id + " count=" + itos(selected_elem_ids.size()) + " prim=" + selected_elem_id);
	_refresh_properties_panel();
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
	bool has_sel = !selected_elem_id.is_empty();
	bool sel_is_folder = false;
	if (has_sel) {
		Dictionary el = get_effective_element(selected_elem_id);
		sel_is_folder = (String(el.get("type", "")) == "folder");
	}
	if (layer_group_btn) layer_group_btn->set_disabled(!has_sel);
	if (layer_ungroup_btn) layer_ungroup_btn->set_disabled(!sel_is_folder);
	if (layer_up_btn) layer_up_btn->set_disabled(!has_sel);
	if (layer_down_btn) layer_down_btn->set_disabled(!has_sel);
	if (layer_del_btn) layer_del_btn->set_disabled(!has_sel);
}

void ClockGearsUiDesignEditor::_on_layers_tree_button_clicked(Object *p_item, int p_column, int p_id, int p_mouse_button_index) {
	TreeItem *ti = Object::cast_to<TreeItem>(p_item);
	if (!ti) {
		return;
	}
	String eid = String(ti->get_metadata(0));
	if (eid.is_empty()) {
		return;
	}
	if (p_id == 0) {
		toggle_element_visibility(eid);
	} else if (p_id == 1) {
		toggle_element_locked(eid);
	}
}

void ClockGearsUiDesignEditor::_on_layers_tree_item_edited() {
	if (updating_ui || !layers_tree) {
		return;
	}
	TreeItem *edited = layers_tree->get_edited();
	if (!edited) {
		return;
	}
	String eid = String(edited->get_metadata(0));
	String new_name = edited->get_text(0);
	edited->set_editable(0, false);
	if (!eid.is_empty()) {
		rename_element_inline(eid, new_name);
	}
}

void ClockGearsUiDesignEditor::_on_layers_tree_item_collapsed(Object *p_item) {
	if (updating_ui) {
		return;
	}
	TreeItem *ti = Object::cast_to<TreeItem>(p_item);
	if (!ti) {
		return;
	}
	String eid = String(ti->get_metadata(0));
	if (!eid.is_empty()) {
		set_element_collapsed(eid, ti->is_collapsed());
	}
}

void ClockGearsUiDesignEditor::_on_layers_tree_item_mouse_selected(const Vector2 &p_pos, int p_mouse_button_index) {
	if (p_mouse_button_index == int(MouseButton::RIGHT) && layers_tree) {
		show_layer_context_menu(layers_tree->get_screen_transform().xform(p_pos));
	}
}

void ClockGearsUiDesignEditor::show_layer_context_menu(const Vector2 &p_screen_pos) {
	if (!layer_context_menu) {
		return;
	}
	bool has_sel = !selected_elem_id.is_empty();
	bool is_folder = false;
	bool has_parent = false;
	if (has_sel) {
		Dictionary el = get_effective_element(selected_elem_id);
		is_folder = (String(el.get("type", "")) == "folder");
		has_parent = !String(el.get("parent_id", "")).strip_edges().is_empty();
	}
	if (reparent_submenu) {
		reparent_submenu->clear();
		reparent_candidate_ids.clear();
		reparent_submenu->add_item(String::utf8("— Nivel raíz (sin padre) —"), 0);
		reparent_candidate_ids.push_back(String(""));
		if (has_sel) {
			Array info = get_layer_tree_info();
			PackedStringArray drag_arr;
			drag_arr.push_back(selected_elem_id);
			int idx = 1;
			for (int i = 0; i < info.size(); i++) {
				Dictionary r = info[i];
				String cid = String(r.get("id", ""));
				if (cid.is_empty() || cid == selected_elem_id) {
					continue;
				}
				if (!can_drop_in_layer_tree(drag_arr, cid, 0)) {
					continue;
				}
				int d = int(r.get("depth", 0));
				String prefix;
				for (int k = 0; k < d; k++) {
					prefix += "  ";
				}
				reparent_submenu->add_item(prefix + String(r.get("name", cid)), idx);
				reparent_candidate_ids.push_back(cid);
				idx++;
			}
		}
	}
	auto set_dis = [&](int p_id, bool p_dis) {
		int idx = layer_context_menu->get_item_index(p_id);
		if (idx >= 0) {
			layer_context_menu->set_item_disabled(idx, p_dis);
		}
	};
	set_dis(11, !has_sel);
	set_dis(20, !has_sel);
	set_dis(22, !has_sel);
	set_dis(21, !has_sel || !has_parent);
	set_dis(1, !has_sel);
	set_dis(2, !has_sel);
	set_dis(3, !is_folder);
	set_dis(4, !has_sel);
	set_dis(5, !has_sel);
	set_dis(6, !has_sel);
	set_dis(7, !has_sel);
	set_dis(8, !has_sel);
	layer_context_menu->set_position(Vector2i(p_screen_pos));
	layer_context_menu->popup();
}

void ClockGearsUiDesignEditor::_on_add_child_submenu_id(int p_id) {
	if (!add_child_submenu || selected_elem_id.is_empty()) {
		return;
	}
	int idx = add_child_submenu->get_item_index(p_id);
	if (idx < 0) {
		return;
	}
	String t = String(add_child_submenu->get_item_metadata(idx));
	if (!t.is_empty()) {
		(void)add_child_element_of_type(selected_elem_id, t);
	}
}

void ClockGearsUiDesignEditor::_on_reparent_submenu_id(int p_id) {
	if (selected_elem_id.is_empty() || p_id < 0 || p_id >= reparent_candidate_ids.size()) {
		return;
	}
	reparent_element_to(selected_elem_id, reparent_candidate_ids[p_id]);
}

void ClockGearsUiDesignEditor::_on_layer_context_menu_id(int p_id) {
	if (p_id == 11) {
		open_floating_element_window(selected_elem_id);
	} else if (p_id == 21) {
		unparent_element(selected_elem_id);
	} else if (p_id == 1) duplicate_selected_element();
	else if (p_id == 2) (void)group_selected_into_folder();
	else if (p_id == 3) ungroup_selected_folder();
	else if (p_id == 4) begin_selected_tree_rename();
	else if (p_id == 5) move_selected_layer(1);
	else if (p_id == 6) move_selected_layer(-1);
	else if (p_id == 7) {
		save_tpl_is_component = true;
		if (save_tpl_name_edit) save_tpl_name_edit->set_text("Mi componente");
		if (save_tpl_name_dialog) save_tpl_name_dialog->popup_centered();
	} else if (p_id == 8) {
		delete_selected_element();
	} else if (p_id == 9) {
		(void)group_selected_into_folder();
	} else if (p_id == 10) {
		ungroup_selected_folder();
	}
}

void ClockGearsUiDesignEditor::_on_layer_add_folder_pressed() {
	begin_inline_folder_create();
}

void ClockGearsUiDesignEditor::begin_inline_folder_create() {
	if (!folder_prompt_box || !folder_name_edit) {
		(void)confirm_inline_folder_create("Carpeta");
		return;
	}
	folder_prompt_box->show();
	folder_name_edit->set_text("Carpeta");
	if (is_inside_tree()) {
		folder_name_edit->grab_focus();
		folder_name_edit->select_all();
	}
}

String ClockGearsUiDesignEditor::confirm_inline_folder_create(const String &p_name_override) {
	String raw = p_name_override.strip_edges();
	if (raw.is_empty() && folder_name_edit) {
		raw = folder_name_edit->get_text().strip_edges();
	}
	if (raw.is_empty()) {
		raw = "Carpeta";
	}
	if (folder_prompt_box) {
		folder_prompt_box->hide();
	}
	if (!selected_elem_ids.is_empty() || !selected_elem_id.is_empty()) {
		return group_selected_into_folder(raw);
	}
	return create_folder_element(raw, "");
}

void ClockGearsUiDesignEditor::cancel_inline_folder_create() {
	if (folder_prompt_box) {
		folder_prompt_box->hide();
	}
}

void ClockGearsUiDesignEditor::_on_folder_prompt_confirm(const String &p_text) {
	(void)confirm_inline_folder_create(p_text);
}

void ClockGearsUiDesignEditor::_on_folder_prompt_cancel() {
	cancel_inline_folder_create();
}

void ClockGearsUiDesignEditor::_on_folder_name_gui_input(const Ref<InputEvent> &p_ev) {
	Ref<InputEventKey> k = p_ev;
	if (k.is_valid() && k->is_pressed() && !k->is_echo() && k->get_keycode() == Key::ESCAPE) {
		cancel_inline_folder_create();
		if (folder_name_edit) {
			folder_name_edit->accept_event();
		}
	}
}

void ClockGearsUiDesignEditor::_on_layer_group_pressed() {
	(void)group_selected_into_folder();
}

void ClockGearsUiDesignEditor::_on_layer_ungroup_pressed() {
	ungroup_selected_folder();
}

bool ClockGearsUiDesignEditor::move_selected_layer(int p_delta) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	if (dm->move_ui_element_tree_step(current_screen_id, selected_elem_id, p_delta, was_custom)) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_trace_log("layer_move_step", "screen=" + current_screen_id + " elem=" + selected_elem_id + " delta=" + itos(p_delta));
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
	_trace_log("undo", "screen=" + current_screen_id);
	_notify_data_mutated(true);
	return true;
}

bool ClockGearsUiDesignEditor::apply_redo() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !dm->redo_ui_design(current_screen_id)) {
		return false;
	}
	_trace_log("redo", "screen=" + current_screen_id);
	_notify_data_mutated(true);
	return true;
}

void ClockGearsUiDesignEditor::request_reset_to_default(bool p_confirmed_directly) {
	_trace_log("reset_request", "screen=" + current_screen_id + " direct=" + (p_confirmed_directly ? "true" : "false"));
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
	Dictionary el = get_effective_element(p_elem_id);
	if (el.is_empty()) {
		return;
	}
	String et = String(el.get("type", "text"));
	if (et != "text" && et != "button" && et != "bar" && et != "variable") {
		return;
	}
	float scale = get_canvas_stage_scale();
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
	bool is_folder = false;
	if (has_sel) {
		Dictionary el = get_effective_element(selected_elem_id);
		is_folder = (String(el.get("type", "")) == "folder");
	}
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(11), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(1), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(2), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(3), !has_clip);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(4), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(5), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(6), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(7), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(8), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(9), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(10), !is_folder);
	canvas_context_menu->set_position(Vector2i(p_screen_pos));
	canvas_context_menu->popup();
}

void ClockGearsUiDesignEditor::_on_canvas_context_menu_id(int p_id) {
	if (p_id == 11) {
		open_floating_element_window(selected_elem_id);
	} else if (p_id == 1) {
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
	} else if (p_id == 9) {
		(void)group_selected_into_folder();
	} else if (p_id == 10) {
		ungroup_selected_folder();
	}
}

bool ClockGearsUiDesignEditor::align_selected_elements(const String &p_mode) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	if (p_mode == "distribute_v") {
		Array elems = get_effective_screen_elements();
		if (elems.size() < 2) {
			return false;
		}
		Dictionary prev_d = dm->get_ui_screen_design(current_screen_id);
		ensure_screen_customizable_for_edit(false);
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
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	int w = int(el.get("width", 120));
	int h = int(el.get("height", 36));
	el["anchor"] = "top_left";
	if (p_mode == "center_h" || p_mode == "center_both") {
		el["offset_x"] = (BASE_W - w) / 2;
	}
	if (p_mode == "center_v" || p_mode == "center_both") {
		el["offset_y"] = (BASE_H - h) / 2;
	}
	dm->update_ui_element(current_screen_id, selected_elem_id, el, was_custom);
	if (!was_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	_notify_data_mutated(true);
	return true;
}

bool ClockGearsUiDesignEditor::duplicate_selected_element() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return false;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	String nid = dm->duplicate_ui_element(current_screen_id, selected_elem_id, Vector2i(16, 16));
	if (!nid.is_empty()) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		selected_elem_id = nid;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(nid);
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

void ClockGearsUiDesignEditor::copy_selected_to_clipboard() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || (selected_elem_id.is_empty() && selected_elem_ids.is_empty())) {
		return;
	}
	PackedStringArray ids = selected_elem_ids;
	if (!selected_elem_id.is_empty() && !ids.has(selected_elem_id)) {
		ids.push_back(selected_elem_id);
	}
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
		selected_elem_ids = added;
		_notify_data_mutated(true);
	}
	return added;
}

Dictionary ClockGearsUiDesignEditor::_get_active_radial_layout(bool *r_is_elem) const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary sel = _get_selected_element_copy();
	if (!sel.is_empty() && String(sel.get("type", "")) == "radial") {
		if (r_is_elem) *r_is_elem = true;
		Dictionary lay = sel.get("layout", Dictionary());
		return ClockGearsDataManager::normalize_menu_layout(lay);
	}
	if (r_is_elem) *r_is_elem = false;
	if (!dm) return Dictionary();
	Dictionary d = dm->get_ui_screen_design(current_screen_id);
	return ClockGearsDataManager::normalize_menu_layout(d.get("layout", Dictionary()));
}

void ClockGearsUiDesignEditor::_commit_active_radial_layout(const Dictionary &p_lay, bool p_push_undo, bool p_refresh_inspector) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary norm_lay = ClockGearsDataManager::normalize_menu_layout(p_lay);
	Dictionary sel = _get_selected_element_copy();
	if (!sel.is_empty() && String(sel.get("type", "")) == "radial") {
		Dictionary snap = dm->get_ui_screen_design(current_screen_id);
		bool prev_custom = dm->has_custom_ui_design(current_screen_id);
		ensure_screen_customizable_for_edit(false);
		sel["layout"] = norm_lay;
		dm->update_ui_element(current_screen_id, selected_elem_id, sel, prev_custom && p_push_undo);
		if (p_push_undo && !prev_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		if (p_refresh_inspector) {
			_notify_data_mutated(true);
		} else {
			_refresh_canvas_stage();
			emit_signal("design_modified");
		}
		return;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
	d["layout"] = norm_lay;
	dm->set_ui_screen_design(current_screen_id, d, p_push_undo);
	if (p_refresh_inspector) {
		_notify_data_mutated(true);
	} else {
		_refresh_canvas_stage();
		emit_signal("design_modified");
	}
}

void ClockGearsUiDesignEditor::_refresh_radial_inspector() {
	if (!prop_radial_box) {
		return;
	}
	bool is_el = false;
	Dictionary lay = _get_active_radial_layout(&is_el);
	Array slots = lay.get("radial_slots", Array());
	bool prev_upd = updating_ui;
	updating_ui = true;
	if (prop_rad_count_slider) prop_rad_count_slider->set_value(MAX(1, slots.size()));
	if (prop_rad_count_lbl) prop_rad_count_lbl->set_text(itos(slots.size()));
	if (prop_rad_radius_slider) prop_rad_radius_slider->set_value(int(lay.get("radial_radius", 128)));
	if (prop_rad_start_slider) prop_rad_start_slider->set_value(float(lay.get("radial_start_angle_deg", -90.0)));
	float sweep = float(lay.get("radial_sweep_angle_deg", 360.0));
	if (prop_rad_full_chk) prop_rad_full_chk->set_pressed(sweep >= 359.5f);
	if (prop_rad_sweep_slider) {
		prop_rad_sweep_slider->set_value(sweep);
		prop_rad_sweep_slider->set_editable(sweep < 359.5f);
	}
	if (prop_rad_cw_chk) prop_rad_cw_chk->set_pressed(lay.has("radial_clockwise") ? bool(lay["radial_clockwise"]) : true);
	if (prop_rad_slot_size_slider) prop_rad_slot_size_slider->set_value(int(lay.get("radial_slot_size", 56)));
	if (prop_rad_center_info_chk) prop_rad_center_info_chk->set_pressed(lay.has("radial_show_center_info") ? bool(lay["radial_show_center_info"]) : true);
	if (prop_rad_open_anim_opt) s_select_option_meta(prop_rad_open_anim_opt, String(lay.get("radial_open_anim", "radial_spin")));

	if (prop_rad_slot_opt) {
		prop_rad_slot_opt->clear();
		for (int i = 0; i < slots.size(); i++) {
			Dictionary rs = slots[i].get_type() == Variant::DICTIONARY ? Dictionary(slots[i]) : Dictionary();
			String stitle = String(rs.get("title", String::utf8("Ranura ") + itos(i + 1)));
			prop_rad_slot_opt->add_item(itos(i + 1) + ". " + stitle, i);
			prop_rad_slot_opt->set_item_metadata(i, i);
		}
		if (slots.size() > 0) {
			prop_rad_selected_slot = CLAMP(prop_rad_selected_slot, 0, slots.size() - 1);
			prop_rad_slot_opt->select(prop_rad_selected_slot);
		} else {
			prop_rad_selected_slot = 0;
		}
	}
	bool has_slot = (prop_rad_selected_slot >= 0 && prop_rad_selected_slot < slots.size() && slots[prop_rad_selected_slot].get_type() == Variant::DICTIONARY);
	Dictionary cur_s = has_slot ? Dictionary(slots[prop_rad_selected_slot]) : Dictionary();
	if (prop_rad_del_slot_btn) prop_rad_del_slot_btn->set_disabled(slots.size() <= 1);
	if (prop_rad_slot_name_edit) prop_rad_slot_name_edit->set_text(String(cur_s.get("title", "")));
	if (prop_rad_slot_desc_edit) prop_rad_slot_desc_edit->set_text(String(cur_s.get("description", "")));
	if (prop_rad_slot_icon_edit) prop_rad_slot_icon_edit->set_text(String(cur_s.get("icon_path", "")));
	_populate_multi_actions_list(prop_rad_slot_actions_vbox, cur_s.get("actions", Array()));
	updating_ui = prev_upd;
}

void ClockGearsUiDesignEditor::_on_prop_radial_changed() {
	if (updating_ui) {
		return;
	}
	Dictionary lay = _get_active_radial_layout();
	lay["mode"] = "radial";
	if (prop_rad_radius_slider) lay["radial_radius"] = int(Math::round(prop_rad_radius_slider->get_value()));
	if (prop_rad_start_slider) lay["radial_start_angle_deg"] = float(prop_rad_start_slider->get_value());
	bool full = prop_rad_full_chk ? prop_rad_full_chk->is_pressed() : false;
	if (full) {
		lay["radial_sweep_angle_deg"] = 360.0;
	} else if (prop_rad_sweep_slider) {
		double sw = prop_rad_sweep_slider->get_value();
		lay["radial_sweep_angle_deg"] = (sw >= 359.5) ? 270.0 : sw;
	}
	if (prop_rad_cw_chk) lay["radial_clockwise"] = prop_rad_cw_chk->is_pressed();
	if (prop_rad_slot_size_slider) lay["radial_slot_size"] = int(Math::round(prop_rad_slot_size_slider->get_value()));
	if (prop_rad_center_info_chk) lay["radial_show_center_info"] = prop_rad_center_info_chk->is_pressed();
	if (prop_rad_open_anim_opt && prop_rad_open_anim_opt->get_selected() >= 0) {
		lay["radial_open_anim"] = String(prop_rad_open_anim_opt->get_item_metadata(prop_rad_open_anim_opt->get_selected()));
	}
	_commit_active_radial_layout(lay, false);
}

void ClockGearsUiDesignEditor::_on_prop_radial_count_changed(double p_val) {
	if (updating_ui) {
		return;
	}
	int target = CLAMP(int(Math::round(p_val)), 1, 16);
	Dictionary lay = _get_active_radial_layout();
	lay["mode"] = "radial";
	Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
	if (slots.size() < target) {
		Array defs = ClockGearsDataManager::make_default_radial_slots(target, current_screen_id);
		while (slots.size() < target) {
			int idx = slots.size();
			Dictionary ns = defs[idx];
			ns["id"] = vformat("slot_%d", idx + 1);
			slots.push_back(ns);
		}
	} else if (slots.size() > target) {
		slots.resize(target);
	}
	lay["radial_slots"] = slots;
	prop_rad_selected_slot = CLAMP(prop_rad_selected_slot, 0, target - 1);
	_commit_active_radial_layout(lay, true);
}

void ClockGearsUiDesignEditor::_on_prop_radial_slot_selected(int p_idx) {
	if (updating_ui) {
		return;
	}
	prop_rad_selected_slot = MAX(0, p_idx);
	_refresh_radial_inspector();
}

void ClockGearsUiDesignEditor::_on_prop_radial_slot_edited() {
	if (updating_ui) {
		return;
	}
	Dictionary lay = _get_active_radial_layout();
	Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
	if (prop_rad_selected_slot < 0 || prop_rad_selected_slot >= slots.size()) {
		return;
	}
	Dictionary rs = Dictionary(slots[prop_rad_selected_slot]).duplicate(true);
	if (prop_rad_slot_name_edit) rs["title"] = prop_rad_slot_name_edit->get_text();
	if (prop_rad_slot_desc_edit) rs["description"] = prop_rad_slot_desc_edit->get_text();
	if (prop_rad_slot_icon_edit) rs["icon_path"] = prop_rad_slot_icon_edit->get_text().strip_edges();
	slots[prop_rad_selected_slot] = rs;
	lay["radial_slots"] = slots;
	_commit_active_radial_layout(lay, false, false);
}

void ClockGearsUiDesignEditor::_on_prop_radial_del_slot_pressed() {
	Dictionary lay = _get_active_radial_layout();
	Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
	if (slots.size() <= 1 || prop_rad_selected_slot < 0 || prop_rad_selected_slot >= slots.size()) {
		return;
	}
	slots.remove_at(prop_rad_selected_slot);
	prop_rad_selected_slot = CLAMP(prop_rad_selected_slot, 0, slots.size() - 1);
	lay["radial_slots"] = slots;
	_commit_active_radial_layout(lay, true);
}

void ClockGearsUiDesignEditor::_on_prop_radial_slot_icon_import_pressed() {
	image_import_target = 20;
	_on_prop_import_image_pressed();
}

void ClockGearsUiDesignEditor::set_radial_layout_live(int p_radius, float p_start_deg) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary sel = _get_selected_element_copy();
	if (!sel.is_empty() && String(sel.get("type", "")) == "radial") {
		Dictionary lay = sel.get("layout", Dictionary()).duplicate(true);
		lay["mode"] = "radial";
		lay["radial_radius"] = CLAMP(p_radius, 48, 250);
		lay["radial_start_angle_deg"] = CLAMP(p_start_deg, -360.0f, 360.0f);
		sel["layout"] = ClockGearsDataManager::normalize_menu_layout(lay);
		dm->update_ui_element(current_screen_id, selected_elem_id, sel, false);
		_refresh_canvas_stage();
		return;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
	Dictionary lay = d.get("layout", Dictionary());
	lay["mode"] = "radial";
	lay["radial_radius"] = CLAMP(p_radius, 48, 250);
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
	bool is_el = false;
	Dictionary lay = _get_active_radial_layout(&is_el);
	lay["mode"] = "radial";
	Array rslots = Array(lay.get("radial_slots", Array())).duplicate(true);
	int next_idx = rslots.size() + 1;
	Dictionary ns;
	ns["id"] = vformat("slot_%d", next_idx);
	ns["title"] = String::utf8("Ranura ") + itos(next_idx);
	ns["description"] = "";
	ns["icon_path"] = "";
	ns["pixel_art"] = true;
	ns["action"] = "";
	ns["actions"] = Array();
	rslots.push_back(ns);
	lay["radial_slots"] = rslots;
	prop_rad_selected_slot = rslots.size() - 1;
	_commit_active_radial_layout(lay, true);
	return true;
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
		Array tpls = ClockGearsDataManager::get_ui_templates_for_system(current_screen_id);
		if (!tpls.is_empty() && tpls[0].get_type() == Variant::DICTIONARY) {
			wizard_selected_template_id = String(Dictionary(tpls[0]).get("id", "classic_jrpg"));
		}
		for (int i = 0; i < tpls.size(); i++) {
			Dictionary t = tpls[i];
			String tid = String(t.get("id", ""));
			PanelContainer *card = memnew(PanelContainer);
			card->set_theme_type_variation("ClockGearsContainer");
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
			sb->set_meta("tpl_id", tid);
			s_make_rounded_action_btn(sb);
			sb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_tpl_selected).bind(tid));
			vb->add_child(sb);
			wizard_tpl_grid->add_child(card);
		}
	}
	_update_wizard_step_ui();
	_refresh_wizard_conflict_label();
	ClockGearsTheme::popup_fitted_window(
			menu_wizard_dialog,
			Size2i(int(820 * EDSCALE), int(580 * EDSCALE)),
			Size2i(int(760 * EDSCALE), int(560 * EDSCALE)),
			"ui_menu_wizard");
}

void ClockGearsUiDesignEditor::_on_wizard_tpl_selected(const String &p_tpl_id) {
	wizard_selected_template_id = p_tpl_id;
	if (wizard_tpl_grid) {
		for (int i = 0; i < wizard_tpl_grid->get_child_count(); i++) {
			PanelContainer *card = Object::cast_to<PanelContainer>(wizard_tpl_grid->get_child(i));
			if (card && card->get_child_count() > 0 && card->get_child(0)->get_child_count() > 1) {
				Button *b = Object::cast_to<Button>(card->get_child(0)->get_child(1));
				if (b) b->set_pressed_no_signal(String(b->get_meta("tpl_id", "")) == p_tpl_id);
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
	Dictionary opts = p_opts.duplicate(true);
	if (!system_scope_root.is_empty()) {
		if (!opts.has("parent_id")) {
			opts["parent_id"] = current_screen_id.is_empty() ? system_scope_root : current_screen_id;
		}
		if (!opts.has("system_group")) {
			opts["system_group"] = system_scope_root;
		}
	}
	String nid = dm->create_menu_from_wizard(opts);
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
	Array sys_tpls = ClockGearsDataManager::get_ui_templates_for_system(current_screen_id, true);
	bool sel_in_sys = false;
	for (int i = 0; i < sys_tpls.size(); i++) {
		if (String(Dictionary(sys_tpls[i]).get("id", "")) == selected_gallery_template_id) {
			sel_in_sys = true;
			break;
		}
	}
	if ((selected_gallery_template_id.is_empty() || !sel_in_sys) && !sys_tpls.is_empty() && sys_tpls[0].get_type() == Variant::DICTIONARY) {
		selected_gallery_template_id = String(Dictionary(sys_tpls[0]).get("id", ""));
	}
	auto fill_grid = [&](GridContainer *p_grid, const Array &p_list) {
		if (!p_grid) return;
		s_clear_container_children(p_grid);
		for (int i = 0; i < p_list.size(); i++) {
			Dictionary t = p_list[i];
			String tid = String(t.get("id", ""));
			PanelContainer *card = memnew(PanelContainer);
			card->set_theme_type_variation("ClockGearsContainer");
			card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			VBoxContainer *vb = memnew(VBoxContainer);
			vb->add_theme_constant_override("separation", int(4 * EDSCALE));
			card->add_child(vb);
			ClockGearsTemplateThumbCtrl *th = memnew(ClockGearsTemplateThumbCtrl);
			th->set_custom_minimum_size(Size2(192 * EDSCALE, 108 * EDSCALE));
			th->setup_template(t);
			vb->add_child(th);
			String tname = String(t.get("name", tid));
			String tdesc = String(t.get("description", ""));
			String tpl_tip = tdesc.is_empty() ? tname : (tname + "\n" + tdesc);
			Button *sb = memnew(Button(tname));
			sb->set_toggle_mode(true);
			sb->set_pressed(tid == selected_gallery_template_id);
			sb->set_clip_text(true);
			sb->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			sb->set_tooltip_text(tpl_tip);
			sb->set_meta("tpl_id", tid);
			ClockGearsTheme::set_control_long_help(sb, tpl_tip);
			ClockGearsTheme::set_control_long_help(card, tpl_tip);
			s_make_rounded_action_btn(sb);
			sb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_card_selected).bind(tid));
			vb->add_child(sb);
			if (!tdesc.is_empty()) {
				Label *dl = memnew(Label(tdesc));
				dl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
				dl->set_max_lines_visible(2);
				dl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				dl->add_theme_font_size_override("font_size", MAX(10, int(Math::round(11.0f * EDSCALE))));
				dl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
				vb->add_child(dl);
			}
			p_grid->add_child(card);
		}
	};
	Array comp_tpls = ClockGearsDataManager::get_builtin_component_templates(current_screen_id);
	Array raw_user = dm ? dm->get_user_menu_templates() : Array();
	Array compat_user;
	for (int i = 0; i < raw_user.size(); i++) {
		if (raw_user[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary ut = raw_user[i];
		if (ClockGearsDataManager::is_ui_template_compatible_with_screen(ut, current_screen_id)) {
			compat_user.push_back(ut);
		}
	}
	fill_grid(tpl_screen_grid, sys_tpls);
	fill_grid(tpl_comp_grid, comp_tpls);
	fill_grid(tpl_user_grid, compat_user);
}

void ClockGearsUiDesignEditor::_on_tpl_close_requested() {
	pending_apply_template_id = "";
	if (template_gallery_dialog) {
		template_gallery_dialog->hide();
	}
}

void ClockGearsUiDesignEditor::_on_tpl_window_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> ek = p_event;
	if (ek.is_valid() && ek->is_pressed() && !ek->is_echo() && ek->get_keycode() == Key::ESCAPE) {
		_on_tpl_close_requested();
	}
}

void ClockGearsUiDesignEditor::_on_templates_btn_pressed() {
	if (!template_gallery_dialog) {
		return;
	}
	_populate_template_gallery();
	if (!selected_gallery_template_id.is_empty()) {
		_on_tpl_card_selected(selected_gallery_template_id);
	}
	ClockGearsTheme::popup_fitted_window(
			template_gallery_dialog,
			Size2i(int(900 * EDSCALE), int(640 * EDSCALE)),
			Size2i(int(760 * EDSCALE), int(560 * EDSCALE)),
			"ui_template_gallery");
}

void ClockGearsUiDesignEditor::_on_tpl_card_selected(const String &p_tpl_id) {
	selected_gallery_template_id = p_tpl_id;
	auto sync_grid = [&](GridContainer *p_grid) {
		if (!p_grid) return;
		for (int i = 0; i < p_grid->get_child_count(); i++) {
			PanelContainer *card = Object::cast_to<PanelContainer>(p_grid->get_child(i));
			if (card && card->get_child_count() > 0 && card->get_child(0)->get_child_count() > 1) {
				Button *b = Object::cast_to<Button>(card->get_child(0)->get_child(1));
				if (b) b->set_pressed_no_signal(String(b->get_meta("tpl_id", "")) == p_tpl_id);
			}
		}
	};
	sync_grid(tpl_screen_grid);
	sync_grid(tpl_comp_grid);
	sync_grid(tpl_user_grid);
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
	if (menu_bg_scale_opt && menu_bg_scale_opt->get_selected() >= 0) {
		bg["scale_mode"] = String(menu_bg_scale_opt->get_item_metadata(menu_bg_scale_opt->get_selected()));
	}
	if (menu_bg_pixel_chk) bg["pixel_art"] = menu_bg_pixel_chk->is_pressed();
	if (menu_bg_scroll_x_spin) bg["scroll_speed_x"] = menu_bg_scroll_x_spin->get_value();
	if (menu_bg_scroll_y_spin) bg["scroll_speed_y"] = menu_bg_scroll_y_spin->get_value();
	if (menu_bg_blur_chk) bg["blur_enabled"] = menu_bg_blur_chk->is_pressed();
	if (menu_bg_blur_slider) bg["blur_intensity"] = int(Math::round(menu_bg_blur_slider->get_value()));
	if (menu_bg_veil_chk) {
		bool veil_on = menu_bg_veil_chk->is_pressed();
		bg["veil_enabled"] = veil_on;
		bg["dark_veil"] = veil_on;
	}
	if (menu_bg_veil_col_btn || menu_bg_veil_opacity_slider) {
		Color vc = menu_bg_veil_col_btn ? menu_bg_veil_col_btn->get_pick_color() : Color(0.02f, 0.03f, 0.05f, 1.0f);
		float va = menu_bg_veil_opacity_slider ? float(CLAMP(menu_bg_veil_opacity_slider->get_value(), 0.0, 100.0) / 100.0) : 0.65f;
		vc.a = va;
		bg["veil_color"] = "#" + vc.to_html(true);
	}
	d["background"] = ClockGearsDataManager::normalize_ui_background(bg);
	s_sync_manual_adjust_btn_state(menu_bg_adjust_manual_btn, String(bg.get("scale_mode", "stretch")), !String(bg.get("image_path", "")).strip_edges().is_empty());

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

	// Sincronización bidireccional con configuraciones específicas de pantallas estándar (9B Corr Punto 5)
	Dictionary synced_bg;
	String bg_t = String(bg.get("type", "default"));
	if (bg_t != "default") {
		synced_bg["type"] = (bg_t == "image") ? "image" : "color";
		synced_bg["color"] = String(bg.get("color", "#141824"));
		synced_bg["image_path"] = String(bg.get("image_path", ""));
	}
	if (current_screen_id == "title") {
		Dictionary tcfg = dm->get_title_menu_config();
		if (!synced_bg.is_empty()) tcfg["background"] = synced_bg;
		if (menu_bgm_edit) tcfg["bgm_path"] = d.get("bgm_path", "");
		dm->set_title_menu_config(tcfg);
	} else if (current_screen_id == "credits") {
		Dictionary ccfg = dm->get_credits_config();
		if (!synced_bg.is_empty()) ccfg["background"] = synced_bg;
		if (menu_bgm_edit) ccfg["bgm_path"] = d.get("bgm_path", "");
		dm->set_credits_config(ccfg);
	} else if (current_screen_id == "game_over") {
		Dictionary gcfg = dm->get_game_over_config();
		if (!synced_bg.is_empty()) gcfg["background"] = synced_bg;
		if (menu_bgm_edit) gcfg["bgm_path"] = d.get("bgm_path", "");
		dm->set_game_over_config(gcfg);
	} else if (current_screen_id.begins_with("game_menu_") && !synced_bg.is_empty()) {
		String sub = current_screen_id.trim_prefix("game_menu_");
		if (sub == "main") sub = "menu";
		if (sub == "menu" || sub == "items" || sub == "equip" || sub == "status") {
			Dictionary gm = dm->get_game_menu_config();
			Dictionary bgs = Dictionary(gm.get("backgrounds", Dictionary())).duplicate(true);
			bgs[sub] = synced_bg;
			gm["backgrounds"] = bgs;
			dm->set_game_menu_config(gm);
		}
	}

	_refresh_canvas_stage();
	_refresh_screen_settings_panel();
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

ClockGearsImageAdjustDialog *ClockGearsUiDesignEditor::_ensure_image_adjust_dialog() {
	if (!image_adjust_dialog) {
		image_adjust_dialog = memnew(ClockGearsImageAdjustDialog);
		add_child(image_adjust_dialog);
	}
	return image_adjust_dialog;
}

void ClockGearsUiDesignEditor::open_manual_image_adjust_for_dict(
		const String &p_title,
		const Vector2 &p_logical_sz,
		const Dictionary &p_shape_cfg,
		const Dictionary &p_cur_cfg,
		const Vector<Pair<String, String>> &p_state_imgs,
		bool p_pixel_art,
		const Callable &p_on_confirm) {
	ClockGearsImageAdjustDialog *dlg = _ensure_image_adjust_dialog();
	if (!dlg) {
		return;
	}
	dlg->open_adjust(p_title, p_logical_sz, p_shape_cfg, p_cur_cfg, p_state_imgs, p_pixel_art, p_on_confirm);
}

void ClockGearsUiDesignEditor::open_manual_image_adjust_for_element_dict(const Dictionary &p_el, const Callable &p_on_confirm) {
	if (p_el.is_empty()) {
		return;
	}
	String sm = String(p_el.get("scale_mode", "stretch")).strip_edges().to_lower();
	if (sm == "nine_slice") {
		return;
	}
	String etype = String(p_el.get("type", "image")).strip_edges().to_lower();
	Vector<Pair<String, String>> imgs;
	if (etype == "button") {
		for (const Pair<const char *, const char *> &st : {
				Pair<const char *, const char *>("Normal", "image_normal"),
				Pair<const char *, const char *>("Encima", "image_hover"),
				Pair<const char *, const char *>("Pulsado", "image_pressed"),
				Pair<const char *, const char *>("Desact.", "image_disabled") }) {
			String p = String(p_el.get(st.second, "")).strip_edges();
			if (!p.is_empty()) {
				imgs.push_back(Pair<String, String>(String::utf8(st.first), p));
			}
		}
	}
	String main_p = String(p_el.get("image_path", p_el.get("icon_path", ""))).strip_edges();
	if (!main_p.is_empty() && imgs.is_empty()) {
		imgs.push_back(Pair<String, String>(String("Imagen"), main_p));
	}
	if (imgs.is_empty()) {
		return;
	}
	Vector2 fsz(MAX(16.0f, float(p_el.get("width", 120))), MAX(16.0f, float(p_el.get("height", 80))));
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary shape_st = s_extract_elem_shape_ui_state(dm, p_el);
	bool pa = p_el.has("pixel_art") ? bool(p_el["pixel_art"]) : true;
	open_manual_image_adjust_for_dict(
			String::utf8("Ajustar imagen: ") + String(p_el.get("name", p_el.get("id", "elemento"))),
			fsz,
			shape_st,
			p_el,
			imgs,
			pa,
			p_on_confirm);
}

void ClockGearsUiDesignEditor::open_manual_image_adjust_for_selected() {
	if (selected_elem_id.is_empty()) {
		return;
	}
	Dictionary el = get_effective_element(selected_elem_id);
	open_manual_image_adjust_for_element_dict(el, callable_mp(this, &ClockGearsUiDesignEditor::_on_manual_adjust_elem_confirmed));
}

void ClockGearsUiDesignEditor::open_manual_image_adjust_for_screen_bg() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id);
	Dictionary bg = d.get("background", Dictionary());
	String sm = String(bg.get("scale_mode", "stretch")).strip_edges().to_lower();
	String ip = String(bg.get("image_path", "")).strip_edges();
	if (sm == "nine_slice" || ip.is_empty()) {
		return;
	}
	Vector<Pair<String, String>> imgs;
	imgs.push_back(Pair<String, String>(String("Fondo"), ip));
	Dictionary frame_shape;
	frame_shape["shape_type"] = "rect";
	bool pa = bg.has("pixel_art") ? bool(bg["pixel_art"]) : true;
	open_manual_image_adjust_for_dict(
			String::utf8("Ajustar fondo de pantalla"),
			Vector2(BASE_W, BASE_H),
			frame_shape,
			bg,
			imgs,
			pa,
			callable_mp(this, &ClockGearsUiDesignEditor::_on_manual_adjust_bg_confirmed));
}

void ClockGearsUiDesignEditor::_on_prop_image_adjust_manual_pressed() {
	open_manual_image_adjust_for_selected();
}

void ClockGearsUiDesignEditor::_on_menu_bg_adjust_manual_pressed() {
	open_manual_image_adjust_for_screen_bg();
}

void ClockGearsUiDesignEditor::_on_manual_adjust_elem_confirmed(const Dictionary &p_vals) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return;
	}
	Dictionary el = get_effective_element(selected_elem_id);
	if (el.is_empty()) {
		return;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	Array keys = p_vals.keys();
	for (int i = 0; i < keys.size(); i++) {
		el[keys[i]] = p_vals[keys[i]];
	}
	el["scale_mode"] = "manual";
	if (dm->update_ui_element(current_screen_id, selected_elem_id, el, false)) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
		_refresh_properties_panel();
		_refresh_layers_list();
		_refresh_canvas_stage();
		_refresh_undo_redo_buttons();
		_sync_open_floating_windows();
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_manual_adjust_bg_confirmed(const Dictionary &p_vals) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
	Dictionary bg = Dictionary(d.get("background", Dictionary())).duplicate(true);
	Array keys = p_vals.keys();
	for (int i = 0; i < keys.size(); i++) {
		bg[keys[i]] = p_vals[keys[i]];
	}
	bg["scale_mode"] = "manual";
	d["background"] = ClockGearsDataManager::normalize_ui_background(bg);
	dm->set_ui_screen_design(current_screen_id, d, false);
	dm->push_ui_design_undo_state(current_screen_id, snap);
	_refresh_menu_global_properties();
	_refresh_canvas_stage();
	_refresh_undo_redo_buttons();
	_notify_data_mutated(true);
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
	if (!dm || el.is_empty()) {
		return;
	}
	if (String(el.get("type", "")) == "radial") {
		if (!prop_rad_slot_add_act_opt || prop_rad_slot_add_act_opt->get_selected() < 0) return;
		Dictionary lay = _get_active_radial_layout();
		Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
		if (prop_rad_selected_slot < 0 || prop_rad_selected_slot >= slots.size()) return;
		Dictionary rs = Dictionary(slots[prop_rad_selected_slot]).duplicate(true);
		Array acts = rs.get("actions", Array()).duplicate(true);
		Dictionary na;
		na["type"] = String(prop_rad_slot_add_act_opt->get_item_metadata(prop_rad_slot_add_act_opt->get_selected()));
		acts.push_back(na);
		rs["actions"] = acts;
		slots[prop_rad_selected_slot] = rs;
		lay["radial_slots"] = slots;
		_commit_active_radial_layout(lay, true);
		return;
	}
	if (!prop_add_action_opt || prop_add_action_opt->get_selected() < 0) {
		return;
	}
	String atype = String(prop_add_action_opt->get_item_metadata(prop_add_action_opt->get_selected()));
	Array acts = el.get("actions", Array()).duplicate(true);
	Dictionary na;
	na["type"] = atype;
	acts.push_back(na);
	el["actions"] = acts;
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_remove_multi_action_pressed(int p_idx) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	if (String(el.get("type", "")) == "radial") {
		Dictionary lay = _get_active_radial_layout();
		Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
		if (prop_rad_selected_slot < 0 || prop_rad_selected_slot >= slots.size()) return;
		Dictionary rs = Dictionary(slots[prop_rad_selected_slot]).duplicate(true);
		Array acts = rs.get("actions", Array()).duplicate(true);
		if (p_idx >= 0 && p_idx < acts.size()) {
			acts.remove_at(p_idx);
			rs["actions"] = acts;
			slots[prop_rad_selected_slot] = rs;
			lay["radial_slots"] = slots;
			_commit_active_radial_layout(lay, true);
		}
		return;
	}
	Array acts = el.get("actions", Array()).duplicate(true);
	if (p_idx >= 0 && p_idx < acts.size()) {
		acts.remove_at(p_idx);
		el["actions"] = acts;
		Dictionary snap = dm->get_ui_screen_design(current_screen_id);
		bool prev_custom = dm->has_custom_ui_design(current_screen_id);
		ensure_screen_customizable_for_edit(false);
		dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
		if (!prev_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_prop_move_multi_action_pressed(int p_idx, int p_delta) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	if (String(el.get("type", "")) == "radial") {
		Dictionary lay = _get_active_radial_layout();
		Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
		if (prop_rad_selected_slot < 0 || prop_rad_selected_slot >= slots.size()) return;
		Dictionary rs = Dictionary(slots[prop_rad_selected_slot]).duplicate(true);
		Array acts = rs.get("actions", Array()).duplicate(true);
		int target = p_idx + p_delta;
		if (p_idx < 0 || p_idx >= acts.size() || target < 0 || target >= acts.size() || target == p_idx) return;
		Variant moved = acts[p_idx];
		acts.remove_at(p_idx);
		acts.insert(target, moved);
		rs["actions"] = acts;
		slots[prop_rad_selected_slot] = rs;
		lay["radial_slots"] = slots;
		_commit_active_radial_layout(lay, true);
		return;
	}
	Array acts = el.get("actions", Array()).duplicate(true);
	int target = p_idx + p_delta;
	if (p_idx < 0 || p_idx >= acts.size() || target < 0 || target >= acts.size() || target == p_idx) {
		return;
	}
	Variant moved = acts[p_idx];
	acts.remove_at(p_idx);
	acts.insert(target, moved);
	el["actions"] = acts;
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_edit_multi_action_step(const Variant &p_val, int p_idx, const String &p_key) {
	if (updating_ui) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	if (String(el.get("type", "")) == "radial") {
		Dictionary lay = _get_active_radial_layout();
		Array slots = Array(lay.get("radial_slots", Array())).duplicate(true);
		if (prop_rad_selected_slot < 0 || prop_rad_selected_slot >= slots.size()) return;
		Dictionary rs = Dictionary(slots[prop_rad_selected_slot]).duplicate(true);
		Array acts = rs.get("actions", Array()).duplicate(true);
		if (p_idx < 0 || p_idx >= acts.size() || acts[p_idx].get_type() != Variant::DICTIONARY) return;
		Dictionary step = Dictionary(acts[p_idx]).duplicate(true);
		step[p_key] = p_val;
		if (p_key == "menu_id") step["target"] = p_val;
		acts[p_idx] = step;
		rs["actions"] = acts;
		slots[prop_rad_selected_slot] = rs;
		lay["radial_slots"] = slots;
		_commit_active_radial_layout(lay, false, false);
		return;
	}
	Array acts = el.get("actions", Array()).duplicate(true);
	if (p_idx < 0 || p_idx >= acts.size() || acts[p_idx].get_type() != Variant::DICTIONARY) {
		return;
	}
	Dictionary step = Dictionary(acts[p_idx]).duplicate(true);
	step[p_key] = p_val;
	if (p_key == "menu_id") {
		step["target"] = p_val;
	}
	acts[p_idx] = step;
	el["actions"] = acts;
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_prop_edit_multi_action_menu(int p_opt_idx, ObjectID p_opt_id, int p_idx, const String &p_key) {
	OptionButton *opt = Object::cast_to<OptionButton>(ObjectDB::get_instance(p_opt_id));
	if (!opt || p_opt_idx < 0) {
		return;
	}
	_on_prop_edit_multi_action_step(opt->get_item_metadata(p_opt_idx), p_idx, p_key);
}

void ClockGearsUiDesignEditor::_on_prop_btn_state_import_pressed(int p_state) {
	image_import_target = 10 + CLAMP(p_state, 0, 3);
	_on_prop_import_image_pressed();
}

void ClockGearsUiDesignEditor::_on_prop_btn_state_clear_pressed(int p_state) {
	int s = CLAMP(p_state, 0, 3);
	if (prop_btn_state_img_edits[s]) {
		prop_btn_state_img_edits[s]->set_text(String());
		_on_prop_field_changed();
	}
}

void ClockGearsUiDesignEditor::_on_prop_btn_shortcut_capture_pressed() {
	capturing_key_for_btn_shortcut = true;
	if (prop_btn_shortcut_lbl) {
		prop_btn_shortcut_lbl->set_text("Pulsa tecla/mando...");
	}
}

void ClockGearsUiDesignEditor::_on_prop_btn_shortcut_clear_pressed() {
	capturing_key_for_btn_shortcut = false;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	el.erase("shortcut");
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_screen_selected(int p_idx) {
	if (updating_ui || !screen_opt || p_idx < 0) {
		return;
	}
	select_screen(String(screen_opt->get_item_metadata(p_idx)));
}

void ClockGearsUiDesignEditor::_on_zoom_pressed(int p_mode) {
	set_canvas_zoom_mode(p_mode);
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
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	el["anchor"] = _anchor_key_by_index(p_idx);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, was_custom);
	if (!was_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
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
			(!prop_locked_chk || bool(el.get("locked", false)) == prop_locked_chk->is_pressed()) &&
			(!prop_clip_children_chk || bool(el.get("clip_children", false)) == prop_clip_children_chk->is_pressed()) &&
			(!prop_parent_layout_opt || prop_parent_layout_opt->get_selected() < 0 || String(el.get("layout_mode", "free")) == String(prop_parent_layout_opt->get_item_metadata(prop_parent_layout_opt->get_selected()))) &&
			(!prop_parent_cols_spin || int(el.get("layout_columns", 2)) == int(Math::round(prop_parent_cols_spin->get_value()))) &&
			(!prop_parent_space_spin || int(el.get("layout_spacing", 8)) == int(Math::round(prop_parent_space_spin->get_value()))) &&
			was_custom &&
			(!prop_show_bg_chk || bool(el.get("show_background", false)) == prop_show_bg_chk->is_pressed()) &&
			(!prop_opacity_slider || int(Math::round(float(el.get("opacity", 1.0)) * 100.0f)) == int(Math::round(prop_opacity_slider->get_value()))));
	el["name"] = prop_name_edit->get_text();
	el["offset_x"] = int(Math::round(prop_ox_spin->get_value()));
	el["offset_y"] = int(Math::round(prop_oy_spin->get_value()));
	el["width"] = int(Math::round(prop_w_spin->get_value()));
	el["height"] = int(Math::round(prop_h_spin->get_value()));
	el["layer_order"] = int(Math::round(prop_layer_spin->get_value()));
	el["visible"] = prop_visible_chk->is_pressed();
	if (prop_locked_chk) {
		el["locked"] = prop_locked_chk->is_pressed();
	}
	if (prop_clip_children_chk) {
		el["clip_children"] = prop_clip_children_chk->is_pressed();
	}
	if (prop_parent_layout_opt && prop_parent_layout_opt->get_selected() >= 0) {
		String plm = String(prop_parent_layout_opt->get_item_metadata(prop_parent_layout_opt->get_selected()));
		if (plm == "free") {
			el.erase("layout_mode");
		} else {
			el["layout_mode"] = plm;
		}
	}
	if (prop_parent_cols_spin) {
		el["layout_columns"] = int(Math::round(prop_parent_cols_spin->get_value()));
	}
	if (prop_parent_space_spin) {
		el["layout_spacing"] = int(Math::round(prop_parent_space_spin->get_value()));
	}
	if (prop_show_bg_chk) {
		el["show_background"] = prop_show_bg_chk->is_pressed();
	}
	if (prop_opacity_slider) {
		int op_pct = CLAMP(int(Math::round(prop_opacity_slider->get_value())), 0, 100);
		el["opacity"] = double(op_pct) / 100.0;
		if (prop_opacity_val_lbl) prop_opacity_val_lbl->set_text(itos(op_pct) + "%");
	}
	bool style_changed = false;
	if (prop_style_opt && prop_style_opt->get_selected() >= 0) {
		String next_ws = String(prop_style_opt->get_item_metadata(prop_style_opt->get_selected()));
		if (String(el.get("window_style_id", "")) != next_ws) {
			only_geom = false;
			style_changed = true;
		}
		el["window_style_id"] = next_ws;
	}
	Dictionary base_sh = s_extract_elem_shape_ui_state(dm, el);
	Dictionary next_sh = base_sh.duplicate(true);
	if (prop_shape_opt && prop_shape_opt->get_selected() >= 0) {
		next_sh["shape_type"] = String(prop_shape_opt->get_item_metadata(prop_shape_opt->get_selected()));
	}
	if (prop_radius_slider) {
		int nr = int(Math::round(prop_radius_slider->get_value()));
		next_sh["corner_radius"] = nr;
		if (prop_radius_val_lbl) prop_radius_val_lbl->set_text(itos(nr) + "px");
	}
	if (prop_fill_color_btn) {
		String fc = "#" + prop_fill_color_btn->get_pick_color().to_html(true);
		if (String(base_sh.get("fill_color", "")) != fc) {
			dm->record_recent_ui_color(fc);
			if (etype == "bar") {
				el["fill_color"] = fc;
				only_geom = false;
			}
		}
		next_sh["fill_color"] = fc;
	}
	if (prop_grad_chk) next_sh["gradient_enabled"] = prop_grad_chk->is_pressed();
	if (prop_grad_top_btn) next_sh["gradient_top_color"] = "#" + prop_grad_top_btn->get_pick_color().to_html(true);
	if (prop_grad_bot_btn) next_sh["gradient_bottom_color"] = "#" + prop_grad_bot_btn->get_pick_color().to_html(true);
	if (prop_border_w_slider) next_sh["border_width"] = int(Math::round(prop_border_w_slider->get_value()));
	if (prop_border_col_btn) next_sh["border_color"] = "#" + prop_border_col_btn->get_pick_color().to_html(true);
	if (prop_shadow_chk) next_sh["shadow_enabled"] = prop_shadow_chk->is_pressed();
	if (prop_glow_chk) next_sh["glow_enabled"] = prop_glow_chk->is_pressed();
	if (prop_glow_col_btn) next_sh["glow_color"] = "#" + prop_glow_col_btn->get_pick_color().to_html(true);
	if (prop_clip_chk) next_sh["clip_content"] = prop_clip_chk->is_pressed();
	if (prop_pixel_art_chk) {
		bool pa = prop_pixel_art_chk->is_pressed();
		if (bool(el.get("pixel_art", true)) != pa) only_geom = false;
		el["pixel_art"] = pa;
		next_sh["pixel_art"] = pa;
	}
	if (style_changed) {
		// Al cambiar de Estilo del tema, limpiar la forma personalizada anterior salvo que el usuario la esté editando aparte
		el.erase("shape");
		only_geom = false;
	} else {
		bool shape_touched = (el.has("shape") && el["shape"].get_type() == Variant::DICTIONARY);
		if (!shape_touched) {
			Array s_keys = next_sh.keys();
			for (int k = 0; k < s_keys.size(); k++) {
				Variant kk = s_keys[k];
				if (kk == "pixel_art") continue;
				if (base_sh.get(kk, Variant()) != next_sh.get(kk, Variant())) {
					shape_touched = true;
					break;
				}
			}
		}
		if (shape_touched) {
			Dictionary prev_sh = (el.has("shape") && el["shape"].get_type() == Variant::DICTIONARY) ? Dictionary(el["shape"]) : Dictionary();
			Dictionary norm_sh = ClockGearsDataManager::normalize_ui_shape(next_sh);
			if (prev_sh != norm_sh) {
				only_geom = false;
			}
			el["shape"] = norm_sh;
		}
	}
	if (prop_anim_opt && prop_anim_opt->get_selected() >= 0) {
		el["in_anim"] = String(prop_anim_opt->get_item_metadata(prop_anim_opt->get_selected()));
	}

	if (prop_text_row->is_visible()) {
		if (etype == "bar") {
			if (String(el.get("label_text", "")) != prop_text_edit->get_text()) only_geom = false;
			el["label_text"] = prop_text_edit->get_text();
		} else {
			if (String(el.get("text", "")) != prop_text_edit->get_text()) only_geom = false;
			el["text"] = prop_text_edit->get_text();
		}
	}
	if (etype == "text" || etype == "button" || etype == "variable" || etype == "bar" || etype == "currency" || etype == "item") {
		if (prop_font_opt && prop_font_opt->get_selected() >= 0) {
			String nf = String(prop_font_opt->get_item_metadata(prop_font_opt->get_selected()));
			if (String(el.get("font_path", "")) != nf) only_geom = false;
			el["font_path"] = nf;
		}
		if (prop_font_size_slider) {
			int nfs = int(Math::round(prop_font_size_slider->get_value()));
			if (int(el.get("font_size", 16)) != nfs) only_geom = false;
			el["font_size"] = nfs;
		}
		if (prop_font_color_btn) {
			String nfc = "#" + prop_font_color_btn->get_pick_color().to_html(true);
			if (String(el.get("font_color", "")) != nfc) only_geom = false;
			el["font_color"] = nfc;
		}
	}
	if (prop_image_row->is_visible()) {
		String ikey = (etype == "currency" || etype == "item") ? "icon_path" : "image_path";
		String next_img = prop_image_edit->get_text().strip_edges();
		if (String(el.get(ikey, "")) != next_img) only_geom = false;
		el[ikey] = next_img;
		if (etype == "image" || etype == "frame" || etype == "button") {
			if (prop_image_scale_opt && prop_image_scale_opt->get_selected() >= 0) {
				String sm = String(prop_image_scale_opt->get_item_metadata(prop_image_scale_opt->get_selected()));
				if (String(el.get("scale_mode", "stretch")) != sm) only_geom = false;
				el["scale_mode"] = sm;
			}
			if (prop_nine_slice_spin) {
				int nm = int(Math::round(prop_nine_slice_spin->get_value()));
				if (int(el.get("nine_slice_margin", 12)) != nm) only_geom = false;
				el["nine_slice_margin"] = nm;
			}
		}
		if (etype == "button") {
			el["icon_path"] = next_img;
			static const char *k_sk[4] = { "image_normal", "image_hover", "image_pressed", "image_disabled" };
			for (int si = 0; si < 4; si++) {
				if (prop_btn_state_img_edits[si]) {
					String sv = prop_btn_state_img_edits[si]->get_text().strip_edges();
					if (String(el.get(k_sk[si], "")) != sv) only_geom = false;
					el[k_sk[si]] = sv;
				}
			}
		}
		if (prop_image_adjust_manual_btn && (etype == "image" || etype == "frame" || etype == "button")) {
			s_sync_manual_adjust_btn_state(prop_image_adjust_manual_btn, String(el.get("scale_mode", "stretch")), s_elem_has_any_image(el));
		}
	}
	if (prop_var_row->is_visible()) {
		String next_var = prop_var_edit->get_text().strip_edges();
		if (String(el.get("variable_name", "")) != next_var) only_geom = false;
		el["variable_name"] = next_var;
		if (etype == "text" && String(el.get("data_binding", "none")) == "variable") {
			el["text_mode"] = "variable";
		}
	}
	if (etype == "image" && prop_rotatable_chk) {
		bool nrot = prop_rotatable_chk->is_pressed();
		if (bool(el.get("rotatable", false)) != nrot) only_geom = false;
		el["rotatable"] = nrot;
	}
	if (etype == "button" && prop_action_row->is_visible()) {
		int next_fo = int(Math::round(prop_focus_spin->get_value()));
		if (int(el.get("focus_order", 0)) != next_fo) only_geom = false;
		el["focus_order"] = next_fo;
		if (prop_sound_hover_edit) el["sound_hover"] = prop_sound_hover_edit->get_text().strip_edges();
		if (prop_sound_click_edit) el["sound_click"] = prop_sound_click_edit->get_text().strip_edges();
		if (prop_vis_mode_opt && prop_vis_sw_edit) {
			String vm = String(prop_vis_mode_opt->get_item_metadata(prop_vis_mode_opt->get_selected()));
			String vsw = prop_vis_sw_edit->get_text().strip_edges();
			el["visibility_mode"] = (vm == "always") ? "always" : "switch";
			el["visibility_invert"] = (vm == "switch_off");
			el["visibility_switch"] = vsw;
			Dictionary vc;
			vc["mode"] = vm;
			vc["switch_name"] = vsw;
			el["visible_condition"] = vc;
			only_geom = false;
		}
		if (prop_en_mode_opt && prop_en_sw_edit) {
			String em = String(prop_en_mode_opt->get_item_metadata(prop_en_mode_opt->get_selected()));
			String esw = prop_en_sw_edit->get_text().strip_edges();
			el["enabled_mode"] = (em == "always") ? "always" : "switch";
			el["enabled_invert"] = (em == "switch_off");
			el["enabled_switch"] = esw;
			Dictionary ec;
			ec["mode"] = em;
			ec["switch_name"] = esw;
			el["enabled_condition"] = ec;
			only_geom = false;
		}
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	if (only_geom) {
		refresh_geometry_fast(false);
		_refresh_undo_redo_buttons();
		emit_signal("design_modified");
	} else {
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_test_data_count_selected(int p_idx) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !test_data_opt || p_idx < 0) {
		return;
	}
	int cnt = int(test_data_opt->get_item_metadata(p_idx));
	dm->set_editor_test_data_count(cnt);
	if (design_layer) {
		design_layer->set_use_editor_test_data(cnt >= 0);
	}
	_refresh_canvas_stage();
}

void ClockGearsUiDesignEditor::_refresh_container_inspector(const Dictionary &p_el) {
	if (!prop_container_box) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;
	String ds = ClockGearsDataManager::normalize_ui_data_source(p_el.get("data_source", "manual"));
	if (prop_data_source_opt) s_select_option_meta(prop_data_source_opt, ds);
	if (prop_data_filter_opt) s_select_option_meta(prop_data_filter_opt, String(p_el.get("data_filter_category", "")));
	if (prop_data_usable_chk) prop_data_usable_chk->set_pressed(bool(p_el.get("data_only_usable", false)));
	if (prop_data_sort_opt) s_select_option_meta(prop_data_sort_opt, String(p_el.get("data_sort_by", "id")));
	if (prop_data_equip_slot_spin) prop_data_equip_slot_spin->set_value(CLAMP(int(p_el.get("data_equip_slot", 0)), 0, 7));
	if (prop_container_layout_opt) s_select_option_meta(prop_container_layout_opt, String(p_el.get("layout_mode", "grid")));
	if (prop_container_cols_spin) prop_container_cols_spin->set_value(CLAMP(int(p_el.get("layout_columns", 4)), 1, 20));
	if (prop_container_rows_spin) prop_container_rows_spin->set_value(CLAMP(int(p_el.get("layout_rows", 2)), 0, 20));
	if (prop_container_spx_spin) prop_container_spx_spin->set_value(CLAMP(int(p_el.get("layout_spacing", 8)), 0, 64));
	if (prop_container_spy_spin) prop_container_spy_spin->set_value(CLAMP(int(p_el.get("layout_spacing_y", p_el.get("layout_spacing", 8))), 0, 64));
	if (prop_container_scroll_chk) prop_container_scroll_chk->set_pressed(p_el.has("layout_scroll") ? bool(p_el["layout_scroll"]) : true);
	if (prop_data_show_empty_chk) prop_data_show_empty_chk->set_pressed(p_el.has("data_show_empty") ? bool(p_el["data_show_empty"]) : true);
	if (prop_data_empty_cnt_spin) prop_data_empty_cnt_spin->set_value(CLAMP(int(p_el.get("data_empty_count", 8)), 0, 64));
	if (prop_slot_action_opt) s_select_option_meta(prop_slot_action_opt, String(p_el.get("slot_action", "show_detail")));
	updating_ui = prev_upd;
	_on_prop_slot_style_state_selected(prop_slot_style_state_opt ? prop_slot_style_state_opt->get_selected() : 0);
}

void ClockGearsUiDesignEditor::_on_prop_container_changed() {
	if (updating_ui || selected_elem_id.is_empty()) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	if (prop_data_source_opt && prop_data_source_opt->get_selected() >= 0) {
		el["data_source"] = String(prop_data_source_opt->get_item_metadata(prop_data_source_opt->get_selected()));
	}
	if (prop_data_filter_opt && prop_data_filter_opt->get_selected() >= 0) {
		el["data_filter_category"] = String(prop_data_filter_opt->get_item_metadata(prop_data_filter_opt->get_selected()));
	}
	if (prop_data_usable_chk) el["data_only_usable"] = prop_data_usable_chk->is_pressed();
	if (prop_data_sort_opt && prop_data_sort_opt->get_selected() >= 0) {
		el["data_sort_by"] = String(prop_data_sort_opt->get_item_metadata(prop_data_sort_opt->get_selected()));
	}
	if (prop_data_equip_slot_spin) el["data_equip_slot"] = int(Math::round(prop_data_equip_slot_spin->get_value()));
	if (prop_container_layout_opt && prop_container_layout_opt->get_selected() >= 0 && String(el.get("type", "")) != "radial") {
		el["layout_mode"] = String(prop_container_layout_opt->get_item_metadata(prop_container_layout_opt->get_selected()));
	}
	if (prop_container_cols_spin) el["layout_columns"] = int(Math::round(prop_container_cols_spin->get_value()));
	if (prop_container_rows_spin) el["layout_rows"] = int(Math::round(prop_container_rows_spin->get_value()));
	if (prop_container_spx_spin) el["layout_spacing"] = int(Math::round(prop_container_spx_spin->get_value()));
	if (prop_container_spy_spin) el["layout_spacing_y"] = int(Math::round(prop_container_spy_spin->get_value()));
	if (prop_container_scroll_chk) el["layout_scroll"] = prop_container_scroll_chk->is_pressed();
	if (prop_data_show_empty_chk) el["data_show_empty"] = prop_data_show_empty_chk->is_pressed();
	if (prop_data_empty_cnt_spin) el["data_empty_count"] = int(Math::round(prop_data_empty_cnt_spin->get_value()));
	if (prop_slot_action_opt && prop_slot_action_opt->get_selected() >= 0) {
		el["slot_action"] = String(prop_slot_action_opt->get_item_metadata(prop_slot_action_opt->get_selected()));
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_slot_style_state_selected(int p_idx) {
	if (!prop_slot_style_state_opt || p_idx < 0) {
		return;
	}
	Dictionary el = _get_selected_element_copy();
	String st = String(prop_slot_style_state_opt->get_item_metadata(p_idx));
	Dictionary sstyles = (el.has("slot_styles") && el["slot_styles"].get_type() == Variant::DICTIONARY) ? Dictionary(el["slot_styles"]) : Dictionary();
	Dictionary cfg = sstyles.has(st) && sstyles[st].get_type() == Variant::DICTIONARY ? Dictionary(sstyles[st]) : Dictionary();
	bool prev_upd = updating_ui;
	updating_ui = true;
	String def_fill = (st == "selected") ? "#382e1ffa" : (st == "disabled" ? "#14171f8c" : (st == "empty" ? "#0f121a66" : "#1c212ee6"));
	String def_bord = (st == "selected") ? "#f5bc54ff" : (st == "disabled" ? "#52576166" : (st == "empty" ? "#595c6b4d" : "#8c734da6"));
	String fc = String(cfg.get("fill_color", def_fill));
	String bc = String(cfg.get("border_color", def_bord));
	if (prop_slot_style_fill_btn && Color::html_is_valid(fc)) prop_slot_style_fill_btn->set_pick_color(Color::html(fc));
	if (prop_slot_style_border_btn && Color::html_is_valid(bc)) prop_slot_style_border_btn->set_pick_color(Color::html(bc));
	if (prop_slot_style_bw_spin) prop_slot_style_bw_spin->set_value(int(cfg.get("border_width", st == "selected" ? 2 : 1)));
	updating_ui = prev_upd;
}

void ClockGearsUiDesignEditor::_on_prop_slot_style_edited() {
	if (updating_ui || selected_elem_id.is_empty() || !prop_slot_style_state_opt || prop_slot_style_state_opt->get_selected() < 0) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	String st = String(prop_slot_style_state_opt->get_item_metadata(prop_slot_style_state_opt->get_selected()));
	Dictionary sstyles = (el.has("slot_styles") && el["slot_styles"].get_type() == Variant::DICTIONARY) ? Dictionary(el["slot_styles"]).duplicate(true) : Dictionary();
	Dictionary cfg = sstyles.has(st) && sstyles[st].get_type() == Variant::DICTIONARY ? Dictionary(sstyles[st]).duplicate(true) : Dictionary();
	if (prop_slot_style_fill_btn) cfg["fill_color"] = "#" + prop_slot_style_fill_btn->get_pick_color().to_html(true);
	if (prop_slot_style_border_btn) cfg["border_color"] = "#" + prop_slot_style_border_btn->get_pick_color().to_html(true);
	if (prop_slot_style_bw_spin) cfg["border_width"] = int(Math::round(prop_slot_style_bw_spin->get_value()));
	sstyles[st] = cfg;
	el["slot_styles"] = sstyles;
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_create_item_tpl_pressed() {
	if (selected_elem_id.is_empty()) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	Array elems = get_effective_screen_elements();
	for (int i = 0; i < elems.size(); i++) {
		if (elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary d = elems[i];
		if (String(d.get("parent_id", "")) == selected_elem_id && (bool(d.get("is_item_template", false)) || String(d.get("role", "")) == "item_template")) {
			select_element(String(d.get("id", "")));
			return;
		}
	}
	String container_id = selected_elem_id;
	String tpl_id = add_child_element_of_type(container_id, "folder");
	if (tpl_id.is_empty()) {
		return;
	}
	Dictionary tpl = dm->get_ui_element(current_screen_id, tpl_id);
	tpl["name"] = "Plantilla del elemento";
	tpl["role"] = "item_template";
	tpl["is_item_template"] = true;
	tpl["width"] = 160;
	tpl["height"] = 56;
	dm->update_ui_element(current_screen_id, tpl_id, tpl, false);

	String ic_id = add_child_element_of_type(tpl_id, "image");
	if (!ic_id.is_empty()) {
		Dictionary ic = dm->get_ui_element(current_screen_id, ic_id);
		ic["name"] = "Icono objeto";
		ic["data_binding"] = "item_icon";
		ic["offset_x"] = 4; ic["offset_y"] = 8; ic["width"] = 40; ic["height"] = 40;
		dm->update_ui_element(current_screen_id, ic_id, ic, false);
	}
	String nm_id = add_child_element_of_type(tpl_id, "text");
	if (!nm_id.is_empty()) {
		Dictionary nm = dm->get_ui_element(current_screen_id, nm_id);
		nm["name"] = "Nombre objeto";
		nm["data_binding"] = "item_name";
		nm["offset_x"] = 48; nm["offset_y"] = 6; nm["width"] = 104; nm["height"] = 22; nm["font_size"] = 13;
		dm->update_ui_element(current_screen_id, nm_id, nm, false);
	}
	String qt_id = add_child_element_of_type(tpl_id, "text");
	if (!qt_id.is_empty()) {
		Dictionary qt = dm->get_ui_element(current_screen_id, qt_id);
		qt["name"] = "Cantidad objeto";
		qt["data_binding"] = "item_qty";
		qt["offset_x"] = 48; qt["offset_y"] = 30; qt["width"] = 104; qt["height"] = 18; qt["font_size"] = 11;
		dm->update_ui_element(current_screen_id, qt_id, qt, false);
	}
	Dictionary pel = dm->get_ui_element(current_screen_id, container_id);
	pel["item_template_folder_id"] = tpl_id;
	dm->update_ui_element(current_screen_id, container_id, pel, false);
	select_element(tpl_id);
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
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
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
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
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
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool prev_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, prev_custom);
	if (!prev_custom) {
		dm->push_ui_design_undo_state(current_screen_id, snap);
	}
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
		} else if (image_import_target == 20 && prop_rad_slot_icon_edit) {
			prop_rad_slot_icon_edit->set_text(final_path);
			_on_prop_radial_slot_edited();
		} else if (image_import_target >= 10 && image_import_target <= 13) {
			int si = image_import_target - 10;
			if (prop_btn_state_img_edits[si]) {
				prop_btn_state_img_edits[si]->set_text(final_path);
				_on_prop_field_changed();
			}
		} else if (prop_image_edit) {
			prop_image_edit->set_text(final_path);
			_on_prop_field_changed();
		}
	}
	image_import_target = 0;
}

String ClockGearsUiDesignEditor::drop_image_file_on_canvas(const String &p_path, const Vector2i &p_canvas_pos) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || p_path.strip_edges().is_empty()) {
		return String();
	}
	String final_path = p_path.strip_edges();
	if (owner_db) {
		final_path = owner_db->import_menu_asset_for_ui_designer(final_path, true);
	}
	if (final_path.is_empty()) {
		return String();
	}
	cached_thumbs_scanned = false;
	Vector2i pos = p_canvas_pos;
	if (pos.x < 0 || pos.y < 0) {
		pos = Vector2i(BASE_W / 2 - 96, BASE_H / 2 - 64);
	}
	String eid = add_element_at("image", pos);
	if (eid.is_empty()) {
		return String();
	}
	Dictionary el = _get_selected_element_copy();
	if (!el.is_empty()) {
		el["image_path"] = final_path;
		el["name"] = final_path.get_file().get_basename();
		Ref<Texture2D> tex;
		if (ResourceLoader::exists(final_path)) {
			tex = ResourceLoader::load(final_path);
		}
		if (tex.is_null()) {
			Ref<Image> im = Image::load_from_file(final_path);
			if (im.is_valid() && !im->is_empty()) {
				el["width"] = MIN(im->get_width(), BASE_W - 40);
				el["height"] = MIN(im->get_height(), BASE_H - 40);
			}
		} else {
			Size2i tsz = Size2i(tex->get_size());
			if (tsz.x >= 8 && tsz.y >= 8) {
				el["width"] = MIN(tsz.x, BASE_W - 40);
				el["height"] = MIN(tsz.y, BASE_H - 40);
			}
		}
		dm->update_ui_element(current_screen_id, eid, el, false);
		_notify_data_mutated(true);
		_trace_log("drop_image_canvas", eid + " -> " + final_path);
	}
	return eid;
}

void ClockGearsUiDesignEditor::_on_window_files_dropped(const Vector<String> &p_files) {
	if (!is_visible_in_tree() || p_files.is_empty()) {
		return;
	}
	if (canvas_overlay) {
		Vector2 mp = canvas_overlay->get_local_mouse_position();
		Rect2 r(Vector2(), canvas_overlay->get_size());
		if (!r.has_point(mp)) {
			return;
		}
		Vector2i cpos(int(Math::round(mp.x / MAX(0.05f, canvas_zoom))), int(Math::round(mp.y / MAX(0.05f, canvas_zoom))));
		for (int i = 0; i < p_files.size(); i++) {
			String ext = p_files[i].get_extension().to_lower();
			if (ext == "png" || ext == "webp" || ext == "jpg" || ext == "jpeg" || ext == "svg") {
				(void)drop_image_file_on_canvas(p_files[i], cpos + Vector2i(i * 16, i * 16));
			}
		}
	}
}

void ClockGearsUiDesignEditor::_on_prop_edit_style_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	String sid;
	if (prop_style_opt && prop_style_opt->get_selected() >= 0) {
		sid = String(prop_style_opt->get_item_metadata(prop_style_opt->get_selected()));
	}
	if (sid.is_empty() || sid == "none") {
		Array styles = dm->get_window_styles();
		if (!styles.is_empty() && styles[0].get_type() == Variant::DICTIONARY) {
			sid = String(Dictionary(styles[0]).get("id", "classic"));
		} else {
			sid = "classic";
		}
	}
	Dictionary st = dm->get_window_style(sid);
	if (st.is_empty()) {
		return;
	}
	style_editing_id = sid;
	if (!style_inline_dialog) {
		style_inline_dialog = memnew(AcceptDialog);
		style_inline_dialog->set_name("UiInlineStyleDialog");
		style_inline_dialog->set_title(String::utf8("Editar estilo de marco"));
		style_inline_dialog->set_min_size(Size2i(int(760 * EDSCALE), int(560 * EDSCALE)));
		style_inline_dialog->get_ok_button()->set_text("Aplicar al estilo");
		style_inline_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_inline_style_applied));
		ScrollContainer *st_sc = memnew(ScrollContainer);
		st_sc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		st_sc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		st_sc->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		style_inline_dialog->add_child(st_sc);
		VBoxContainer *vb = memnew(VBoxContainer);
		vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		vb->add_theme_constant_override("separation", int(8 * EDSCALE));
		st_sc->add_child(vb);

		auto add_row = [&](const String &p_lbl, Control *p_ctrl) {
			HBoxContainer *hb = memnew(HBoxContainer);
			hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			hb->add_theme_constant_override("separation", int(8 * EDSCALE));
			Label *l = memnew(Label(p_lbl));
			l->set_custom_minimum_size(Size2(130 * EDSCALE, 0));
			hb->add_child(l);
			p_ctrl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			hb->add_child(p_ctrl);
			vb->add_child(hb);
		};
		style_edit_name = memnew(LineEdit);
		add_row("Nombre:", style_edit_name);
		style_edit_fill_col = memnew(ColorPickerButton);
		style_edit_fill_col->set_edit_alpha(true);
		add_row("Color fondo:", style_edit_fill_col);
		style_edit_border_col = memnew(ColorPickerButton);
		style_edit_border_col->set_edit_alpha(true);
		add_row("Color borde:", style_edit_border_col);
		style_edit_border_w = memnew(SpinBox);
		style_edit_border_w->set_min(0); style_edit_border_w->set_max(16);
		add_row("Grosor borde:", style_edit_border_w);
		style_edit_radius = memnew(SpinBox);
		style_edit_radius->set_min(0); style_edit_radius->set_max(64);
		add_row("Redondeo:", style_edit_radius);
		style_edit_pad = memnew(SpinBox);
		style_edit_pad->set_min(0); style_edit_pad->set_max(48);
		add_row("Margen interno:", style_edit_pad);
		style_edit_opacity = memnew(HSlider);
		style_edit_opacity->set_min(0.05); style_edit_opacity->set_max(1.0); style_edit_opacity->set_step(0.05);
		add_row("Opacidad:", style_edit_opacity);
		add_child(style_inline_dialog);
	}
	if (style_edit_name) style_edit_name->set_text(String(st.get("name", sid)));
	String fhex = String(st.get("fill_color", "#141824ff"));
	String bhex = String(st.get("border_color", "#e0a040ff"));
	Color fcol = Color::html_is_valid(fhex) ? Color::html(fhex) : Color(0.08f, 0.09f, 0.14f, 0.94f);
	if (style_edit_fill_col) style_edit_fill_col->set_pick_color(fcol);
	if (style_edit_border_col && Color::html_is_valid(bhex)) style_edit_border_col->set_pick_color(Color::html(bhex));
	if (style_edit_border_w) style_edit_border_w->set_value(int(st.get("border_width", 2)));
	if (style_edit_radius) style_edit_radius->set_value(int(st.get("corner_radius", 8)));
	int pad_v = 12;
	if (st.get("padding", Variant()).get_type() == Variant::DICTIONARY) {
		pad_v = int(Dictionary(st["padding"]).get("left", 12));
	} else {
		pad_v = int(st.get("padding", 12));
	}
	if (style_edit_pad) style_edit_pad->set_value(pad_v);
	if (style_edit_opacity) style_edit_opacity->set_value(fcol.a);
	ClockGearsTheme::popup_fitted_window(
			style_inline_dialog,
			Size2i(int(760 * EDSCALE), int(560 * EDSCALE)),
			Size2i(int(760 * EDSCALE), int(560 * EDSCALE)),
			"ui_inline_style");
}

void ClockGearsUiDesignEditor::_on_inline_style_applied() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || style_editing_id.is_empty()) {
		return;
	}
	Dictionary patch = dm->get_window_style(style_editing_id).duplicate(true);
	if (style_edit_name && !style_edit_name->get_text().strip_edges().is_empty()) {
		patch["name"] = style_edit_name->get_text().strip_edges();
	}
	if (style_edit_fill_col) {
		Color fc = style_edit_fill_col->get_pick_color();
		if (style_edit_opacity) {
			fc.a = float(CLAMP(style_edit_opacity->get_value(), 0.05, 1.0));
		}
		patch["fill_color"] = "#" + fc.to_html(true);
	}
	if (style_edit_border_col) patch["border_color"] = "#" + style_edit_border_col->get_pick_color().to_html(true);
	int bw = style_edit_border_w ? int(Math::round(style_edit_border_w->get_value())) : int(patch.get("border_width", 2));
	int rad = style_edit_radius ? int(Math::round(style_edit_radius->get_value())) : int(patch.get("corner_radius", 8));
	patch["border_width"] = bw;
	patch["corner_radius"] = rad;
	if (rad > 0) {
		patch["border_type"] = "rounded";
	} else if (bw > 0) {
		patch["border_type"] = "rect";
	}
	if (style_edit_pad) {
		int p = int(Math::round(style_edit_pad->get_value()));
		Dictionary pad_d;
		pad_d["left"] = p; pad_d["top"] = p; pad_d["right"] = p; pad_d["bottom"] = p;
		patch["padding"] = pad_d;
	}
	dm->update_window_style(style_editing_id, patch);
	cached_opts_styles_cnt = -1;
	style_thumb_cache.clear();
	_refresh_style_binding_action_options();
	if (!selected_elem_id.is_empty()) {
		Dictionary el = _get_selected_element_copy();
		if (!el.is_empty() && String(el.get("window_style_id", "")).is_empty()) {
			el["window_style_id"] = style_editing_id;
			dm->update_ui_element(current_screen_id, selected_elem_id, el, false);
		}
	}
	_notify_data_mutated(true);
	_trace_log("inline_style_applied", style_editing_id);
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
	if (test_gmenu) {
		if (test_gmenu->is_menu_open()) {
			test_gmenu->close_menu();
		}
		test_gmenu = nullptr;
	}
	if (test_credits) {
		test_credits->stop_credits(true);
		test_credits = nullptr;
	}
	test_title = nullptr;
	test_hud = nullptr;
	if (test_window) {
		test_window->hide();
		test_window->queue_free();
		test_window = nullptr;
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
	test_window->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	test_window->set_title(String::utf8("Probar interfaz (") + itos(test_sz.x) + "x" + itos(test_sz.y) + String::utf8(", Esc para cerrar)"));
	test_window->set_size(test_sz);
	test_window->set_content_scale_size(Size2i(BASE_W, BASE_H));
	test_window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
	test_window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
	test_window->set_transient(true);
	test_window->set_exclusive(false);
	test_window->connect("close_requested", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_close_requested), CONNECT_DEFERRED);
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
	} else if (current_screen_id == "messages" || current_screen_id.begins_with("messages_") || current_screen_id == "battle" || current_screen_id.begins_with("battle_") || (current_screen_id == "game_over" && !dm->has_custom_ui_design("game_over"))) {
		String scope_id = dm->get_menu_system_group(current_screen_id);
		if (scope_id.is_empty()) scope_id = current_screen_id;
		ColorRect *bg = memnew(ColorRect);
		bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(false, false, 1.0f);
		bg->set_color(tk.surface_base);
		test_window->add_child(bg);

		if (scope_id == "battle") {
			UiBattleBackdropPreview *bd = memnew(UiBattleBackdropPreview);
			bd->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			bd->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
			test_window->add_child(bd);
		}

		Control *comp_root = memnew(Control);
		comp_root->set_name("TestUiIsolatedLayer");
		comp_root->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		comp_root->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		test_window->add_child(comp_root);

		if (scope_id == "messages" || scope_id == "battle") {
			Array all_m = dm->get_all_menus_for_selector();
			for (int i = 0; i < all_m.size(); i++) {
				Dictionary md = all_m[i];
				String msid = String(md.get("id", ""));
				String msg_grp = String(md.get("system_group", dm->get_menu_system_group(msid)));
				if (msid != scope_id && msg_grp != scope_id) continue;
				// En Probar completo para Batalla o Mensajes mostramos las sub-pantallas clave juntas sin solapar submenús modales sobre diálogo
				ClockGearsUiDesignLayer *layer = memnew(ClockGearsUiDesignLayer);
				layer->set_name("TestSub_" + msid);
				layer->set_preview_mode(false);
				layer->set_window_type_fallback(scope_id == "messages" ? "message" : "battle_commands");
				layer->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
				layer->set_use_editor_test_data(dm->get_editor_test_data_count() >= 0);
				Dictionary d = dm->has_custom_ui_design(msid) ? dm->get_ui_screen_design(msid) : dm->get_default_ui_screen_template(msid);
				layer->apply_prebuilt_screen_design(msid, d);
				layer->connect("action_triggered", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_close_requested).unbind(3), CONNECT_DEFERRED);
				comp_root->add_child(layer);
			}
		} else {
			ClockGearsUiDesignLayer *layer = memnew(ClockGearsUiDesignLayer);
			layer->set_name("TestUiIsolatedLayerGameOver");
			layer->set_preview_mode(false);
			layer->set_window_type_fallback("game_menu");
			layer->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
			layer->set_use_editor_test_data(dm->get_editor_test_data_count() >= 0);
			Dictionary d = dm->has_custom_ui_design(current_screen_id)
					? dm->get_ui_screen_design(current_screen_id)
					: dm->get_default_ui_screen_template(current_screen_id);
			layer->apply_prebuilt_screen_design(current_screen_id, d);
			layer->connect("action_triggered", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_close_requested).unbind(3), CONNECT_DEFERRED);
			comp_root->add_child(layer);
		}
	} else {
		test_gmenu = memnew(ClockGearsGameMenu);
		test_gmenu->set_editor_test_popup_mode(true);
		if (test_gmenu->get_custom_ui_layer()) {
			test_gmenu->get_custom_ui_layer()->set_use_editor_test_data(dm->get_editor_test_data_count() >= 0);
		}
		test_window->add_child(test_gmenu);
		test_gmenu->open_menu_by_id(current_screen_id, true, false, true);
		test_gmenu->connect("menu_closed", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_close_requested), CONNECT_DEFERRED);
	}

	Button *close_btn = memnew(Button("Cerrar"));
	close_btn->set_name("UiTestCloseButton");
	close_btn->set_tooltip_text(String::utf8("Cierra la vista de prueba (Esc)."));
	close_btn->set_anchors_preset(Control::PRESET_TOP_RIGHT);
	close_btn->set_offset(SIDE_LEFT, -84);
	close_btn->set_offset(SIDE_TOP, 8);
	close_btn->set_offset(SIDE_RIGHT, -8);
	close_btn->set_offset(SIDE_BOTTOM, 34);
	close_btn->set_z_index(100);
	s_make_rounded_action_btn(close_btn);
	close_btn->set_clip_text(false);
	Ref<InputEventKey> esc_ev;
	esc_ev.instantiate();
	esc_ev->set_keycode(Key::ESCAPE);
	Ref<Shortcut> esc_sc;
	esc_sc.instantiate();
	Array esc_arr;
	esc_arr.push_back(esc_ev);
	esc_sc->set_events(esc_arr);
	close_btn->set_shortcut(esc_sc);
	close_btn->set_shortcut_in_tooltip(false);
	close_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_close_requested), CONNECT_DEFERRED);
	test_window->add_child(close_btn);

	test_window->popup_centered(Size2i(MIN(test_sz.x, 1280), MIN(test_sz.y, 720)));
	return test_window;
}

Rect2 ClockGearsUiDesignEditor::get_overlay_element_rect(const String &p_elem_id) const {
	return canvas_overlay ? canvas_overlay->get_element_widget_rect(p_elem_id) : Rect2();
}

Rect2 ClockGearsUiDesignEditor::get_overlay_selected_rect() const {
	return canvas_overlay ? canvas_overlay->get_selected_widget_rect() : Rect2();
}

Rect2 ClockGearsUiDesignEditor::get_overlay_handle_rect(int p_handle) const {
	return canvas_overlay ? canvas_overlay->get_handle_widget_rect(p_handle) : Rect2();
}

Rect2 ClockGearsUiDesignEditor::get_rendered_element_stage_rect(const String &p_elem_id) const {
	if (!design_layer || p_elem_id.is_empty()) {
		return Rect2();
	}
	Control *ctl = design_layer->get_element_control(p_elem_id);
	if (!ctl) {
		return Rect2();
	}
	Vector2 pos;
	Control *cur = ctl;
	int guard = 0;
	while (cur && cur != design_layer && guard < 32) {
		pos += cur->get_position();
		cur = cur->get_parent_control();
		guard++;
	}
	if (cur != design_layer) {
		return Rect2();
	}
	return Rect2(pos, ctl->get_size());
}

Dictionary ClockGearsUiDesignEditor::get_ui_handles() const {
	Dictionary out;
	out["panel"] = this;
	out["screen_opt"] = screen_opt;
	out["system_scope_root"] = system_scope_root;
	out["new_menu_wizard_btn"] = new_menu_wizard_btn;
	out["add_submenu_btn"] = add_submenu_btn;
	out["add_button_bar_btn"] = add_button_bar_btn;
	out["templates_btn"] = templates_btn;
	out["menus_list_vbox"] = menus_list_vbox;
	out["menu_wizard_dialog"] = menu_wizard_dialog;
	out["wizard_tpl_grid"] = wizard_tpl_grid;
	out["wizard_name_edit"] = wizard_name_edit;
	out["wizard_next_btn"] = wizard_next_btn;
	out["wizard_finish_btn"] = wizard_finish_btn;
	out["template_gallery_dialog"] = template_gallery_dialog;
	out["tpl_close_x_btn"] = tpl_close_x_btn;
	out["template_tabs"] = template_tabs;
	out["tpl_screen_grid"] = tpl_screen_grid;
	out["tpl_comp_grid"] = tpl_comp_grid;
	out["tpl_user_grid"] = tpl_user_grid;
	out["tpl_apply_btn"] = tpl_apply_btn;
	out["template_apply_confirm_dialog"] = template_apply_confirm_dialog;
	out["props_mode_simple_btn"] = props_mode_simple_btn;
	out["props_mode_adv_btn"] = props_mode_adv_btn;
	out["props_search_edit"] = props_search_edit;
	out["prop_shape_opt"] = prop_shape_opt;
	out["prop_radius_slider"] = prop_radius_slider;
	out["prop_opacity_slider"] = prop_opacity_slider;
	out["prop_fill_color_btn"] = prop_fill_color_btn;
	out["prop_grad_chk"] = prop_grad_chk;
	out["prop_grad_top_btn"] = prop_grad_top_btn;
	out["prop_grad_bot_btn"] = prop_grad_bot_btn;
	out["prop_border_w_slider"] = prop_border_w_slider;
	out["prop_border_col_btn"] = prop_border_col_btn;
	out["prop_shadow_chk"] = prop_shadow_chk;
	out["prop_glow_chk"] = prop_glow_chk;
	out["prop_glow_col_btn"] = prop_glow_col_btn;
	out["prop_clip_chk"] = prop_clip_chk;
	out["prop_show_bg_chk"] = prop_show_bg_chk;
	out["prop_font_opt"] = prop_font_opt;
	out["prop_font_size_slider"] = prop_font_size_slider;
	out["prop_font_color_btn"] = prop_font_color_btn;
	out["prop_text_row"] = prop_text_row;
	out["prop_image_row"] = prop_image_row;
	out["prop_action_row"] = prop_action_row;
	out["prop_binding_row"] = prop_binding_row;
	out["menu_props_box"] = menu_props_box;
	out["zoom_1_btn"] = zoom_btns[0];
	out["zoom_2_btn"] = zoom_btns[1];
	out["zoom_3_btn"] = zoom_btns[2];
	out["zoom_50_btn"] = zoom_btns[3];
	out["zoom_300_btn"] = zoom_btns[4];
	out["undo_btn"] = undo_btn;
	out["redo_btn"] = redo_btn;
	out["reset_btn"] = reset_btn;
	out["reset_confirm_dialog"] = reset_confirm_dialog;
	out["test_btn"] = test_btn;
	out["top_card"] = top_card;
	out["top_row1_flow"] = top_row1_flow;
	out["top_row2_flow"] = top_row2_flow;
	out["screen_status_lbl"] = screen_status_lbl;
	out["context_btn"] = context_btn;
	out["context_popover"] = context_popover;
	out["context_opacity_slider"] = context_opacity_slider;
	out["context_siblings_list"] = context_siblings_list;
	out["context_all_btn"] = context_all_btn;
	out["context_none_btn"] = context_none_btn;
	out["sample_scene_opacity_slider"] = sample_scene_opacity_slider;
	out["view_context_chk"] = view_context_chk;
	out["sample_scene_chk"] = sample_scene_chk;
	out["context_layers_root"] = context_layers_root;
	out["ctx_battle_backdrop_node"] = ctx_battle_backdrop_node;
	out["canvas_card"] = canvas_card;
	out["wide_left_col"] = wide_left_col;
	out["left_col_scroll"] = left_col_scroll;
	out["wide_hbox"] = wide_hbox;
	out["wide_right_split"] = wide_right_split;
	out["wide_center_col"] = wide_center_col;
	out["wide_right_col"] = wide_right_col;
	out["props_card"] = props_card;
	out["props_fields_box"] = props_fields_box;
	out["canvas_aspect"] = canvas_aspect;
	out["canvas_scroll"] = canvas_scroll;
	out["canvas_fit_holder"] = canvas_fit_holder;
	out["canvas_stage"] = canvas_stage;
	out["design_layer"] = design_layer;
	out["canvas_overlay"] = canvas_overlay;
	out["menus_card"] = menus_card;
	out["left_elements_hdr"] = menus_card ? menus_card->find_child("LeftElementsHeaderBtn", true, false) : nullptr;
	out["design_cards_scroll"] = design_cards_scroll;
	out["design_cards_vbox"] = design_cards_vbox;
	out["layer_context_menu"] = layer_context_menu;
	out["add_child_submenu"] = add_child_submenu;
	out["reparent_submenu"] = reparent_submenu;
	out["layers_card"] = layers_card;
	out["layers_tree"] = layers_tree;
	out["layer_add_button_btn"] = layer_add_button_btn;
	out["layer_add_folder_btn"] = layer_add_folder_btn;
	out["folder_prompt_box"] = folder_prompt_box;
	out["folder_name_edit"] = folder_name_edit;
	out["folder_confirm_btn"] = folder_confirm_btn;
	out["folder_cancel_btn"] = folder_cancel_btn;
	out["layer_group_btn"] = layer_group_btn;
	out["layer_ungroup_btn"] = layer_ungroup_btn;
	out["layers_list_vbox"] = layers_list_vbox;
	out["layer_up_btn"] = layer_up_btn;
	out["layer_down_btn"] = layer_down_btn;
	out["layer_del_btn"] = layer_del_btn;
	out["align_center_h_btn"] = align_center_h_btn;
	out["align_center_v_btn"] = align_center_v_btn;
	out["dist_v_btn"] = dist_v_btn;
	out["dup_elem_btn"] = dup_elem_btn;
	out["copy_elem_btn"] = copy_elem_btn;
	out["paste_elem_btn"] = paste_elem_btn;
	out["menu_dup_btn"] = menu_dup_btn;
	out["menu_up_btn"] = menu_up_btn;
	out["menu_down_btn"] = menu_down_btn;
	out["menu_del_btn"] = menu_del_btn;
	out["prop_name_edit"] = prop_name_edit;
	out["prop_ox_spin"] = prop_ox_spin;
	out["prop_oy_spin"] = prop_oy_spin;
	out["prop_w_spin"] = prop_w_spin;
	out["prop_h_spin"] = prop_h_spin;
	out["prop_layer_spin"] = prop_layer_spin;
	out["prop_visible_chk"] = prop_visible_chk;
	out["prop_locked_chk"] = prop_locked_chk;
	out["prop_clip_children_chk"] = prop_clip_children_chk;
	out["prop_parent_layout_opt"] = prop_parent_layout_opt;
	out["delete_parent_dialog"] = delete_parent_dialog;
	out["grid_step_opt"] = grid_step_opt;
	out["prop_style_opt"] = prop_style_opt;
	out["prop_edit_style_btn"] = prop_edit_style_btn;
	out["style_inline_dialog"] = style_inline_dialog;
	out["style_edit_name"] = style_edit_name;
	out["style_edit_fill_col"] = style_edit_fill_col;
	out["style_edit_border_col"] = style_edit_border_col;
	out["style_edit_border_w"] = style_edit_border_w;
	out["style_edit_radius"] = style_edit_radius;
	out["style_edit_pad"] = style_edit_pad;
	out["style_edit_opacity"] = style_edit_opacity;
	out["prop_text_edit"] = prop_text_edit;
	out["prop_image_edit"] = prop_image_edit;
	out["prop_image_scale_opt"] = prop_image_scale_opt;
	out["prop_image_adjust_manual_btn"] = prop_image_adjust_manual_btn;
	out["prop_nine_slice_spin"] = prop_nine_slice_spin;
	out["prop_pixel_art_chk"] = prop_pixel_art_chk;
	out["menu_bg_type_opt"] = menu_bg_type_opt;
	out["menu_bg_img_edit"] = menu_bg_img_edit;
	out["menu_bg_scale_opt"] = menu_bg_scale_opt;
	out["menu_bg_adjust_manual_btn"] = menu_bg_adjust_manual_btn;
	out["image_adjust_dialog"] = image_adjust_dialog;
	out["menu_bg_pixel_chk"] = menu_bg_pixel_chk;
	out["menu_bg_blur_chk"] = menu_bg_blur_chk;
	out["menu_bg_blur_slider"] = menu_bg_blur_slider;
	out["menu_bg_veil_chk"] = menu_bg_veil_chk;
	out["menu_bg_veil_col_btn"] = menu_bg_veil_col_btn;
	out["menu_bg_veil_opacity_slider"] = menu_bg_veil_opacity_slider;
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
	out["screens_tree"] = screens_tree;
	out["screen_settings_box"] = screen_settings_box;
	out["screen_settings_body"] = screen_settings_body;
	out["prop_add_action_opt"] = prop_add_action_opt;
	out["prop_add_action_btn"] = prop_add_action_btn;
	out["prop_multi_actions_vbox"] = prop_multi_actions_vbox;
	out["prop_sound_hover_edit"] = prop_sound_hover_edit;
	out["prop_sound_click_edit"] = prop_sound_click_edit;
	out["prop_vis_mode_opt"] = prop_vis_mode_opt;
	out["prop_vis_sw_edit"] = prop_vis_sw_edit;
	out["prop_en_mode_opt"] = prop_en_mode_opt;
	out["prop_en_sw_edit"] = prop_en_sw_edit;
	out["prop_btn_shortcut_lbl"] = prop_btn_shortcut_lbl;
	out["prop_btn_shortcut_capture_btn"] = prop_btn_shortcut_capture_btn;
	Array btn_state_arr;
	for (int si = 0; si < 4; si++) {
		btn_state_arr.push_back(prop_btn_state_img_edits[si]);
	}
	out["prop_btn_state_img_edits"] = btn_state_arr;
	out["prop_radial_box"] = prop_radial_box;
	out["prop_rad_count_slider"] = prop_rad_count_slider;
	out["prop_rad_radius_slider"] = prop_rad_radius_slider;
	out["prop_rad_start_slider"] = prop_rad_start_slider;
	out["prop_rad_full_chk"] = prop_rad_full_chk;
	out["prop_rad_sweep_slider"] = prop_rad_sweep_slider;
	out["prop_rad_cw_chk"] = prop_rad_cw_chk;
	out["prop_rad_slot_size_slider"] = prop_rad_slot_size_slider;
	out["prop_rad_center_info_chk"] = prop_rad_center_info_chk;
	out["prop_rad_open_anim_opt"] = prop_rad_open_anim_opt;
	out["prop_rad_slot_opt"] = prop_rad_slot_opt;
	out["prop_rad_add_slot_btn"] = prop_rad_add_slot_btn;
	out["prop_rad_del_slot_btn"] = prop_rad_del_slot_btn;
	out["prop_rad_slot_name_edit"] = prop_rad_slot_name_edit;
	out["prop_rad_slot_desc_edit"] = prop_rad_slot_desc_edit;
	out["prop_rad_slot_icon_edit"] = prop_rad_slot_icon_edit;
	out["prop_rad_slot_add_act_opt"] = prop_rad_slot_add_act_opt;
	out["prop_rad_slot_add_act_btn"] = prop_rad_slot_add_act_btn;
	out["prop_rad_slot_actions_vbox"] = prop_rad_slot_actions_vbox;
	out["test_data_opt"] = test_data_opt;
	out["prop_rotatable_chk"] = prop_rotatable_chk;
	out["prop_container_box"] = prop_container_box;
	out["prop_data_source_opt"] = prop_data_source_opt;
	out["prop_data_filter_opt"] = prop_data_filter_opt;
	out["prop_data_usable_chk"] = prop_data_usable_chk;
	out["prop_data_sort_opt"] = prop_data_sort_opt;
	out["prop_data_equip_slot_spin"] = prop_data_equip_slot_spin;
	out["prop_container_layout_opt"] = prop_container_layout_opt;
	out["prop_container_cols_spin"] = prop_container_cols_spin;
	out["prop_container_rows_spin"] = prop_container_rows_spin;
	out["prop_container_scroll_chk"] = prop_container_scroll_chk;
	out["prop_data_show_empty_chk"] = prop_data_show_empty_chk;
	out["prop_data_empty_cnt_spin"] = prop_data_empty_cnt_spin;
	out["prop_slot_action_opt"] = prop_slot_action_opt;
	out["prop_create_item_tpl_btn"] = prop_create_item_tpl_btn;
	out["prop_slot_style_state_opt"] = prop_slot_style_state_opt;
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

String ClockGearsUiDesignEditor::get_screen_long_help(const String &p_screen_id, const String &p_screen_name) {
	static const struct {
		const char *id;
		const char *help;
	} k_helps[] = {
		{ "title", "Menú principal\nPantalla de título al iniciar el juego: configura el logotipo, fondo animado o imagen y los botones principales como Continuar o Nueva partida." },
		{ "game_menu_options", "Menú Opciones\nPantalla de ajustes donde el jugador regula el volumen, la resolución, pantalla completa y controles." },
		{ "game_menu_save", "Guardar / Cargar\nVentana con la lista de ranuras de partida para guardar el progreso actual o cargar un archivo existente." },
		{ "game_menu_main", "Menú del juego\nMenú general al abrir la pausa durante la partida, con resumen del grupo, dinero y accesos al resto de submenús." },
		{ "game_menu_items", "Inventario\nPantalla de objetos divididos por categorías para usar consumibles o inspeccionar objetos clave." },
		{ "game_menu_equip", "Equipo\nInterfaz para equipar armas, armaduras y accesorios comparando cambios de estadísticas en el personaje." },
		{ "game_menu_status", "Estado\nFicha de atributos detallados del personaje, experiencia, habilidades aprendidas y resistencias." },
		{ "game_over", "Game Over\nPantalla mostrada al caer derrotado en batalla, con opciones para reintentar, cargar o volver al título." },
		{ "credits", "Créditos\nSecuencia desplazable con agradecimientos, reparto del proyecto, logotipos y música de cierre." },
		{ "hud", "HUD (Juego)\nBarras, medidores y contadores flotantes visibles durante el mapa o el combate en tiempo real." },
		{ "messages", "Cuadro de diálogo\nVentana principal inferior donde se muestran las líneas de diálogo, nombre del hablante y retrato." },
		{ "messages_choices", "Lista de elecciones\nVentana emergente con opciones múltiples para que el jugador decida respuestas o acciones en eventos." },
		{ "messages_number", "Entrada de número\nSelector numérico dígito a dígito utilizado para introducir códigos, precios o cantidades." },
		{ "messages_history", "Historial de texto\nRegistro desplazable que permite releer conversaciones y avisos recientes de la partida." },
		{ "messages_gold", "Ventana de dinero\nRecuadro auxiliar en esquina superior que muestra el dinero actual durante tiendas o pagos." },
		{ "battle", "Comandos de batalla\nPanel de acciones del personaje activo en su turno: Atacar, Habilidades, Objetos, Defender o Huir." },
		{ "battle_party", "Panel del grupo\nBarra inferior con los héroes del equipo activo, sus puntos de vida (PV), maná (PM) y estado en combate." },
		{ "battle_enemies", "Zona de enemigos\nÁrea visual de referencia donde el motor coloca libremente los sprites de los enemigos sin marco de ventana." },
		{ "battle_notice", "Avisos de batalla\nVentana empleada para mensajes rápidos de combate, turnos especiales y resumen de victoria o botín." },
		{ "battle_submenu", "Submenú de habilidades / objetos\nLista desplegada al escoger Habilidades u Objetos para elegir la técnica o consumible a usar." },
		{ "battle_target", "Selector de objetivo\nGuía visual sobre el enemigo o aliado seleccionado como blanco antes de ejecutar la acción." }
	};
	for (const auto &entry : k_helps) {
		if (p_screen_id == entry.id) {
			return String::utf8(entry.help);
		}
	}
	if (!p_screen_id.is_empty() && ClockGearsDataManager::get_singleton()) {
		Dictionary scr = ClockGearsDataManager::get_singleton()->get_ui_screen_design(p_screen_id);
		String desc = String(scr.get("description", "")).strip_edges();
		String nm = p_screen_name.is_empty() ? String(scr.get("name", p_screen_id)) : p_screen_name;
		if (!desc.is_empty()) {
			return nm + "\n" + desc;
		}
		if (String(scr.get("screen_type", "")) == "submenu") {
			return nm + String::utf8("\nSubmenú personalizado anidado bajo la pantalla padre para mostrar vistas o comandos propios.");
		}
	}
	String label = p_screen_name.is_empty() ? p_screen_id : p_screen_name;
	return label + String::utf8("\nDiseño de pantalla seleccionable en el lienzo. Elige una opción para editar sus paneles, botones y estilo.");
}

String ClockGearsUiDesignEditor::get_section_long_help(const String &p_section_title) {
	static const struct {
		const char *title;
		const char *help;
	} k_secs[] = {
		{ "Diseños", "Panel de elementos de esta pantalla\nLista todos los paneles, botones, textos y carpetas ordenados por capa para seleccionar, ocultar o reorganizar." },
		{ "Elementos", "Panel de elementos\nLista los paneles, botones, imágenes, textos y carpetas de esta pantalla; haz clic para editar o arrastra para reordenar capas." },
		{ "Elementos (arrastrar)", "Paleta de componentes\nArrastra cualquier botón, caja, texto o imagen directamente hacia el lienzo para añadirlo en la posición exacta." },
		{ "Propiedades", "Inspector de propiedades\nEdita las medidas, posición, texto, imagen, estilo de ventana y acción del elemento o pantalla seleccionados." },
		{ "Ajustes de la pantalla", "Ajustes generales de la pantalla\nConfigura el nombre, dimensiones base, estilo global por defecto y comportamiento general de este diseño." },
		{ "Fondo y transición del menú", "Fondo y transición\nDefine la imagen o color de fondo, desenfoque opcional y animación al abrir o cerrar esta pantalla." },
		{ "Apertura y distribución (3B/3E)", "Apertura y atajos del menú\nConfigura cómo se distribuyen automáticamente los elementos y con qué teclas o botones se abre o cierra." },
		{ "Sonidos del menú", "Efectos de sonido del menú\nAsigna clips de audio al mover el cursor, confirmar una opción o cancelar en esta pantalla." },
		{ "Identidad y jerarquía", "Identidad y jerarquía\nCambia el nombre visible, la carpeta contenedora y el orden de profundidad de este elemento." },
		{ "Posición y tamaño", "Posición y tamaño\nAjusta con precisión numérica las coordenadas X/Y, el anclaje y el ancho/alto sobre el lienzo." },
		{ "Estilo y Apariencia", "Estilo y apariencia\nPersonaliza el marco 9-slice, bordes, radios, opacidades y colores de fondo del elemento." },
		{ "Imagen", "Propiedades de imagen\nSelecciona la textura, el modo de ajuste o recorte manual y la nitidez píxel a píxel." },
		{ "Texto y fuente", "Texto y tipografía\nEdita el texto mostrado, la fuente tipográfica, el tamaño de letra, color y alineación." },
		{ "Acción al pulsar", "Acción del botón\nDefine qué ocurre cuando el jugador pulsa este control: abrir otro menú, iniciar escena o ejecutar acción." }
	};
	for (const auto &s : k_secs) {
		if (p_section_title == String::utf8(s.title)) {
			return String::utf8(s.help);
		}
	}
	return p_section_title + String::utf8("\nSección desplegable de configuración del diseño actual; haz clic en la cabecera para expandirla o contraerla.");
}

bool ClockGearsUiDesignEditor::resolve_long_help_target(Window *p_host_parent, const Vector2 &p_pointer_in_host, ClockGearsLongHelpController::Target &r_target) const {
	// 1. Prioridad máxima: desplegable "Pantalla" abierto (no cerrarlo ni tapar las opciones).
	if (screen_opt && screen_opt->get_popup() && screen_opt->get_popup()->is_visible()) {
		PopupMenu *pop = screen_opt->get_popup();
		const Rect2 pop_rect = ClockGearsLongHelpController::get_popup_global_rect_in_host(pop, p_host_parent);
		const int cnt = pop->get_item_count();
		int idx = -1;
		if (cnt > 0 && pop_rect.has_point(p_pointer_in_host)) {
			const float rel_y = CLAMP((p_pointer_in_host.y - pop_rect.position.y) / MAX(1.0f, pop_rect.size.y), 0.0f, 0.9999f);
			idx = CLAMP(int(rel_y * float(cnt)), 0, cnt - 1);
		} else {
			idx = pop->get_focused_item();
		}
		if (idx >= 0 && idx < cnt && !pop->is_item_separator(idx)) {
			const String sid = String(pop->get_item_metadata(idx));
			const String sname = pop->get_item_text(idx).trim_prefix(String::utf8("  ↳ "));
			const float row_h = pop_rect.size.y / MAX(1.0f, float(cnt));
			r_target.key = "screen_popup:" + sid + ":" + itos(idx);
			r_target.text = get_screen_long_help(sid, sname);
			r_target.anchor_global_rect = Rect2(pop_rect.position.x, pop_rect.position.y + float(idx) * row_h, pop_rect.size.x, row_h);
			r_target.avoid_global_rect = pop_rect;
			r_target.prefer_beside = true;
		}
		return true;
	}

	// 2. Diálogo de plantillas visible.
	if (template_gallery_dialog && template_gallery_dialog->is_visible()) {
		const Vector2 local_pt = ClockGearsLongHelpController::host_point_to_window_local(template_gallery_dialog, p_host_parent, p_pointer_in_host);
		if (Control *hit = ClockGearsLongHelpController::find_long_help_control_in_window(template_gallery_dialog, local_pt)) {
			const String txt = ClockGearsTheme::get_control_long_help(hit);
			if (!txt.is_empty()) {
				r_target.key = "tpl_ctrl:" + itos(hit->get_instance_id());
				r_target.text = txt;
				r_target.anchor_global_rect = ClockGearsLongHelpController::get_control_global_rect_in_host(hit, p_host_parent);
				r_target.prefer_beside = false;
				return true;
			}
		}
		return true;
	}

	// 3. Editor propio dentro de su ventana.
	Window *win = get_window();
	if (win && win->is_visible()) {
		const Vector2 local_pt = ClockGearsLongHelpController::host_point_to_window_local(win, p_host_parent, p_pointer_in_host);
		if (Control *hit = ClockGearsLongHelpController::find_long_help_control_in_window(win, local_pt)) {
			const String txt = ClockGearsTheme::get_control_long_help(hit);
			if (!txt.is_empty()) {
				r_target.key = "ui_ctrl:" + itos(hit->get_instance_id());
				r_target.text = txt;
				r_target.anchor_global_rect = ClockGearsLongHelpController::get_control_global_rect_in_host(hit, p_host_parent);
				r_target.prefer_beside = false;
				return true;
			}
		}
	}
	return false;
}

Dictionary ClockGearsUiDesignEditor::debug_step_long_help(float p_delta_sec, const Vector2 &p_pointer_pos) {
	Window *host_parent = get_window();
	if (get_tree() && get_tree()->get_root()) {
		host_parent = get_tree()->get_root();
	}
	Vector2 ptr = p_pointer_pos;
	if (ptr.x < 0.0f && ptr.y < 0.0f) {
		ptr = host_parent ? host_parent->get_mouse_position() : Vector2();
	}
	ClockGearsLongHelpController::Target tgt;
	resolve_long_help_target(host_parent, ptr, tgt);
	long_help_ctrl.step(this, p_delta_sec, ptr, tgt);
	return long_help_ctrl.get_debug_state();
}

Dictionary ClockGearsUiDesignEditor::debug_get_long_help_state() const {
	return long_help_ctrl.get_debug_state();
}

// ============================================================================
// Sección consolidada "Ajustes de la pantalla" (9B Corr Punto 5)
// ============================================================================

static HBoxContainer *s_ss_add_row(VBoxContainer *p_parent, const String &p_label, const String &p_tip, Control *p_ctrl, float p_scale) {
	HBoxContainer *hb = memnew(HBoxContainer);
	hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hb->set_custom_minimum_size(Size2(0, 32 * p_scale));
	hb->add_theme_constant_override("separation", int(8 * p_scale));
	Label *lbl = memnew(Label(p_label));
	lbl->set_custom_minimum_size(Size2(122 * p_scale, 32 * p_scale));
	lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	lbl->set_clip_text(false);
	lbl->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	lbl->set_tooltip_text(p_tip);
	hb->add_child(lbl);
	if (p_ctrl) {
		p_ctrl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		if (p_ctrl->get_tooltip_text().is_empty()) {
			p_ctrl->set_tooltip_text(p_tip);
		}
		s_clamp_narrow_controls(p_ctrl, p_scale);
		hb->add_child(p_ctrl);
	}
	p_parent->add_child(hb);
	return hb;
}

static void s_ss_populate_window_styles(OptionButton *p_opt, ClockGearsDataManager *p_dm, const String &p_selected_id, const String &p_default_label) {
	if (!p_opt) return;
	p_opt->clear();
	p_opt->add_item(p_default_label, 0);
	p_opt->set_item_metadata(0, String(""));
	if (p_dm) {
		Array styles = p_dm->get_window_styles();
		for (int i = 0; i < styles.size(); i++) {
			Dictionary st = styles[i];
			String sid = String(st.get("id", ""));
			if (sid.is_empty()) continue;
			int idx = p_opt->get_item_count();
			p_opt->add_item(String(st.get("name", sid)), idx);
			p_opt->set_item_metadata(idx, sid);
		}
	}
	s_select_option_meta(p_opt, p_selected_id);
}

void ClockGearsUiDesignEditor::_refresh_screen_settings_panel() {
	if (!screen_settings_body) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}

	s_clear_container_children(screen_settings_body);
	ss_hud_add_type_opt = nullptr;
	ss_hud_sample_hp_spin = nullptr;
	ss_hud_sample_gold_spin = nullptr;
	ss_hud_sample_qty_spin = nullptr;
	ss_cred_add_type_opt = nullptr;
	ss_cred_item_text_edit = nullptr;
	ss_cred_item_role_edit = nullptr;
	ss_cred_item_name_edit = nullptr;
	ss_cred_item_img_edit = nullptr;
	ss_cred_item_spacer_spin = nullptr;
	ss_cred_item_align_opt = nullptr;

	bool prev_upd = updating_ui;
	updating_ui = true;

	auto add_file_picker = [&](VBoxContainer *p_parent, const String &p_label, const String &p_tip, const String &p_ctrl_name, const String &p_val, const String &p_target, bool p_is_audio, const Callable &p_on_change) -> LineEdit * {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", int(4 * EDSCALE));
		LineEdit *ed = memnew(LineEdit);
		ed->set_name(p_ctrl_name);
		ed->set_text(p_val);
		ed->set_placeholder(p_is_audio ? "res://audio/..." : "res://img/...");
		ed->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ed->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
		ed->set_tooltip_text(p_tip);
		ed->connect("text_changed", p_on_change.unbind(1));
		hb->add_child(ed);

		Button *pick_b = memnew(Button(String::utf8("Elegir…")));
		pick_b->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
		pick_b->set_clip_text(false);
		s_make_rounded_action_btn(pick_b);
		pick_b->set_tooltip_text(String::utf8(p_is_audio ? "Elegir archivo de audio del equipo." : "Elegir archivo de imagen del equipo."));
		pick_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_asset_pick_pressed).bind(p_target, p_is_audio));
		hb->add_child(pick_b);

		Button *clr_b = memnew(Button("X"));
		clr_b->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
		clr_b->set_clip_text(false);
		s_make_rounded_action_btn(clr_b);
		clr_b->set_tooltip_text(String::utf8("Vaciar esta ruta de archivo."));
		clr_b->connect("pressed", callable_mp(ed, &LineEdit::set_text).bind(String("")));
		clr_b->connect("pressed", p_on_change);
		hb->add_child(clr_b);

		s_ss_add_row(p_parent, p_label, p_tip, hb, EDSCALE);
		return ed;
	};

	auto build_entries_list = [&](const Array &p_entries, bool p_is_gm) {
		Label *hdr = memnew(Label(String::utf8("Entradas visibles y orden:")));
		hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
		screen_settings_body->add_child(hdr);

		for (int i = 0; i < p_entries.size(); i++) {
			if (p_entries[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary e = p_entries[i];
			String eid = String(e.get("id", ""));
			bool enabled = bool(e.get("enabled", true));
			bool reserved = bool(e.get("reserved", false));
			String etext = String(e.get("text", eid));

			HBoxContainer *row = memnew(HBoxContainer);
			row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			row->add_theme_constant_override("separation", int(4 * EDSCALE));

			CheckBox *chk = memnew(CheckBox);
			chk->set_pressed(enabled);
			chk->set_disabled(reserved);
			chk->set_tooltip_text(reserved ? String::utf8("Opción reservada para sistema futuro.") : String::utf8("Mostrar u ocultar esta opción en el menú."));
			chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_entry_toggled).bind(eid, p_is_gm));
			row->add_child(chk);

			LineEdit *ed = memnew(LineEdit);
			ed->set_text(etext);
			ed->set_editable(!reserved);
			ed->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			ed->set_tooltip_text(String::utf8("Texto visible de esta entrada del menú."));
			ed->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_entry_text_changed).bind(eid, p_is_gm));
			row->add_child(ed);

			Button *up_b = memnew(Button(String::utf8("↑")));
			s_make_rounded_action_btn(up_b);
			up_b->set_disabled(i == 0);
			up_b->set_tooltip_text(String::utf8("Subir entrada una posición."));
			up_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_entry_move).bind(eid, -1, p_is_gm), CONNECT_DEFERRED);
			row->add_child(up_b);

			Button *dn_b = memnew(Button(String::utf8("↓")));
			s_make_rounded_action_btn(dn_b);
			dn_b->set_disabled(i + 1 >= p_entries.size());
			dn_b->set_tooltip_text(String::utf8("Bajar entrada una posición."));
			dn_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_entry_move).bind(eid, 1, p_is_gm), CONNECT_DEFERRED);
			row->add_child(dn_b);

			screen_settings_body->add_child(row);
		}
	};

	if (current_screen_id == "title") {
		Dictionary tcfg = dm->get_title_menu_config();
		Dictionary tit = tcfg.get("title", Dictionary());
		Callable cb = callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_title_changed);

		OptionButton *type_opt = memnew(OptionButton);
		type_opt->set_name("SsTitleTypeOpt");
		type_opt->add_item("Texto", 0); type_opt->set_item_metadata(0, "text");
		type_opt->add_item("Imagen / Logo", 1); type_opt->set_item_metadata(1, "image");
		s_select_option_meta(type_opt, String(tit.get("type", "text")));
		type_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, String::utf8("Tipo título:"), String::utf8("Elige si el título muestra texto o una imagen de logo."), type_opt, EDSCALE);

		LineEdit *txt_ed = memnew(LineEdit);
		txt_ed->set_name("SsTitleTextEdit");
		txt_ed->set_text(String(tit.get("text", "")));
		txt_ed->set_placeholder("Nombre del juego");
		txt_ed->connect("text_changed", cb.unbind(1));
		s_ss_add_row(screen_settings_body, String::utf8("Título:"), String::utf8("Texto principal mostrado en la pantalla de título."), txt_ed, EDSCALE);

		add_file_picker(screen_settings_body, String::utf8("Logo título:"), String::utf8("Imagen de logo opcional para el título."), "SsTitleImgEdit", String(tit.get("image_path", "")), "title_image", false, cb);

		OptionButton *flt_opt = memnew(OptionButton);
		flt_opt->set_name("SsTitleFilterOpt");
		flt_opt->add_item("Pixel art", 0); flt_opt->set_item_metadata(0, "nearest");
		flt_opt->add_item("Suave (HD)", 1); flt_opt->set_item_metadata(1, "linear");
		s_select_option_meta(flt_opt, String(tit.get("filter", "nearest")));
		flt_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Filtro logo:", String::utf8("Pixel art mantiene bordes nítidos; Suave interpola imágenes HD."), flt_opt, EDSCALE);

		add_file_picker(screen_settings_body, String::utf8("Música (BGM):"), String::utf8("Música de fondo de la pantalla de título."), "SsTitleBgmEdit", String(tcfg.get("bgm_path", "")), "title_bgm", true, cb);

		SpinBox *slots_sp = memnew(SpinBox);
		slots_sp->set_name("SsSaveSlotsSpin");
		slots_sp->set_min(1); slots_sp->set_max(99);
		slots_sp->set_value(int(tcfg.get("save_slots", 10)));
		slots_sp->connect("value_changed", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Ranuras:", String::utf8("Cantidad máxima de ranuras en la lista de partidas."), slots_sp, EDSCALE);

		build_entries_list(Array(tcfg.get("entries", Array())), false);
	} else if (current_screen_id == "game_menu_main") {
		Dictionary gm = dm->get_game_menu_config();
		build_entries_list(Array(gm.get("entries", Array())), true);
	} else if (current_screen_id == "game_menu_items") {
		Dictionary gm = dm->get_game_menu_config();
		Dictionary ip = gm.get("item_preview", Dictionary());
		Callable cb = callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_gm_item_preview_changed);

		CheckBox *en_chk = memnew(CheckBox("Activar"));
		en_chk->set_name("SsIpEnabledChk");
		en_chk->set_pressed(ip.has("enabled") ? bool(ip["enabled"]) : true);
		en_chk->connect("toggled", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Vista previa:", String::utf8("Muestra un recuadro con el objeto seleccionado y su descripción."), en_chk, EDSCALE);

		OptionButton *pos_opt = memnew(OptionButton);
		pos_opt->set_name("SsIpPosOpt");
		pos_opt->add_item("Derecha", 0); pos_opt->set_item_metadata(0, "right");
		pos_opt->add_item("Izquierda", 1); pos_opt->set_item_metadata(1, "left");
		s_select_option_meta(pos_opt, String(ip.get("position", "right")));
		pos_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, String::utf8("Posición:"), String::utf8("Coloca la vista previa a izquierda o derecha del listado."), pos_opt, EDSCALE);

		HFlowContainer *flags_hb = memnew(HFlowContainer);
		flags_hb->add_theme_constant_override("h_separation", int(4 * EDSCALE));
		flags_hb->add_theme_constant_override("v_separation", int(2 * EDSCALE));
		CheckBox *ic_chk = memnew(CheckBox("Icono"));
		ic_chk->set_name("SsIpIconChk");
		ic_chk->set_pressed(ip.has("show_icon") ? bool(ip["show_icon"]) : true);
		ic_chk->set_tooltip_text(String::utf8("Muestra el icono ampliado del objeto."));
		ic_chk->connect("toggled", cb.unbind(1));
		flags_hb->add_child(ic_chk);
		CheckBox *nm_chk = memnew(CheckBox("Nombre"));
		nm_chk->set_name("SsIpNameChk");
		nm_chk->set_pressed(ip.has("show_name") ? bool(ip["show_name"]) : true);
		nm_chk->set_tooltip_text(String::utf8("Muestra el nombre del objeto."));
		nm_chk->connect("toggled", cb.unbind(1));
		flags_hb->add_child(nm_chk);
		CheckBox *ds_chk = memnew(CheckBox("Desc."));
		ds_chk->set_name("SsIpDescChk");
		ds_chk->set_pressed(ip.has("show_desc") ? bool(ip["show_desc"]) : true);
		ds_chk->set_tooltip_text(String::utf8("Muestra la descripción del objeto."));
		ds_chk->connect("toggled", cb.unbind(1));
		flags_hb->add_child(ds_chk);
		s_ss_add_row(screen_settings_body, "Mostrar:", String::utf8("Elementos incluidos dentro del panel de vista previa."), flags_hb, EDSCALE);

		OptionButton *sc_opt = memnew(OptionButton);
		sc_opt->set_name("SsIpScaleOpt");
		for (int s = 1; s <= 4; s++) {
			sc_opt->add_item(itos(s) + "x", s - 1);
			sc_opt->set_item_metadata(s - 1, itos(s));
		}
		s_select_option_meta(sc_opt, itos(CLAMP(int(ip.get("icon_scale", 2)), 1, 4)));
		sc_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Escala icono:", String::utf8("Aumento del icono del objeto en vista previa."), sc_opt, EDSCALE);

		HBoxContainer *rot_hb = memnew(HBoxContainer);
		CheckBox *rot_chk = memnew(CheckBox("Rotar"));
		rot_chk->set_name("SsIpRotChk");
		rot_chk->set_pressed(ip.has("rotation_enabled") ? bool(ip["rotation_enabled"]) : true);
		rot_chk->set_tooltip_text(String::utf8("Anima una rotación suave sobre el icono."));
		rot_chk->connect("toggled", cb.unbind(1));
		rot_hb->add_child(rot_chk);
		SpinBox *step_sp = memnew(SpinBox);
		step_sp->set_name("SsIpRotStepSpin");
		step_sp->set_min(1); step_sp->set_max(90);
		step_sp->set_value(int(ip.get("rotation_step", 15)));
		step_sp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		step_sp->set_tooltip_text(String::utf8("Grados por paso de giro en la vista previa."));
		step_sp->connect("value_changed", cb.unbind(1));
		rot_hb->add_child(step_sp);
		s_ss_add_row(screen_settings_body, String::utf8("Rotación:"), String::utf8("Configura el giro automático del icono de vista previa."), rot_hb, EDSCALE);

		OptionButton *pws_opt = memnew(OptionButton);
		pws_opt->set_name("SsIpPrevStyleOpt");
		s_ss_populate_window_styles(pws_opt, dm, String(ip.get("preview_window_style", "")), String::utf8("(Por defecto)"));
		pws_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Estilo panel:", String::utf8("Estilo de ventana del recuadro de vista previa."), pws_opt, EDSCALE);

		OptionButton *dws_opt = memnew(OptionButton);
		dws_opt->set_name("SsIpDescStyleOpt");
		s_ss_populate_window_styles(dws_opt, dm, String(ip.get("desc_window_style", "")), String::utf8("(Por defecto)"));
		dws_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Estilo desc.:", String::utf8("Estilo de ventana del panel de descripción."), dws_opt, EDSCALE);
	} else if (current_screen_id == "game_menu_save") {
		Dictionary tcfg = dm->get_title_menu_config();
		SpinBox *slots_sp = memnew(SpinBox);
		slots_sp->set_name("SsSaveScreenSlotsSpin");
		slots_sp->set_min(1); slots_sp->set_max(99);
		slots_sp->set_value(int(tcfg.get("save_slots", 10)));
		slots_sp->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_title_changed).unbind(1));
		s_ss_add_row(screen_settings_body, "Ranuras:", String::utf8("Número de ranuras visibles al guardar o cargar partida."), slots_sp, EDSCALE);
	} else if (current_screen_id == "credits") {
		Dictionary ccfg = dm->get_credits_config();
		Callable cb = callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_meta_changed);

		add_file_picker(screen_settings_body, String::utf8("Música (BGM):"), String::utf8("Música reproducida durante los créditos."), "SsCredBgmEdit", String(ccfg.get("bgm_path", "")), "credits_bgm", true, cb);

		SpinBox *spd_sp = memnew(SpinBox);
		spd_sp->set_name("SsCredSpeedSpin");
		spd_sp->set_min(5); spd_sp->set_max(600);
		spd_sp->set_value(double(ccfg.get("scroll_speed", 60.0)));
		spd_sp->connect("value_changed", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Velocidad:", String::utf8("Velocidad vertical del scroll de créditos en px/s."), spd_sp, EDSCALE);

		OptionButton *al_opt = memnew(OptionButton);
		al_opt->set_name("SsCredAlignOpt");
		al_opt->add_item("Izquierda", 0); al_opt->set_item_metadata(0, "left");
		al_opt->add_item("Centro", 1); al_opt->set_item_metadata(1, "center");
		al_opt->add_item("Derecha", 2); al_opt->set_item_metadata(2, "right");
		s_select_option_meta(al_opt, String(ccfg.get("alignment", "center")));
		al_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, String::utf8("Alineación:"), String::utf8("Alineación horizontal por defecto de los créditos."), al_opt, EDSCALE);

		Dictionary styles = ccfg.get("styles", Dictionary());
		static const char *k_sk[4] = { "section", "role", "name", "text" };
		static const char *k_sl[4] = { "Sección:", "Rol:", "Nombre:", "Texto:" };
		for (int i = 0; i < 4; i++) {
			Dictionary st = styles.get(k_sk[i], Dictionary());
			HBoxContainer *shb = memnew(HBoxContainer);
			shb->add_theme_constant_override("separation", int(4 * EDSCALE));
			SpinBox *sz = memnew(SpinBox);
			sz->set_name(String("SsCredStyleSize_") + k_sk[i]);
			sz->set_min(8); sz->set_max(96);
			sz->set_value(int(st.get("font_size", 18)));
			sz->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			sz->set_tooltip_text(String::utf8("Tamaño de fuente en píxeles."));
			sz->connect("value_changed", cb.unbind(1));
			shb->add_child(sz);
			ColorPickerButton *cp = memnew(ColorPickerButton);
			cp->set_name(String("SsCredStyleCol_") + k_sk[i]);
			String hc = String(st.get("color", "#ffffffff"));
			if (Color::html_is_valid(hc)) cp->set_pick_color(Color::html(hc));
			cp->set_tooltip_text(String::utf8("Color de este estilo de texto."));
			cp->connect("color_changed", cb.unbind(1));
			shb->add_child(cp);
			s_ss_add_row(screen_settings_body, String::utf8(k_sl[i]), String::utf8("Tamaño y color para líneas de este tipo."), shb, EDSCALE);
		}

		HFlowContainer *add_hb = memnew(HFlowContainer);
		add_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		add_hb->add_theme_constant_override("h_separation", int(4 * EDSCALE));
		add_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
		ss_cred_add_type_opt = memnew(OptionButton);
		ss_cred_add_type_opt->add_item(String::utf8("Sección"), 0); ss_cred_add_type_opt->set_item_metadata(0, "section");
		ss_cred_add_type_opt->add_item("Rol + Nombre", 1); ss_cred_add_type_opt->set_item_metadata(1, "role_name");
		ss_cred_add_type_opt->add_item("Texto", 2); ss_cred_add_type_opt->set_item_metadata(2, "text");
		ss_cred_add_type_opt->add_item("Imagen", 3); ss_cred_add_type_opt->set_item_metadata(3, "image");
		ss_cred_add_type_opt->add_item("Espacio", 4); ss_cred_add_type_opt->set_item_metadata(4, "spacer");
		ss_cred_add_type_opt->set_custom_minimum_size(Size2(110 * EDSCALE, 0));
		ss_cred_add_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ss_cred_add_type_opt->set_tooltip_text(String::utf8("Tipo de entrada nueva para añadir a los créditos."));
		add_hb->add_child(ss_cred_add_type_opt);
		Button *cadd = memnew(Button(String::utf8("+ Añadir")));
		s_make_rounded_action_btn(cadd);
		cadd->set_tooltip_text(String::utf8("Añade un elemento al final de la lista de créditos."));
		cadd->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_add_pressed), CONNECT_DEFERRED);
		add_hb->add_child(cadd);
		Button *cdel = memnew(Button("Quitar"));
		s_make_rounded_action_btn(cdel);
		cdel->set_tooltip_text(String::utf8("Elimina la línea seleccionada de créditos."));
		cdel->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_del_pressed), CONNECT_DEFERRED);
		add_hb->add_child(cdel);
		Button *cup = memnew(Button(String::utf8("↑")));
		s_make_rounded_action_btn(cup);
		cup->set_tooltip_text(String::utf8("Sube la línea seleccionada de créditos."));
		cup->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_move_pressed).bind(-1), CONNECT_DEFERRED);
		add_hb->add_child(cup);
		Button *cdn = memnew(Button(String::utf8("↓")));
		s_make_rounded_action_btn(cdn);
		cdn->set_tooltip_text(String::utf8("Baja la línea seleccionada de créditos."));
		cdn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_move_pressed).bind(1), CONNECT_DEFERRED);
		add_hb->add_child(cdn);
		screen_settings_body->add_child(add_hb);

		Array items = ccfg.get("items", Array());
		ItemList *clist = memnew(ItemList);
		clist->set_name("SsCredItemsList");
		clist->set_custom_minimum_size(Size2(0, 92 * EDSCALE));
		clist->set_allow_reselect(true);
		for (int i = 0; i < items.size(); i++) {
			Dictionary it = items[i];
			String tp = String(it.get("type", "text"));
			String desc;
			if (tp == "role_name") desc = String(it.get("role", "")) + String::utf8(" — ") + String(it.get("name", ""));
			else if (tp == "image") desc = "[Imagen] " + String(it.get("image_path", "")).get_file();
			else if (tp == "spacer") desc = "[Espacio " + itos(int(it.get("height", 32))) + "px]";
			else desc = "[" + tp + "] " + String(it.get("text", ""));
			clist->add_item(desc);
		}
		if (ss_selected_cred_idx >= items.size()) ss_selected_cred_idx = items.size() - 1;
		if (ss_selected_cred_idx < 0 && !items.is_empty()) ss_selected_cred_idx = 0;
		if (ss_selected_cred_idx >= 0) clist->select(ss_selected_cred_idx);
		clist->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_item_selected), CONNECT_DEFERRED);
		screen_settings_body->add_child(clist);

		if (ss_selected_cred_idx >= 0 && ss_selected_cred_idx < items.size()) {
			Dictionary sel_it = items[ss_selected_cred_idx];
			String itp = String(sel_it.get("type", "text"));
			Callable icb = callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_credits_item_field_changed);
			if (itp == "section" || itp == "text") {
				ss_cred_item_text_edit = memnew(LineEdit);
				ss_cred_item_text_edit->set_text(String(sel_it.get("text", "")));
				ss_cred_item_text_edit->connect("text_changed", icb.unbind(1));
				s_ss_add_row(screen_settings_body, "Contenido:", String::utf8("Texto mostrado en esta línea de créditos."), ss_cred_item_text_edit, EDSCALE);
			} else if (itp == "role_name") {
				ss_cred_item_role_edit = memnew(LineEdit);
				ss_cred_item_role_edit->set_text(String(sel_it.get("role", "")));
				ss_cred_item_role_edit->connect("text_changed", icb.unbind(1));
				s_ss_add_row(screen_settings_body, "Rol:", String::utf8("Cargo o título de la persona o equipo."), ss_cred_item_role_edit, EDSCALE);
				ss_cred_item_name_edit = memnew(LineEdit);
				ss_cred_item_name_edit->set_text(String(sel_it.get("name", "")));
				ss_cred_item_name_edit->connect("text_changed", icb.unbind(1));
				s_ss_add_row(screen_settings_body, "Nombre:", String::utf8("Nombre de la persona o estudio."), ss_cred_item_name_edit, EDSCALE);
			} else if (itp == "image") {
				ss_cred_item_img_edit = add_file_picker(screen_settings_body, "Imagen:", String::utf8("Logotipo o ilustración dentro de los créditos."), "SsCredItemImgEdit", String(sel_it.get("image_path", "")), "credits_item_image", false, icb);
			} else if (itp == "spacer") {
				ss_cred_item_spacer_spin = memnew(SpinBox);
				ss_cred_item_spacer_spin->set_min(4); ss_cred_item_spacer_spin->set_max(480);
				ss_cred_item_spacer_spin->set_value(int(sel_it.get("height", 32)));
				ss_cred_item_spacer_spin->connect("value_changed", icb.unbind(1));
				s_ss_add_row(screen_settings_body, "Alto (px):", String::utf8("Separación vertical en píxeles."), ss_cred_item_spacer_spin, EDSCALE);
			}
			ss_cred_item_align_opt = memnew(OptionButton);
			ss_cred_item_align_opt->add_item("(General)", 0); ss_cred_item_align_opt->set_item_metadata(0, "");
			ss_cred_item_align_opt->add_item("Izquierda", 1); ss_cred_item_align_opt->set_item_metadata(1, "left");
			ss_cred_item_align_opt->add_item("Centro", 2); ss_cred_item_align_opt->set_item_metadata(2, "center");
			ss_cred_item_align_opt->add_item("Derecha", 3); ss_cred_item_align_opt->set_item_metadata(3, "right");
			s_select_option_meta(ss_cred_item_align_opt, String(sel_it.get("align", "")));
			ss_cred_item_align_opt->connect("item_selected", icb.unbind(1));
			s_ss_add_row(screen_settings_body, String::utf8("Alineación:"), String::utf8("Alineación propia para este elemento."), ss_cred_item_align_opt, EDSCALE);
		}
	} else if (current_screen_id == "hud") {
		HBoxContainer *whb = memnew(HBoxContainer);
		whb->add_theme_constant_override("separation", int(4 * EDSCALE));
		ss_hud_add_type_opt = memnew(OptionButton);
		ss_hud_add_type_opt->add_item("Barra PV/PM", 0); ss_hud_add_type_opt->set_item_metadata(0, "bar");
		ss_hud_add_type_opt->add_item("Corazones", 1); ss_hud_add_type_opt->set_item_metadata(1, "hearts");
		ss_hud_add_type_opt->add_item("Monedas", 2); ss_hud_add_type_opt->set_item_metadata(2, "currency");
		ss_hud_add_type_opt->add_item("Objeto", 3); ss_hud_add_type_opt->set_item_metadata(3, "item");
		ss_hud_add_type_opt->add_item("Texto", 4); ss_hud_add_type_opt->set_item_metadata(4, "text");
		ss_hud_add_type_opt->add_item("Imagen", 5); ss_hud_add_type_opt->set_item_metadata(5, "image");
		ss_hud_add_type_opt->set_fit_to_longest_item(false);
		ss_hud_add_type_opt->set_clip_text(true);
		ss_hud_add_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ss_hud_add_type_opt->set_tooltip_text(String::utf8("Tipo de widget HUD nuevo que quieres añadir."));
		whb->add_child(ss_hud_add_type_opt);
		Button *hadd = memnew(Button(String::utf8("+ Widget")));
		s_make_rounded_action_btn(hadd);
		hadd->set_tooltip_text(String::utf8("Crea un widget nuevo en el HUD visible sobre el lienzo."));
		hadd->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_hud_add_widget_pressed), CONNECT_DEFERRED);
		whb->add_child(hadd);
		s_ss_add_row(screen_settings_body, "Nuevo HUD:", String::utf8("Añade un widget HUD y edítalo directamente en el lienzo."), whb, EDSCALE);

		VBoxContainer *shb = memnew(VBoxContainer);
		shb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		shb->add_theme_constant_override("separation", int(3 * EDSCALE));
		ss_hud_sample_hp_spin = memnew(SpinBox);
		ss_hud_sample_hp_spin->set_min(0); ss_hud_sample_hp_spin->set_max(9999); ss_hud_sample_hp_spin->set_value(75);
		ss_hud_sample_hp_spin->set_prefix("PV "); ss_hud_sample_hp_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ss_hud_sample_hp_spin->set_tooltip_text(String::utf8("PV de ejemplo en la vista previa del lienzo."));
		ss_hud_sample_hp_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_hud_sample_changed));
		shb->add_child(ss_hud_sample_hp_spin);
		ss_hud_sample_gold_spin = memnew(SpinBox);
		ss_hud_sample_gold_spin->set_min(0); ss_hud_sample_gold_spin->set_max(999999); ss_hud_sample_gold_spin->set_value(250);
		ss_hud_sample_gold_spin->set_prefix("Oro "); ss_hud_sample_gold_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ss_hud_sample_gold_spin->set_tooltip_text(String::utf8("Oro de ejemplo en la vista previa del lienzo."));
		ss_hud_sample_gold_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_hud_sample_changed));
		shb->add_child(ss_hud_sample_gold_spin);
		ss_hud_sample_qty_spin = memnew(SpinBox);
		ss_hud_sample_qty_spin->set_min(0); ss_hud_sample_qty_spin->set_max(99); ss_hud_sample_qty_spin->set_value(3);
		ss_hud_sample_qty_spin->set_prefix("Obj "); ss_hud_sample_qty_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ss_hud_sample_qty_spin->set_tooltip_text(String::utf8("Cantidad de objeto de ejemplo en vista previa."));
		ss_hud_sample_qty_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_hud_sample_changed));
		shb->add_child(ss_hud_sample_qty_spin);
		s_ss_add_row(screen_settings_body, "Muestras:", String::utf8("Valores temporales para previsualizar barras y contadores del HUD."), shb, EDSCALE);
	} else if (current_screen_id == "game_over") {
		Dictionary gcfg = dm->get_game_over_config();
		Callable cb = callable_mp(this, &ClockGearsUiDesignEditor::_on_ss_gameover_changed);

		LineEdit *gt_ed = memnew(LineEdit);
		gt_ed->set_name("SsGoTitleTextEdit");
		gt_ed->set_text(String(gcfg.get("title_text", "GAME OVER")));
		gt_ed->connect("text_changed", cb.unbind(1));
		s_ss_add_row(screen_settings_body, String::utf8("Título:"), String::utf8("Texto principal mostrado al perder la partida."), gt_ed, EDSCALE);

		HBoxContainer *tc_hb = memnew(HBoxContainer);
		tc_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		ColorPickerButton *gc_btn = memnew(ColorPickerButton);
		gc_btn->set_name("SsGoTitleColBtn");
		String gcol = String(gcfg.get("title_color", "#e84a5fff"));
		if (Color::html_is_valid(gcol)) gc_btn->set_pick_color(Color::html(gcol));
		gc_btn->set_tooltip_text(String::utf8("Color del título de Fin del Juego."));
		gc_btn->connect("color_changed", cb.unbind(1));
		tc_hb->add_child(gc_btn);
		SpinBox *gs_spin = memnew(SpinBox);
		gs_spin->set_name("SsGoTitleSizeSpin");
		gs_spin->set_min(16); gs_spin->set_max(128);
		gs_spin->set_value(int(gcfg.get("title_font_size", 48)));
		gs_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		gs_spin->set_tooltip_text(String::utf8("Tamaño en píxeles del título de Fin del Juego."));
		gs_spin->connect("value_changed", cb.unbind(1));
		tc_hb->add_child(gs_spin);
		s_ss_add_row(screen_settings_body, "Aspecto:", String::utf8("Color y tamaño del texto principal."), tc_hb, EDSCALE);

		add_file_picker(screen_settings_body, String::utf8("Música (BGM):"), String::utf8("Música reproducida al llegar a Fin del Juego."), "SsGoBgmEdit", String(gcfg.get("bgm_path", "")), "gameover_bgm", true, cb);

		HBoxContainer *rb_hb = memnew(HBoxContainer);
		CheckBox *r_chk = memnew(CheckBox("Activo"));
		r_chk->set_name("SsGoRetryChk");
		r_chk->set_pressed(gcfg.has("retry_enabled") ? bool(gcfg["retry_enabled"]) : true);
		r_chk->connect("toggled", cb.unbind(1));
		rb_hb->add_child(r_chk);
		LineEdit *r_ed = memnew(LineEdit);
		r_ed->set_name("SsGoRetryEdit");
		r_ed->set_text(String(gcfg.get("retry_label", "Reintentar")));
		r_ed->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_ed->connect("text_changed", cb.unbind(1));
		rb_hb->add_child(r_ed);
		s_ss_add_row(screen_settings_body, "Reintentar:", String::utf8("Activa y personaliza el botón de reintento."), rb_hb, EDSCALE);

		HBoxContainer *tb_hb = memnew(HBoxContainer);
		CheckBox *t_chk = memnew(CheckBox("Activo"));
		t_chk->set_name("SsGoToTitleChk");
		t_chk->set_pressed(gcfg.has("title_enabled") ? bool(gcfg["title_enabled"]) : true);
		t_chk->connect("toggled", cb.unbind(1));
		tb_hb->add_child(t_chk);
		LineEdit *t_ed = memnew(LineEdit);
		t_ed->set_name("SsGoToTitleEdit");
		t_ed->set_text(String(gcfg.get("title_label", String::utf8("Volver al menú"))));
		t_ed->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		t_ed->connect("text_changed", cb.unbind(1));
		tb_hb->add_child(t_ed);
		s_ss_add_row(screen_settings_body, String::utf8("Al título:"), String::utf8("Activa y personaliza el botón de volver al menú principal."), tb_hb, EDSCALE);

		OptionButton *gws_opt = memnew(OptionButton);
		gws_opt->set_name("SsGoWinStyleOpt");
		s_ss_populate_window_styles(gws_opt, dm, String(gcfg.get("window_style_id", "default")), String::utf8("Por defecto"));
		gws_opt->connect("item_selected", cb.unbind(1));
		s_ss_add_row(screen_settings_body, "Estilo:", String::utf8("Estilo visual aplicado a la ventana de Fin del Juego."), gws_opt, EDSCALE);
	} else if (current_screen_id == "messages") {
		Label *mb_info = memnew(Label(String::utf8("Cuadro de diálogo, caja de nombre, retratos, ventana de elecciones, entrada de número e historial. Arrastra o personaliza cualquier elemento en el lienzo.")));
		mb_info->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		mb_info->add_theme_color_override("font_color", Color(0.80f, 0.84f, 0.92f));
		screen_settings_body->add_child(mb_info);
	} else if (current_screen_id == "battle") {
		Label *bt_info = memnew(Label(String::utf8("Panel de comandos de combate, grupo, enemigos y Aviso de batalla. Arrastra o personaliza cualquier elemento en el lienzo.")));
		bt_info->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		bt_info->add_theme_color_override("font_color", Color(0.80f, 0.84f, 0.92f));
		screen_settings_body->add_child(bt_info);
	} else {
		Label *info = memnew(Label(String::utf8("Personaliza fondo, animaciones, entradas y elementos visuales desde el lienzo y las secciones de abajo.")));
		info->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		info->add_theme_color_override("font_color", Color(0.75f, 0.78f, 0.86f));
		screen_settings_body->add_child(info);
	}

	updating_ui = prev_upd;
	s_clamp_narrow_controls(screen_settings_body, EDSCALE);
	if (ClockGearsTheme::get_singleton() && ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass") {
		ClockGearsTheme::get_singleton()->apply_theme_to_node_tree(screen_settings_body);
	}
}

void ClockGearsUiDesignEditor::_on_ss_asset_pick_pressed(const String &p_target, bool p_is_audio) {
	screen_asset_target = p_target;
	if (!screen_asset_dialog) return;
	screen_asset_dialog->clear_filters();
	if (p_is_audio) {
		screen_asset_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Audio");
	} else {
		screen_asset_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; Imagen");
	}
	String proj_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path("res://") : "";
	if (!proj_dir.is_empty()) {
		screen_asset_dialog->set_current_dir(proj_dir);
	}
	screen_asset_dialog->popup_centered_ratio(0.55f);
}

void ClockGearsUiDesignEditor::_on_ss_asset_file_selected(const String &p_path) {
	String final_path = p_path;
	if (screen_asset_target.ends_with("_bgm")) {
		String tdir = "res://audio/bgm";
		DirAccess::make_dir_recursive_absolute(tdir);
		String dst = tdir.path_join(p_path.get_file());
		if (!p_path.begins_with("res://") && FileAccess::exists(p_path)) {
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			if (da.is_valid()) {
				da->copy(p_path, ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(dst) : dst);
			}
			final_path = dst;
		} else if (p_path.begins_with("res://")) {
			final_path = p_path;
		}
	} else if (owner_db) {
		final_path = owner_db->import_menu_asset_for_ui_designer(p_path, true);
	}
	if (final_path.is_empty() || !screen_settings_body) return;

	if (screen_asset_target == "title_image") {
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsTitleImgEdit", true, false))) {
			ed->set_text(final_path);
		}
		_on_ss_title_changed();
	} else if (screen_asset_target == "title_bgm") {
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsTitleBgmEdit", true, false))) {
			ed->set_text(final_path);
		}
		_on_ss_title_changed();
	} else if (screen_asset_target == "credits_bgm") {
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsCredBgmEdit", true, false))) {
			ed->set_text(final_path);
		}
		_on_ss_credits_meta_changed();
	} else if (screen_asset_target == "credits_item_image") {
		if (ss_cred_item_img_edit) ss_cred_item_img_edit->set_text(final_path);
		_on_ss_credits_item_field_changed();
	} else if (screen_asset_target == "gameover_bgm") {
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsGoBgmEdit", true, false))) {
			ed->set_text(final_path);
		}
		_on_ss_gameover_changed();
	}
}

void ClockGearsUiDesignEditor::_on_ss_title_changed() {
	if (updating_ui || !screen_settings_body) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary cfg = dm->get_title_menu_config();
	if (current_screen_id == "title") {
		Dictionary tit = Dictionary(cfg.get("title", Dictionary())).duplicate(true);
		if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsTitleTypeOpt", true, false))) {
			if (opt->get_selected() >= 0) tit["type"] = String(opt->get_item_metadata(opt->get_selected()));
		}
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsTitleTextEdit", true, false))) {
			tit["text"] = ed->get_text();
		}
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsTitleImgEdit", true, false))) {
			tit["image_path"] = ed->get_text().strip_edges();
		}
		if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsTitleFilterOpt", true, false))) {
			if (opt->get_selected() >= 0) tit["filter"] = String(opt->get_item_metadata(opt->get_selected()));
		}
		cfg["title"] = tit;
		if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsTitleBgmEdit", true, false))) {
			String b = ed->get_text().strip_edges();
			cfg["bgm_path"] = b;
			if (menu_bgm_edit) menu_bgm_edit->set_text(b);
		}
		if (SpinBox *sp = Object::cast_to<SpinBox>(screen_settings_body->find_child("SsSaveSlotsSpin", true, false))) {
			cfg["save_slots"] = int(sp->get_value());
		}
	} else if (current_screen_id == "game_menu_save") {
		if (SpinBox *sp = Object::cast_to<SpinBox>(screen_settings_body->find_child("SsSaveScreenSlotsSpin", true, false))) {
			cfg["save_slots"] = int(sp->get_value());
		}
	}
	dm->set_title_menu_config(cfg);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

static void s_sync_ss_entries_to_design(ClockGearsUiDesignEditor *p_ed, ClockGearsDataManager *p_dm, bool p_is_game_menu, const Array &p_entries) {
	if (!p_dm) {
		return;
	}
	String sid = p_is_game_menu ? String("game_menu_main") : String("title");
	if (!p_dm->has_custom_ui_design(sid)) {
		return;
	}
	Dictionary d = p_dm->get_ui_screen_design(sid);
	Array els = d.has("elements") && d["elements"].get_type() == Variant::ARRAY ? Array(d["elements"]).duplicate(true) : Array();
	if (els.is_empty()) {
		return;
	}
	HashMap<String, Dictionary> entry_map;
	HashMap<String, int> order_map;
	for (int i = 0; i < p_entries.size(); i++) {
		if (p_entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = p_entries[i];
		String eid = String(e.get("id", "")).strip_edges().to_lower();
		if (!eid.is_empty()) {
			entry_map[eid] = e;
			order_map[eid] = i;
		}
	}
	bool changed = false;
	for (int i = 0; i < els.size(); i++) {
		if (els[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary el = els[i];
		if (String(el.get("type", "")).to_lower() != "button") continue;
		String act = String(el.get("action", "")).strip_edges().to_lower();
		String el_id = String(el.get("id", "")).strip_edges().to_lower();
		String key;
		if (entry_map.has(act)) {
			key = act;
		} else if (el_id.begins_with("menu_btn_") && entry_map.has(el_id.substr(9))) {
			key = el_id.substr(9);
		} else if (el_id.begins_with("btn_") && entry_map.has(el_id.substr(4))) {
			key = el_id.substr(4);
		}
		if (key.is_empty()) continue;
		Dictionary src = entry_map[key];
		bool en = bool(src.get("enabled", true));
		String txt = String(src.get("text", el.get("text", "")));
		el["visible"] = en;
		el["visibility_mode"] = en ? String("always") : String("never");
		el["text"] = txt;
		el["name"] = txt;
		el["focus_order"] = order_map[key];
		if (String(el.get("role", "")) == "radial_slot") {
			el["slot_index"] = order_map[key];
		}
		els[i] = el;
		changed = true;
	}
	if (changed) {
		// Reordenar botones de radial / menu_btn_* según p_entries cuando corresponda
		for (int i = 0; i < els.size(); i++) {
			if (els[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary a = els[i];
			if (String(a.get("role", "")) != "radial_slot") continue;
			for (int j = i + 1; j < els.size(); j++) {
				if (els[j].get_type() != Variant::DICTIONARY) continue;
				Dictionary b = els[j];
				if (String(b.get("role", "")) != "radial_slot" || String(b.get("parent_id", "")) != String(a.get("parent_id", ""))) continue;
				if (int(b.get("slot_index", 0)) < int(a.get("slot_index", 0))) {
					els[i] = b;
					els[j] = a;
					a = b;
				}
			}
		}
		d["elements"] = els;
		p_dm->set_ui_screen_design(sid, d, false);
		if (p_ed) {
			p_ed->refresh_from_data_manager();
		}
	}
}

void ClockGearsUiDesignEditor::_on_ss_entry_toggled(bool p_enabled, const String &p_entry_id, bool p_is_game_menu) {
	if (updating_ui) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary cfg = p_is_game_menu ? dm->get_game_menu_config() : dm->get_title_menu_config();
	Array entries = Array(cfg.get("entries", Array())).duplicate(true);
	for (int i = 0; i < entries.size(); i++) {
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_entry_id) {
			e["enabled"] = p_enabled;
			entries[i] = e;
			break;
		}
	}
	cfg["entries"] = entries;
	if (p_is_game_menu) dm->set_game_menu_config(cfg);
	else dm->set_title_menu_config(cfg);
	s_sync_ss_entries_to_design(this, dm, p_is_game_menu, entries);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_entry_text_changed(const String &p_text, const String &p_entry_id, bool p_is_game_menu) {
	if (updating_ui) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary cfg = p_is_game_menu ? dm->get_game_menu_config() : dm->get_title_menu_config();
	Array entries = Array(cfg.get("entries", Array())).duplicate(true);
	for (int i = 0; i < entries.size(); i++) {
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_entry_id) {
			e["text"] = p_text;
			entries[i] = e;
			break;
		}
	}
	cfg["entries"] = entries;
	if (p_is_game_menu) dm->set_game_menu_config(cfg);
	else dm->set_title_menu_config(cfg);
	s_sync_ss_entries_to_design(this, dm, p_is_game_menu, entries);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_entry_move(const String &p_entry_id, int p_delta, bool p_is_game_menu) {
	if (updating_ui) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary cfg = p_is_game_menu ? dm->get_game_menu_config() : dm->get_title_menu_config();
	Array entries = Array(cfg.get("entries", Array())).duplicate(true);
	int idx = -1;
	for (int i = 0; i < entries.size(); i++) {
		if (String(Dictionary(entries[i]).get("id", "")) == p_entry_id) {
			idx = i;
			break;
		}
	}
	int tgt = idx + p_delta;
	if (idx < 0 || tgt < 0 || tgt >= entries.size()) return;
	Variant tmp = entries[idx];
	entries[idx] = entries[tgt];
	entries[tgt] = tmp;
	for (int i = 0; i < entries.size(); i++) {
		Dictionary e = entries[i];
		e["order"] = i;
		entries[i] = e;
	}
	cfg["entries"] = entries;
	if (p_is_game_menu) dm->set_game_menu_config(cfg);
	else dm->set_title_menu_config(cfg);
	s_sync_ss_entries_to_design(this, dm, p_is_game_menu, entries);
	_refresh_screen_settings_panel();
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_gm_item_preview_changed() {
	if (updating_ui || !screen_settings_body) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary gm = dm->get_game_menu_config();
	Dictionary ip = Dictionary(gm.get("item_preview", Dictionary())).duplicate(true);
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsIpEnabledChk", true, false))) ip["enabled"] = chk->is_pressed();
	if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsIpPosOpt", true, false))) {
		if (opt->get_selected() >= 0) ip["position"] = String(opt->get_item_metadata(opt->get_selected()));
	}
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsIpIconChk", true, false))) ip["show_icon"] = chk->is_pressed();
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsIpNameChk", true, false))) ip["show_name"] = chk->is_pressed();
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsIpDescChk", true, false))) ip["show_desc"] = chk->is_pressed();
	if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsIpScaleOpt", true, false))) {
		if (opt->get_selected() >= 0) ip["icon_scale"] = String(opt->get_item_metadata(opt->get_selected())).to_int();
	}
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsIpRotChk", true, false))) ip["rotation_enabled"] = chk->is_pressed();
	if (SpinBox *sp = Object::cast_to<SpinBox>(screen_settings_body->find_child("SsIpRotStepSpin", true, false))) ip["rotation_step"] = int(sp->get_value());
	if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsIpPrevStyleOpt", true, false))) {
		if (opt->get_selected() >= 0) ip["preview_window_style"] = String(opt->get_item_metadata(opt->get_selected()));
	}
	if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsIpDescStyleOpt", true, false))) {
		if (opt->get_selected() >= 0) ip["desc_window_style"] = String(opt->get_item_metadata(opt->get_selected()));
	}
	gm["item_preview"] = ip;
	dm->set_game_menu_config(gm);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_credits_meta_changed() {
	if (updating_ui || !screen_settings_body) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary ccfg = dm->get_credits_config();
	if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsCredBgmEdit", true, false))) {
		ccfg["bgm_path"] = ed->get_text().strip_edges();
		if (menu_bgm_edit) menu_bgm_edit->set_text(String(ccfg["bgm_path"]));
	}
	if (SpinBox *sp = Object::cast_to<SpinBox>(screen_settings_body->find_child("SsCredSpeedSpin", true, false))) {
		ccfg["scroll_speed"] = sp->get_value();
	}
	if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsCredAlignOpt", true, false))) {
		if (opt->get_selected() >= 0) ccfg["alignment"] = String(opt->get_item_metadata(opt->get_selected()));
	}
	Dictionary styles = Dictionary(ccfg.get("styles", Dictionary())).duplicate(true);
	static const char *k_sk[4] = { "section", "role", "name", "text" };
	for (int i = 0; i < 4; i++) {
		Dictionary st = Dictionary(styles.get(k_sk[i], Dictionary())).duplicate(true);
		if (SpinBox *sz = Object::cast_to<SpinBox>(screen_settings_body->find_child(String("SsCredStyleSize_") + k_sk[i], true, false))) {
			st["font_size"] = int(sz->get_value());
		}
		if (ColorPickerButton *cp = Object::cast_to<ColorPickerButton>(screen_settings_body->find_child(String("SsCredStyleCol_") + k_sk[i], true, false))) {
			st["color"] = "#" + cp->get_pick_color().to_html(true);
		}
		styles[k_sk[i]] = st;
	}
	ccfg["styles"] = styles;
	dm->set_credits_config(ccfg);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_credits_item_selected(int p_idx) {
	if (updating_ui) return;
	ss_selected_cred_idx = p_idx;
	_refresh_screen_settings_panel();
}

void ClockGearsUiDesignEditor::_on_ss_credits_add_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !ss_cred_add_type_opt || ss_cred_add_type_opt->get_selected() < 0) return;
	String tp = String(ss_cred_add_type_opt->get_item_metadata(ss_cred_add_type_opt->get_selected()));
	Dictionary ccfg = dm->get_credits_config();
	Array items = Array(ccfg.get("items", Array())).duplicate(true);
	Dictionary item;
	item["type"] = tp;
	if (tp == "section") item["text"] = String::utf8("Nueva sección");
	else if (tp == "role_name") { item["role"] = "Rol"; item["name"] = "Nombre"; }
	else if (tp == "text") item["text"] = String::utf8("Línea de texto");
	else if (tp == "spacer") item["height"] = 32;
	items.push_back(item);
	ccfg["items"] = items;
	ss_selected_cred_idx = items.size() - 1;
	dm->set_credits_config(ccfg);
	_refresh_screen_settings_panel();
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_credits_del_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary ccfg = dm->get_credits_config();
	Array items = Array(ccfg.get("items", Array())).duplicate(true);
	if (ss_selected_cred_idx < 0 || ss_selected_cred_idx >= items.size()) return;
	items.remove_at(ss_selected_cred_idx);
	ss_selected_cred_idx = MIN(ss_selected_cred_idx, items.size() - 1);
	ccfg["items"] = items;
	dm->set_credits_config(ccfg);
	_refresh_screen_settings_panel();
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_credits_move_pressed(int p_delta) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary ccfg = dm->get_credits_config();
	Array items = Array(ccfg.get("items", Array())).duplicate(true);
	int tgt = ss_selected_cred_idx + p_delta;
	if (ss_selected_cred_idx < 0 || tgt < 0 || tgt >= items.size()) return;
	Variant tmp = items[ss_selected_cred_idx];
	items[ss_selected_cred_idx] = items[tgt];
	items[tgt] = tmp;
	ss_selected_cred_idx = tgt;
	ccfg["items"] = items;
	dm->set_credits_config(ccfg);
	_refresh_screen_settings_panel();
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_credits_item_field_changed() {
	if (updating_ui) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary ccfg = dm->get_credits_config();
	Array items = Array(ccfg.get("items", Array())).duplicate(true);
	if (ss_selected_cred_idx < 0 || ss_selected_cred_idx >= items.size()) return;
	Dictionary it = Dictionary(items[ss_selected_cred_idx]).duplicate(true);
	if (ss_cred_item_text_edit) it["text"] = ss_cred_item_text_edit->get_text();
	if (ss_cred_item_role_edit) it["role"] = ss_cred_item_role_edit->get_text();
	if (ss_cred_item_name_edit) it["name"] = ss_cred_item_name_edit->get_text();
	if (ss_cred_item_img_edit) it["image_path"] = ss_cred_item_img_edit->get_text().strip_edges();
	if (ss_cred_item_spacer_spin) it["height"] = int(ss_cred_item_spacer_spin->get_value());
	if (ss_cred_item_align_opt && ss_cred_item_align_opt->get_selected() >= 0) {
		it["align"] = String(ss_cred_item_align_opt->get_item_metadata(ss_cred_item_align_opt->get_selected()));
	}
	items[ss_selected_cred_idx] = it;
	ccfg["items"] = items;
	dm->set_credits_config(ccfg);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_ss_hud_add_widget_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !ss_hud_add_type_opt || ss_hud_add_type_opt->get_selected() < 0) return;
	String wtype = String(ss_hud_add_type_opt->get_item_metadata(ss_hud_add_type_opt->get_selected()));
	Dictionary nw = ClockGearsDataManager::make_default_hud_widget(wtype);
	String wid = dm->create_hud_widget(nw);
	if (!wid.is_empty()) {
		selected_elem_id = wid;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(wid);
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_ss_hud_sample_changed(double p_val) {
	if (updating_ui) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	int hp = ss_hud_sample_hp_spin ? int(ss_hud_sample_hp_spin->get_value()) : 75;
	int gold = ss_hud_sample_gold_spin ? int(ss_hud_sample_gold_spin->get_value()) : 250;
	int qty = ss_hud_sample_qty_spin ? int(ss_hud_sample_qty_spin->get_value()) : 3;

	int leader_id = 1;
	Array party = dm->get_party_actor_ids();
	if (!party.is_empty() && int(party[0]) > 0) leader_id = int(party[0]);
	dm->get_actor_runtime_state(leader_id);
	dm->set_actor_runtime_hp(leader_id, hp);
	dm->set_gold(gold);

	Array widgets = dm->get_hud_widgets();
	for (int i = 0; i < widgets.size(); i++) {
		Dictionary w = widgets[i];
		if (String(w.get("type", "")) == "item") {
			String ikey = String(w.get("item_key", "")).strip_edges();
			if (!ikey.is_empty()) {
				int cur = dm->get_item_qty(ikey);
				if (cur != qty) dm->change_item(ikey, qty - cur);
			}
		}
	}
	_refresh_canvas_stage();
}

void ClockGearsUiDesignEditor::_on_ss_gameover_changed() {
	if (updating_ui || !screen_settings_body) return;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary gcfg = dm->get_game_over_config();
	if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsGoTitleTextEdit", true, false))) {
		gcfg["title_text"] = ed->get_text();
	}
	if (ColorPickerButton *cp = Object::cast_to<ColorPickerButton>(screen_settings_body->find_child("SsGoTitleColBtn", true, false))) {
		gcfg["title_color"] = "#" + cp->get_pick_color().to_html(true);
	}
	if (SpinBox *sp = Object::cast_to<SpinBox>(screen_settings_body->find_child("SsGoTitleSizeSpin", true, false))) {
		gcfg["title_font_size"] = int(sp->get_value());
	}
	if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsGoBgmEdit", true, false))) {
		gcfg["bgm_path"] = ed->get_text().strip_edges();
		if (menu_bgm_edit) menu_bgm_edit->set_text(String(gcfg["bgm_path"]));
	}
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsGoRetryChk", true, false))) {
		gcfg["retry_enabled"] = chk->is_pressed();
	}
	if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsGoRetryEdit", true, false))) {
		gcfg["retry_label"] = ed->get_text();
	}
	if (CheckBox *chk = Object::cast_to<CheckBox>(screen_settings_body->find_child("SsGoToTitleChk", true, false))) {
		gcfg["title_enabled"] = chk->is_pressed();
	}
	if (LineEdit *ed = Object::cast_to<LineEdit>(screen_settings_body->find_child("SsGoToTitleEdit", true, false))) {
		gcfg["title_label"] = ed->get_text();
	}
	if (OptionButton *opt = Object::cast_to<OptionButton>(screen_settings_body->find_child("SsGoWinStyleOpt", true, false))) {
		if (opt->get_selected() >= 0) gcfg["window_style_id"] = String(opt->get_item_metadata(opt->get_selected()));
	}
	dm->set_game_over_config(gcfg);
	_refresh_canvas_stage();
	emit_signal("design_modified");
}

// ============================================================================
// Ventana flotante de elemento (9B Capas P4)
// ============================================================================

void ClockGearsUiDesignEditor::_on_layers_tree_item_activated() {
	if (!layers_tree) {
		return;
	}
	TreeItem *sel = layers_tree->get_selected();
	if (!sel) {
		return;
	}
	String eid = String(sel->get_metadata(0));
	if (!eid.is_empty()) {
		select_element(eid);
		open_floating_element_window(eid);
	}
}

void ClockGearsUiDesignEditor::_on_floating_window_closed(const String &p_key, ClockGearsUiElementFloatingWindow *p_win) {
	if (p_win) {
		last_floating_window_pos = p_win->get_position();
		last_floating_window_size = p_win->get_size();
		ClockGearsTheme::save_remembered_window_rect("ui_elem_floating", p_win);
		p_win->queue_free();
	}
	open_floating_windows.erase(p_key);
}

void ClockGearsUiDesignEditor::_sync_open_floating_windows() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Vector<String> to_erase;
	for (KeyValue<String, ClockGearsUiElementFloatingWindow *> &kv : open_floating_windows) {
		ClockGearsUiElementFloatingWindow *w = kv.value;
		if (!w) {
			to_erase.push_back(kv.key);
			continue;
		}
		String sid = w->get_target_screen_id();
		String eid = w->get_target_elem_id();
		Array elems = dm->get_ui_screen_elements(sid);
		if (elems.is_empty()) {
			elems = dm->get_default_ui_screen_template(sid).get("elements", Array());
		}
		String v_pid;
		int v_idx = -1;
		bool is_vslot = s_parse_virtual_slot_id(eid, v_pid, v_idx);
		String lookup_id = is_vslot ? v_pid : eid;
		Dictionary found;
		for (int i = 0; i < elems.size(); i++) {
			if (elems[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary d = elems[i];
			if (String(d.get("id", "")).to_lower() == lookup_id.to_lower()) {
				if (is_vslot) {
					Array vslots = s_collect_virtual_slot_dicts(dm, sid, d);
					if (v_idx >= 0 && v_idx < vslots.size() && vslots[v_idx].get_type() == Variant::DICTIONARY) {
						found = vslots[v_idx];
					}
				} else {
					found = d;
				}
				break;
			}
		}
		if (found.is_empty()) {
			w->hide();
			w->queue_free();
			to_erase.push_back(kv.key);
		} else {
			w->sync_from_element(found);
		}
	}
	for (int i = 0; i < to_erase.size(); i++) {
		open_floating_windows.erase(to_erase[i]);
	}
}

void ClockGearsUiDesignEditor::apply_floating_window_element_update(const String &p_screen_id, const String &p_elem_id, const Dictionary &p_next_elem) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || p_screen_id.is_empty() || p_elem_id.is_empty() || p_next_elem.is_empty()) {
		return;
	}
	String v_pid;
	int v_idx = -1;
	bool is_vslot = s_parse_virtual_slot_id(p_elem_id, v_pid, v_idx);
	auto apply_update = [&](bool p_record_undo) -> bool {
		if (!is_vslot) {
			return dm->update_ui_element(p_screen_id, p_elem_id, p_next_elem, p_record_undo);
		}
		Dictionary parent_el = dm->get_ui_element(p_screen_id, v_pid);
		if (parent_el.is_empty() && p_screen_id == current_screen_id) {
			parent_el = get_effective_element(v_pid);
		}
		if (s_write_virtual_slot_into_parent(p_screen_id, parent_el, v_idx, p_next_elem)) {
			return dm->update_ui_element(p_screen_id, v_pid, parent_el, p_record_undo);
		}
		return false;
	};
	if (p_screen_id == current_screen_id) {
		Dictionary snap = dm->get_ui_screen_design(current_screen_id);
		bool was_custom = dm->has_custom_ui_design(current_screen_id);
		ensure_screen_customizable_for_edit(false);
		if (apply_update(was_custom)) {
			if (!was_custom) {
				dm->push_ui_design_undo_state(current_screen_id, snap);
			}
			_trace_log("float_win_edit", "screen=" + p_screen_id + " elem=" + p_elem_id);
			_notify_data_mutated(true);
		}
	} else {
		apply_update(true);
		emit_signal("design_modified");
		_sync_open_floating_windows();
	}
}

Window *ClockGearsUiDesignEditor::open_floating_element_window(const String &p_elem_id) {
	String target_id = p_elem_id.is_empty() ? selected_elem_id : p_elem_id;
	if (target_id.is_empty()) {
		return nullptr;
	}
	Dictionary el = get_effective_element(target_id);
	if (el.is_empty()) {
		return nullptr;
	}
	String eid = String(el.get("id", target_id));
	String key = current_screen_id + "::" + eid.to_lower();
	if (open_floating_windows.has(key) && open_floating_windows[key]) {
		ClockGearsUiElementFloatingWindow *existing = open_floating_windows[key];
		existing->sync_from_element(el);
		existing->show();
		existing->grab_focus();
		return existing;
	}

	ClockGearsUiElementFloatingWindow *fw = memnew(ClockGearsUiElementFloatingWindow);
	add_child(fw);
	fw->configure(this, current_screen_id, eid, el);
	const Size2i def_sz(int(740 * EDSCALE), int(500 * EDSCALE));
	const Size2i min_sz(int(680 * EDSCALE), int(460 * EDSCALE));
	Rect2i rem = ClockGearsTheme::get_remembered_window_rect("ui_elem_floating");
	Size2i pref_sz = (last_floating_window_size.x >= min_sz.x && last_floating_window_size.y >= min_sz.y)
			? last_floating_window_size
			: ((rem.size.x >= min_sz.x && rem.size.y >= min_sz.y) ? rem.size : def_sz);
	int cascade = int(open_floating_windows.size()) * int(24 * EDSCALE);
	if (last_floating_window_pos != Vector2i(-1, -1)) {
		fw->set_position(last_floating_window_pos + Vector2i(cascade, cascade));
		fw->set_size(pref_sz);
		ClockGearsTheme::save_remembered_window_rect("ui_elem_floating", fw);
	} else if (rem.size.x >= min_sz.x && rem.size.y >= min_sz.y && cascade > 0) {
		fw->set_position(rem.position + Vector2i(cascade, cascade));
		fw->set_size(pref_sz);
		ClockGearsTheme::save_remembered_window_rect("ui_elem_floating", fw);
	}
	ClockGearsTheme::popup_fitted_window(fw, pref_sz, min_sz, "ui_elem_floating");
	Rect2i fitted(fw->get_position(), fw->get_size());
	if (cascade > 0 && last_floating_window_pos == Vector2i(-1, -1) && rem.size.x <= 0) {
		Rect2i usable = ClockGearsTheme::get_window_usable_rect(fw);
		Rect2i casc_rect = ClockGearsTheme::compute_fitted_popup_rect(
				usable,
				fitted.size,
				min_sz,
				Size2i(),
				fitted.position + Vector2i(cascade, cascade),
				true,
				fitted.size);
		fw->set_position(casc_rect.position);
		fw->set_size(casc_rect.size);
		fitted = casc_rect;
	}
	last_floating_window_pos = fitted.position;
	last_floating_window_size = fitted.size;
	fw->connect("close_requested", callable_mp(this, &ClockGearsUiDesignEditor::_on_floating_window_closed).bind(key, fw));
	open_floating_windows[key] = fw;
	_trace_log("float_win_open", "screen=" + current_screen_id + " elem=" + eid);
	return fw;
}

Window *ClockGearsUiDesignEditor::get_floating_window_for_element(const String &p_elem_id) const {
	String eid = p_elem_id.is_empty() ? selected_elem_id : p_elem_id;
	String key = current_screen_id + "::" + eid.to_lower();
	if (open_floating_windows.has(key)) {
		return open_floating_windows[key];
	}
	return nullptr;
}

void ClockGearsUiDesignEditor::close_all_floating_element_windows() {
	for (KeyValue<String, ClockGearsUiElementFloatingWindow *> &kv : open_floating_windows) {
		if (kv.value) {
			last_floating_window_pos = kv.value->get_position();
			last_floating_window_size = kv.value->get_size();
			kv.value->hide();
			kv.value->queue_free();
		}
	}
	open_floating_windows.clear();
	cancel_create_button_window();
}

int ClockGearsUiDesignEditor::get_open_floating_window_count() const {
	return int(open_floating_windows.size());
}

void ClockGearsUiDesignEditor::set_ghost_preview_element(const Dictionary &p_elem, bool p_enabled) {
	ghost_preview_active = p_enabled && !p_elem.is_empty();
	ghost_preview_elem = ghost_preview_active ? p_elem.duplicate(true) : Dictionary();
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
}

void ClockGearsUiDesignEditor::_on_create_floating_window_closed() {
	cancel_create_button_window();
}

Window *ClockGearsUiDesignEditor::open_create_button_window() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return nullptr;
	}
	Dictionary draft = dm->make_default_ui_element("button", current_screen_id);
	Array cur_els = get_effective_screen_elements();
	int top_l = 1;
	for (int i = 0; i < cur_els.size(); i++) {
		if (cur_els[i].get_type() == Variant::DICTIONARY) {
			top_l = MAX(top_l, int(Dictionary(cur_els[i]).get("layer_order", 0)));
		}
	}
	draft["layer_order"] = MIN(1000, top_l + 1);
	draft["id"] = "__creating_ghost__";
	draft["name"] = String::utf8("Nuevo botón");
	draft["text"] = String::utf8("Nuevo botón");
	draft["anchor"] = "center";
	draft["offset_x"] = 0;
	draft["offset_y"] = 0;
	draft["width"] = 180;
	draft["height"] = 44;
	Dictionary sh = ClockGearsDataManager::normalize_ui_shape(draft.get("shape", Dictionary()));
	sh["shape_type"] = "rounded";
	sh["corner_radius"] = 8;
	draft["shape"] = sh;

	if (!selected_elem_id.is_empty()) {
		Dictionary sel = get_effective_element(selected_elem_id);
		if (!sel.is_empty() && String(sel.get("type", "")) == "folder") {
			draft["parent_id"] = String(sel.get("id", selected_elem_id));
		}
	}

	if (!create_floating_window) {
		create_floating_window = memnew(ClockGearsUiElementFloatingWindow);
		add_child(create_floating_window);
		create_floating_window->connect("close_requested", callable_mp(this, &ClockGearsUiDesignEditor::_on_create_floating_window_closed));
	}
	create_floating_window->open_for_create_button(this, current_screen_id, draft);
	const Size2i def_sz(int(740 * EDSCALE), int(500 * EDSCALE));
	const Size2i min_sz(int(680 * EDSCALE), int(460 * EDSCALE));
	Rect2i rem = ClockGearsTheme::get_remembered_window_rect("ui_elem_floating");
	Size2i pref_sz = (last_floating_window_size.x >= min_sz.x && last_floating_window_size.y >= min_sz.y)
			? last_floating_window_size
			: ((rem.size.x >= min_sz.x && rem.size.y >= min_sz.y) ? rem.size : def_sz);
	if (last_floating_window_pos != Vector2i(-1, -1)) {
		create_floating_window->set_position(last_floating_window_pos);
		create_floating_window->set_size(pref_sz);
		ClockGearsTheme::save_remembered_window_rect("ui_elem_floating", create_floating_window);
	}
	ClockGearsTheme::popup_fitted_window(create_floating_window, pref_sz, min_sz, "ui_elem_floating");
	last_floating_window_pos = create_floating_window->get_position();
	last_floating_window_size = create_floating_window->get_size();
	create_floating_window->grab_focus();
	set_ghost_preview_element(draft, true);
	_trace_log("create_button_win_open", "screen=" + current_screen_id + " parent=" + String(draft.get("parent_id", "")));
	return create_floating_window;
}

Window *ClockGearsUiDesignEditor::get_create_button_window() const {
	return create_floating_window;
}

bool ClockGearsUiDesignEditor::confirm_create_button_from_draft(const Dictionary &p_draft) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	set_ghost_preview_element(Dictionary(), false);
	if (!dm || p_draft.is_empty()) {
		return false;
	}
	Dictionary el = p_draft.duplicate(true);
	el.erase("id");
	el["type"] = "button";
	if (String(el.get("name", "")).strip_edges().is_empty()) {
		el["name"] = String::utf8("Nuevo botón");
	}
	Dictionary snap = dm->get_ui_screen_design(current_screen_id);
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	ensure_screen_customizable_for_edit(false);
	String new_id = dm->add_ui_element(current_screen_id, el, was_custom);
	if (!new_id.is_empty()) {
		if (!was_custom) {
			dm->push_ui_design_undo_state(current_screen_id, snap);
		}
		selected_elem_id = new_id;
		selected_elem_ids.clear();
		selected_elem_ids.push_back(new_id);
		if (create_floating_window) {
			last_floating_window_pos = create_floating_window->get_position();
			last_floating_window_size = create_floating_window->get_size();
			create_floating_window->hide();
		}
		_trace_log("create_button_confirm", "screen=" + current_screen_id + " elem=" + new_id);
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

void ClockGearsUiDesignEditor::cancel_create_button_window() {
	set_ghost_preview_element(Dictionary(), false);
	if (create_floating_window) {
		if (create_floating_window->is_visible()) {
			last_floating_window_pos = create_floating_window->get_position();
			last_floating_window_size = create_floating_window->get_size();
			create_floating_window->hide();
		}
	}
}

void ClockGearsUiElementFloatingWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_target_screen_id"), &ClockGearsUiElementFloatingWindow::get_target_screen_id);
	ClassDB::bind_method(D_METHOD("get_target_elem_id"), &ClockGearsUiElementFloatingWindow::get_target_elem_id);
	ClassDB::bind_method(D_METHOD("get_ui_handles"), &ClockGearsUiElementFloatingWindow::get_ui_handles);
	ClassDB::bind_method(D_METHOD("is_creation_mode"), &ClockGearsUiElementFloatingWindow::is_creation_mode);
	ClassDB::bind_method(D_METHOD("confirm_creation"), &ClockGearsUiElementFloatingWindow::confirm_creation);
	ClassDB::bind_method(D_METHOD("cancel_creation"), &ClockGearsUiElementFloatingWindow::cancel_creation);
}

static String s_ui_elem_type_display_name(const String &p_type) {
	if (p_type == "button") return String::utf8("Botón");
	if (p_type == "folder") return "Carpeta";
	if (p_type == "image") return "Imagen";
	if (p_type == "text") return "Texto";
	if (p_type == "frame") return "Marco";
	if (p_type == "bar") return "Barra";
	if (p_type == "hearts") return "Corazones";
	if (p_type == "currency") return "Monedas";
	if (p_type == "item") return "Contador";
	if (p_type == "variable") return "Variable";
	if (p_type == "widget") return "Lista / Widget";
	if (p_type == "radial") return "Rueda radial";
	return p_type;
}

ClockGearsUiElementFloatingWindow::ClockGearsUiElementFloatingWindow() {
	set_name("UiElementFloatingWindow");
	set_title("Propiedades del elemento");
	set_transient(true);
	set_exclusive(false);
	set_wrap_controls(false);
	set_min_size(Size2i(int(760 * EDSCALE), int(560 * EDSCALE)));
	_build_ui();
}

void ClockGearsUiElementFloatingWindow::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		refresh_theme();
	}
}

void ClockGearsUiElementFloatingWindow::refresh_theme() {
	if (ClockGearsTheme *ct = ClockGearsTheme::get_singleton()) {
		if (bg_root) {
			ct->apply_to_control(bg_root);
		}
		if (!ct->is_connected("theme_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_theme_changed).unbind(1))) {
			ct->connect("theme_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_theme_changed).unbind(1));
		}
	}
	if (bg_root) {
		bg_root->set_theme_type_variation("ClockGearsContainer");
	}
}

void ClockGearsUiElementFloatingWindow::_on_theme_changed() {
	refresh_theme();
}

void ClockGearsUiElementFloatingWindow::_build_ui() {
	bg_root = memnew(PanelContainer);
	bg_root->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	add_child(bg_root);

	MarginContainer *mc = memnew(MarginContainer);
	mc->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	mc->add_theme_constant_override("margin_left", int(8 * EDSCALE));
	mc->add_theme_constant_override("margin_right", int(8 * EDSCALE));
	mc->add_theme_constant_override("margin_top", int(8 * EDSCALE));
	mc->add_theme_constant_override("margin_bottom", int(8 * EDSCALE));
	bg_root->add_child(mc);

	VBoxContainer *root_vb = memnew(VBoxContainer);
	root_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	mc->add_child(root_vb);

	tabs = memnew(TabContainer);
	tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_vb->add_child(tabs);

	create_footer = memnew(HBoxContainer);
	create_footer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	create_footer->set_alignment(BoxContainer::ALIGNMENT_END);
	create_footer->add_theme_constant_override("separation", int(8 * EDSCALE));
	create_footer->hide();
	root_vb->add_child(create_footer);

	create_cancel_btn = memnew(Button("Cancelar"));
	create_cancel_btn->set_custom_minimum_size(Size2(104 * EDSCALE, 34 * EDSCALE));
	create_cancel_btn->set_clip_text(false);
	create_cancel_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	create_cancel_btn->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_create_cancel_pressed));
	create_footer->add_child(create_cancel_btn);

	create_confirm_btn = memnew(Button("Crear"));
	create_confirm_btn->set_custom_minimum_size(Size2(116 * EDSCALE, 34 * EDSCALE));
	create_confirm_btn->set_clip_text(false);
	create_confirm_btn->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	create_confirm_btn->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_create_confirm_pressed));
	create_footer->add_child(create_confirm_btn);

	connect("window_input", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_win_unhandled_key));

	auto make_tab_scroll = [&](const String &p_tab_name) -> VBoxContainer * {
		ScrollContainer *sc = memnew(ScrollContainer);
		sc->set_name(p_tab_name);
		sc->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		sc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		MarginContainer *tab_mc = memnew(MarginContainer);
		tab_mc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		tab_mc->add_theme_constant_override("margin_left", int(4 * EDSCALE));
		tab_mc->add_theme_constant_override("margin_right", int(8 * EDSCALE));
		tab_mc->add_theme_constant_override("margin_top", int(4 * EDSCALE));
		tab_mc->add_theme_constant_override("margin_bottom", int(6 * EDSCALE));
		sc->add_child(tab_mc);
		VBoxContainer *vb = memnew(VBoxContainer);
		vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		vb->add_theme_constant_override("separation", int(6 * EDSCALE));
		tab_mc->add_child(vb);
		tabs->add_child(sc);
		return vb;
	};
	auto add_row = [&](VBoxContainer *p_parent, const String &p_label, Control *p_ctrl) {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
		hb->add_theme_constant_override("separation", int(8 * EDSCALE));
		Label *l = memnew(Label(p_label));
		l->set_custom_minimum_size(Size2(130 * EDSCALE, 32 * EDSCALE));
		l->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		l->set_clip_text(false);
		l->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		hb->add_child(l);
		p_ctrl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		Size2 cms = p_ctrl->get_custom_minimum_size();
		if (cms.y < 32.0f * EDSCALE) {
			p_ctrl->set_custom_minimum_size(Size2(cms.x, 32.0f * EDSCALE));
		}
		hb->add_child(p_ctrl);
		p_parent->add_child(hb);
	};

	// --- Tab 1: General ---
	VBoxContainer *tb_gen = make_tab_scroll("General");
	name_edit = memnew(LineEdit);
	name_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_gen, "Nombre:", name_edit);

	quick_shape_opt = memnew(OptionButton);
	quick_shape_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	quick_shape_opt->set_fit_to_longest_item(false);
	quick_shape_opt->set_clip_text(true);
	quick_shape_opt->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	quick_shape_opt->add_item("Rectangular", 0);
	quick_shape_opt->set_item_metadata(0, "rectangular");
	quick_shape_opt->add_item("Cuadrado", 1);
	quick_shape_opt->set_item_metadata(1, "cuadrado");
	quick_shape_opt->add_item("Redondo", 2);
	quick_shape_opt->set_item_metadata(2, "redondo");
	quick_shape_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_quick_shape_selected));
	add_row(tb_gen, String::utf8("Forma rápida:"), quick_shape_opt);
	quick_shape_row = Object::cast_to<HBoxContainer>(quick_shape_opt->get_parent());
	if (quick_shape_row) {
		quick_shape_row->hide();
	}

	type_lbl = memnew(Label("-"));
	add_row(tb_gen, "Tipo:", type_lbl);

	HFlowContainer *flags_hb = memnew(HFlowContainer);
	flags_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	flags_hb->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	flags_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	visible_chk = memnew(CheckBox("Visible"));
	visible_chk->set_clip_text(false);
	visible_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	flags_hb->add_child(visible_chk);
	locked_chk = memnew(CheckBox("Bloqueado"));
	locked_chk->set_clip_text(false);
	locked_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	flags_hb->add_child(locked_chk);
	clip_children_chk = memnew(CheckBox("Recortar hijos"));
	clip_children_chk->set_clip_text(false);
	clip_children_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	flags_hb->add_child(clip_children_chk);
	tb_gen->add_child(flags_hb);

	GridContainer *anc_grid = memnew(GridContainer);
	anc_grid->set_columns(3);
	anc_grid->add_theme_constant_override("h_separation", int(3 * EDSCALE));
	anc_grid->add_theme_constant_override("v_separation", int(3 * EDSCALE));
	static const char *k_ag[9] = { "NO", "N", "NE", "O", "C", "E", "SO", "S", "SE" };
	for (int i = 0; i < 9; i++) {
		Button *ab = memnew(Button(k_ag[i]));
		ab->set_toggle_mode(true);
		ab->set_custom_minimum_size(Size2(34 * EDSCALE, 24 * EDSCALE));
		ab->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_anchor_pressed).bind(i));
		anc_grid->add_child(ab);
		anchor_btns[i] = ab;
	}
	add_row(tb_gen, "Anclaje:", anc_grid);

	auto make_spin = [&](VBoxContainer *p_parent, const String &p_lbl, int p_min, int p_max, SpinBox *&r_sp) {
		r_sp = memnew(SpinBox);
		r_sp->set_min(p_min);
		r_sp->set_max(p_max);
		r_sp->set_step(1);
		r_sp->connect("value_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
		add_row(p_parent, p_lbl, r_sp);
	};
	make_spin(tb_gen, String::utf8("Posición X:"), -960, 960, ox_spin);
	make_spin(tb_gen, String::utf8("Posición Y:"), -540, 540, oy_spin);
	make_spin(tb_gen, "Ancho:", 16, 960, w_spin);
	make_spin(tb_gen, "Alto:", 16, 540, h_spin);
	make_spin(tb_gen, "Capa:", -100, 1000, layer_spin);

	auto setup_opt = [&](OptionButton *p_opt) {
		p_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		p_opt->set_fit_to_longest_item(false);
		p_opt->set_clip_text(true);
		p_opt->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		p_opt->set_custom_minimum_size(Size2(120 * EDSCALE, 32 * EDSCALE));
	};

	slot_vis_mode_opt = memnew(OptionButton);
	setup_opt(slot_vis_mode_opt);
	slot_vis_mode_opt->add_item("Siempre visible", 0); slot_vis_mode_opt->set_item_metadata(0, "always");
	slot_vis_mode_opt->add_item("Si interruptor ON", 1); slot_vis_mode_opt->set_item_metadata(1, "switch");
	slot_vis_mode_opt->add_item("Oculto", 2); slot_vis_mode_opt->set_item_metadata(2, "never");
	slot_vis_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_gen, "Visibilidad:", slot_vis_mode_opt);

	slot_vis_sw_edit = memnew(LineEdit);
	slot_vis_sw_edit->set_placeholder("Interruptor visibilidad (opcional)");
	slot_vis_sw_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_gen, "Int. visible:", slot_vis_sw_edit);

	slot_en_mode_opt = memnew(OptionButton);
	setup_opt(slot_en_mode_opt);
	slot_en_mode_opt->add_item("Siempre activo", 0); slot_en_mode_opt->set_item_metadata(0, "always");
	slot_en_mode_opt->add_item("Si interruptor ON", 1); slot_en_mode_opt->set_item_metadata(1, "switch");
	slot_en_mode_opt->add_item("Desactivado", 2); slot_en_mode_opt->set_item_metadata(2, "disabled");
	slot_en_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_gen, "Estado activo:", slot_en_mode_opt);

	slot_en_sw_edit = memnew(LineEdit);
	slot_en_sw_edit->set_placeholder("Interruptor estado activo");
	slot_en_sw_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_gen, "Int. activo:", slot_en_sw_edit);

	// --- Tab 2: Aspecto ---
	VBoxContainer *tb_asp = make_tab_scroll("Aspecto");
	show_bg_chk = memnew(CheckBox("Mostrar fondo"));
	show_bg_chk->set_clip_text(false);
	show_bg_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Fondo:", show_bg_chk);

	style_opt = memnew(OptionButton);
	setup_opt(style_opt);
	style_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Estilo ventana:", style_opt);

	shape_opt = memnew(OptionButton);
	setup_opt(shape_opt);
	shape_opt->add_item(String::utf8("Rectángulo"), 0); shape_opt->set_item_metadata(0, "rect");
	shape_opt->add_item("Cuadrado", 1); shape_opt->set_item_metadata(1, "square");
	shape_opt->add_item("Redondeado", 2); shape_opt->set_item_metadata(2, "rounded");
	shape_opt->add_item(String::utf8("Píldora"), 3); shape_opt->set_item_metadata(3, "pill");
	shape_opt->add_item(String::utf8("Círculo"), 4); shape_opt->set_item_metadata(4, "circle");
	shape_opt->add_item("Elipse", 5); shape_opt->set_item_metadata(5, "ellipse");
	shape_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Forma:", shape_opt);

	HBoxContainer *rad_hb = memnew(HBoxContainer);
	rad_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rad_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	radius_slider = memnew(HSlider);
	radius_slider->set_min(0); radius_slider->set_max(64); radius_slider->set_step(1);
	radius_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	radius_slider->connect("value_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	rad_hb->add_child(radius_slider);
	radius_val_lbl = memnew(Label("8px"));
	radius_val_lbl->set_custom_minimum_size(Size2(44 * EDSCALE, 0));
	rad_hb->add_child(radius_val_lbl);
	add_row(tb_asp, "Radio esquinas:", rad_hb);

	fill_color_btn = memnew(ColorPickerButton);
	fill_color_btn->set_edit_alpha(true);
	fill_color_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	fill_color_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Relleno:", fill_color_btn);

	grad_chk = memnew(CheckBox("Activar degradado"));
	grad_chk->set_clip_text(false);
	grad_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Degradado:", grad_chk);

	HBoxContainer *gr_hb = memnew(HBoxContainer);
	gr_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gr_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	grad_top_btn = memnew(ColorPickerButton);
	grad_top_btn->set_edit_alpha(true);
	grad_top_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grad_top_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	grad_top_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	gr_hb->add_child(grad_top_btn);
	grad_bot_btn = memnew(ColorPickerButton);
	grad_bot_btn->set_edit_alpha(true);
	grad_bot_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grad_bot_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	grad_bot_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	gr_hb->add_child(grad_bot_btn);
	add_row(tb_asp, "Colores degr.:", gr_hb);

	border_w_slider = memnew(HSlider);
	border_w_slider->set_min(0); border_w_slider->set_max(12); border_w_slider->set_step(1);
	border_w_slider->connect("value_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Grosor borde:", border_w_slider);

	border_col_btn = memnew(ColorPickerButton);
	border_col_btn->set_edit_alpha(true);
	border_col_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	border_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_asp, "Color borde:", border_col_btn);

	HFlowContainer *ef_hb = memnew(HFlowContainer);
	ef_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ef_hb->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	ef_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	shadow_chk = memnew(CheckBox("Sombra"));
	shadow_chk->set_clip_text(false);
	shadow_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	ef_hb->add_child(shadow_chk);
	glow_chk = memnew(CheckBox("Brillo"));
	glow_chk->set_clip_text(false);
	glow_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	ef_hb->add_child(glow_chk);
	glow_col_btn = memnew(ColorPickerButton);
	glow_col_btn->set_edit_alpha(true);
	glow_col_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 28 * EDSCALE));
	glow_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	ef_hb->add_child(glow_col_btn);
	add_row(tb_asp, "Sombra / Brillo:", ef_hb);

	HBoxContainer *op_hb = memnew(HBoxContainer);
	op_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	op_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	opacity_slider = memnew(HSlider);
	opacity_slider->set_min(0); opacity_slider->set_max(100); opacity_slider->set_step(5);
	opacity_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opacity_slider->connect("value_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	op_hb->add_child(opacity_slider);
	opacity_val_lbl = memnew(Label("100%"));
	opacity_val_lbl->set_custom_minimum_size(Size2(46 * EDSCALE, 0));
	op_hb->add_child(opacity_val_lbl);
	add_row(tb_asp, "Opacidad:", op_hb);

	auto make_img_row = [&](VBoxContainer *p_parent, const String &p_lbl, LineEdit *&r_edit, int p_pick_target) {
		HBoxContainer *ihb = memnew(HBoxContainer);
		ihb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ihb->add_theme_constant_override("separation", int(6 * EDSCALE));
		r_edit = memnew(LineEdit);
		r_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_edit->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
		r_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
		ihb->add_child(r_edit);
		Button *ipk = memnew(Button(String::utf8("Elegir…")));
		ipk->set_h_size_flags(Control::SIZE_SHRINK_END);
		ipk->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
		ipk->set_clip_text(false);
		ipk->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_open_img_picker).bind(p_pick_target));
		ihb->add_child(ipk);
		Button *iclr = memnew(Button("X"));
		iclr->set_h_size_flags(Control::SIZE_SHRINK_END);
		iclr->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
		iclr->set_clip_text(false);
		iclr->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_clear_img_picker).bind(p_pick_target));
		ihb->add_child(iclr);
		add_row(p_parent, p_lbl, ihb);
	};

	// --- Tab 3: Distribución ---
	VBoxContainer *tb_dist = make_tab_scroll(String::utf8("Distribución"));
	dist_radial_box = memnew(VBoxContainer);
	dist_radial_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dist_radial_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	tb_dist->add_child(dist_radial_box);

	rad_count_src_opt = memnew(OptionButton);
	setup_opt(rad_count_src_opt);
	rad_count_src_opt->add_item("Manual", 0); rad_count_src_opt->set_item_metadata(0, "manual");
	rad_count_src_opt->add_item("Desde datos", 1); rad_count_src_opt->set_item_metadata(1, "data");
	rad_count_src_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Origen ranuras:", rad_count_src_opt);

	make_spin(dist_radial_box, "Ranuras radial:", 1, 16, rad_count_spin);
	make_spin(dist_radial_box, "Radio (px):", 48, 250, rad_radius_spin);
	make_spin(dist_radial_box, String::utf8("Ángulo inicio:"), -360, 360, rad_start_spin);

	rad_full_chk = memnew(CheckBox(String::utf8("Círculo completo (360°)")));
	rad_full_chk->set_clip_text(false);
	rad_full_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Arco:", rad_full_chk);

	make_spin(dist_radial_box, String::utf8("Barrido (°):"), 45, 360, rad_sweep_spin);

	rad_cw_chk = memnew(CheckBox("Sentido horario"));
	rad_cw_chk->set_clip_text(false);
	rad_cw_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, String::utf8("Dirección:"), rad_cw_chk);

	make_spin(dist_radial_box, String::utf8("Tamaño ranura:"), 28, 140, rad_slot_size_spin);
	make_spin(dist_radial_box, "Espaciado:", 0, 64, rad_slot_spacing_spin);

	rad_slot_shape_opt = memnew(OptionButton);
	setup_opt(rad_slot_shape_opt);
	rad_slot_shape_opt->add_item(String::utf8("Círculo"), 0); rad_slot_shape_opt->set_item_metadata(0, "circle");
	rad_slot_shape_opt->add_item("Sector (tarta)", 1); rad_slot_shape_opt->set_item_metadata(1, "sector");
	rad_slot_shape_opt->add_item(String::utf8("Rectángulo"), 2); rad_slot_shape_opt->set_item_metadata(2, "rectangle");
	rad_slot_shape_opt->add_item("Imagen", 3); rad_slot_shape_opt->set_item_metadata(3, "image");
	rad_slot_shape_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Forma ranuras:", rad_slot_shape_opt);

	rad_sel_scale_spin = memnew(SpinBox);
	rad_sel_scale_spin->set_min(1.0);
	rad_sel_scale_spin->set_max(1.8);
	rad_sel_scale_spin->set_step(0.05);
	rad_sel_scale_spin->connect("value_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Escala selec.:", rad_sel_scale_spin);

	rad_sel_border_col_btn = memnew(ColorPickerButton);
	rad_sel_border_col_btn->set_edit_alpha(true);
	rad_sel_border_col_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	rad_sel_border_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Borde selec.:", rad_sel_border_col_btn);

	make_spin(dist_radial_box, "Grosor selec.:", 0, 12, rad_sel_border_w_spin);

	rad_sel_glow_chk = memnew(CheckBox("Brillo en ranura activa"));
	rad_sel_glow_chk->set_clip_text(false);
	rad_sel_glow_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Resaltado:", rad_sel_glow_chk);

	make_img_row(dist_radial_box, "Img. selec.:", rad_sel_img_edit, 4);

	rad_center_shape_opt = memnew(OptionButton);
	setup_opt(rad_center_shape_opt);
	rad_center_shape_opt->add_item(String::utf8("Círculo"), 0); rad_center_shape_opt->set_item_metadata(0, "circle");
	rad_center_shape_opt->add_item("Cuadro", 1); rad_center_shape_opt->set_item_metadata(1, "square");
	rad_center_shape_opt->add_item("Imagen", 2); rad_center_shape_opt->set_item_metadata(2, "image");
	rad_center_shape_opt->add_item("Ninguno", 3); rad_center_shape_opt->set_item_metadata(3, "none");
	rad_center_shape_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Centro radial:", rad_center_shape_opt);

	make_img_row(dist_radial_box, "Img. centro:", rad_center_img_edit, 5);

	HFlowContainer *rc_fl = memnew(HFlowContainer);
	rc_fl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rc_fl->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	rad_center_icon_chk = memnew(CheckBox("Icono"));
	rad_center_icon_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	rc_fl->add_child(rad_center_icon_chk);
	rad_center_title_chk = memnew(CheckBox(String::utf8("Título")));
	rad_center_title_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	rc_fl->add_child(rad_center_title_chk);
	rad_center_desc_chk = memnew(CheckBox(String::utf8("Descripción")));
	rad_center_desc_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	rc_fl->add_child(rad_center_desc_chk);
	add_row(dist_radial_box, "Info centro:", rc_fl);

	make_img_row(dist_radial_box, "Img. anillo:", rad_ring_img_edit, 6);

	rad_ring_col_btn = memnew(ColorPickerButton);
	rad_ring_col_btn->set_edit_alpha(true);
	rad_ring_col_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	rad_ring_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Color anillo:", rad_ring_col_btn);

	make_spin(dist_radial_box, "Grosor anillo:", 0, 160, rad_ring_thick_spin);

	rad_ind_enabled_chk = memnew(CheckBox("Mostrar puntero giratorio"));
	rad_ind_enabled_chk->set_clip_text(false);
	rad_ind_enabled_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Indicador:", rad_ind_enabled_chk);

	rad_ind_col_btn = memnew(ColorPickerButton);
	rad_ind_col_btn->set_edit_alpha(true);
	rad_ind_col_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	rad_ind_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_radial_box, "Color puntero:", rad_ind_col_btn);

	make_img_row(dist_radial_box, "Img. puntero:", rad_ind_img_edit, 7);

	dist_grid_box = memnew(VBoxContainer);
	dist_grid_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dist_grid_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	tb_dist->add_child(dist_grid_box);

	cont_layout_opt = memnew(OptionButton);
	setup_opt(cont_layout_opt);
	cont_layout_opt->add_item(String::utf8("Cuadrícula"), 0); cont_layout_opt->set_item_metadata(0, "grid");
	cont_layout_opt->add_item("Lista vertical", 1); cont_layout_opt->set_item_metadata(1, "vertical_list");
	cont_layout_opt->add_item("Lista horizontal", 2); cont_layout_opt->set_item_metadata(2, "horizontal_list");
	cont_layout_opt->add_item("Carrusel", 3); cont_layout_opt->set_item_metadata(3, "carousel");
	cont_layout_opt->add_item(String::utf8("Pestañas"), 4); cont_layout_opt->set_item_metadata(4, "tabs");
	cont_layout_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_grid_box, String::utf8("Disposición:"), cont_layout_opt);

	make_spin(dist_grid_box, "Columnas:", 1, 20, cont_cols_spin);
	make_spin(dist_grid_box, "Filas:", 0, 20, cont_rows_spin);
	make_spin(dist_grid_box, "Espacio X:", 0, 128, cont_spx_spin);
	make_spin(dist_grid_box, "Espacio Y:", 0, 128, cont_spy_spin);

	cont_scroll_chk = memnew(CheckBox("Permitir desplazamiento (scroll)"));
	cont_scroll_chk->set_clip_text(false);
	cont_scroll_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(dist_grid_box, "Desplazar:", cont_scroll_chk);

	// --- Tab 4: Elementos ---
	VBoxContainer *tb_el = make_tab_scroll("Elementos");
	el_data_source_opt = memnew(OptionButton);
	setup_opt(el_data_source_opt);
	Array ds_list = ClockGearsDataManager::get_ui_container_data_sources();
	for (int i = 0; i < ds_list.size(); i++) {
		Dictionary dsd = ds_list[i];
		el_data_source_opt->add_item(String(dsd.get("label", dsd.get("id", ""))), i);
		el_data_source_opt->set_item_metadata(i, String(dsd.get("id", "manual")));
	}
	el_data_source_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_el, "Fuente datos:", el_data_source_opt);

	el_filter_opt = memnew(OptionButton);
	setup_opt(el_filter_opt);
	el_filter_opt->add_item(String::utf8("(Todas las categorías)"), 0); el_filter_opt->set_item_metadata(0, String(""));
	el_filter_opt->add_item("Consumibles", 1); el_filter_opt->set_item_metadata(1, "consumables");
	el_filter_opt->add_item("Objetos clave", 2); el_filter_opt->set_item_metadata(2, "key");
	el_filter_opt->add_item("Materiales", 3); el_filter_opt->set_item_metadata(3, "material");
	el_filter_opt->add_item("Armas", 4); el_filter_opt->set_item_metadata(4, "weapons");
	el_filter_opt->add_item("Armaduras", 5); el_filter_opt->set_item_metadata(5, "armors");
	el_filter_opt->add_item("Escudos", 6); el_filter_opt->set_item_metadata(6, "shields");
	el_filter_opt->add_item("Accesorios", 7); el_filter_opt->set_item_metadata(7, "accessories");
	el_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_el, "Filtrar por:", el_filter_opt);

	el_usable_chk = memnew(CheckBox("Solo utilizables"));
	el_usable_chk->set_clip_text(false);
	el_usable_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_el, "Uso:", el_usable_chk);

	el_sort_opt = memnew(OptionButton);
	setup_opt(el_sort_opt);
	el_sort_opt->add_item("Orden inventario / ID", 0); el_sort_opt->set_item_metadata(0, "id");
	el_sort_opt->add_item("Nombre (A-Z)", 1); el_sort_opt->set_item_metadata(1, "name");
	el_sort_opt->add_item("Cantidad", 2); el_sort_opt->set_item_metadata(2, "qty");
	el_sort_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_el, "Ordenar por:", el_sort_opt);

	make_spin(tb_el, "Ranura equipo:", 0, 15, el_equip_slot_spin);

	el_show_empty_chk = memnew(CheckBox(String::utf8("Mostrar huecos vacíos")));
	el_show_empty_chk->set_clip_text(false);
	el_show_empty_chk->connect("toggled", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_el, "Huecos:", el_show_empty_chk);

	make_spin(tb_el, String::utf8("Cant. ranuras/vacíos:"), 0, 64, el_empty_cnt_spin);

	el_slot_action_opt = memnew(OptionButton);
	setup_opt(el_slot_action_opt);
	el_slot_action_opt->add_item("Abrir detalle", 0); el_slot_action_opt->set_item_metadata(0, "show_detail");
	el_slot_action_opt->add_item("Usar objeto", 1); el_slot_action_opt->set_item_metadata(1, "item_use");
	el_slot_action_opt->add_item("Descartar objeto", 2); el_slot_action_opt->set_item_metadata(2, "item_discard");
	el_slot_action_opt->add_item("Equipar pieza", 3); el_slot_action_opt->set_item_metadata(3, "equip_apply");
	el_slot_action_opt->add_item(String::utf8("Cambiar pestaña"), 4); el_slot_action_opt->set_item_metadata(4, "switch_tab");
	el_slot_action_opt->add_item("Ninguna", 5); el_slot_action_opt->set_item_metadata(5, String(""));
	el_slot_action_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_el, String::utf8("Al aceptar ranura:"), el_slot_action_opt);

	HBoxContainer *el_btns = memnew(HBoxContainer);
	el_btns->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	el_btns->add_theme_constant_override("separation", int(8 * EDSCALE));
	el_create_tpl_btn = memnew(Button("Crear plantilla del elemento"));
	el_create_tpl_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	el_create_tpl_btn->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	el_create_tpl_btn->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_create_tpl_pressed));
	el_btns->add_child(el_create_tpl_btn);
	el_apply_all_btn = memnew(Button("Aplicar a todas"));
	el_apply_all_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	el_apply_all_btn->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	el_apply_all_btn->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_apply_all_slots_pressed));
	el_btns->add_child(el_apply_all_btn);
	tb_el->add_child(el_btns);

	// --- Tab 5: Imagen ---
	VBoxContainer *tb_img = make_tab_scroll("Imagen");
	HBoxContainer *mi_hb = memnew(HBoxContainer);
	mi_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mi_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	image_edit = memnew(LineEdit);
	image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	image_edit->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
	image_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	mi_hb->add_child(image_edit);
	Button *mi_pick = memnew(Button(String::utf8("Elegir…")));
	mi_pick->set_h_size_flags(Control::SIZE_SHRINK_END);
	mi_pick->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
	mi_pick->set_clip_text(false);
	mi_pick->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_open_img_picker).bind(-1));
	mi_hb->add_child(mi_pick);
	Button *mi_clr = memnew(Button("X"));
	mi_clr->set_h_size_flags(Control::SIZE_SHRINK_END);
	mi_clr->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
	mi_clr->set_clip_text(false);
	mi_clr->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_clear_img_picker).bind(-1));
	mi_hb->add_child(mi_clr);
	add_row(tb_img, "Imagen:", mi_hb);

	HFlowContainer *sc_flow = memnew(HFlowContainer);
	sc_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sc_flow->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	sc_flow->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	scale_opt = memnew(OptionButton);
	setup_opt(scale_opt);
	scale_opt->set_custom_minimum_size(Size2(140 * EDSCALE, 32 * EDSCALE));
	scale_opt->add_item("Ajustar", 0); scale_opt->set_item_metadata(0, "fit");
	scale_opt->add_item("Rellenar", 1); scale_opt->set_item_metadata(1, "fill");
	scale_opt->add_item("Estirar", 2); scale_opt->set_item_metadata(2, "stretch");
	scale_opt->add_item("Mosaico", 3); scale_opt->set_item_metadata(3, "tile");
	scale_opt->add_item("Nativo", 4); scale_opt->set_item_metadata(4, "native");
	scale_opt->add_item("9 partes", 5); scale_opt->set_item_metadata(5, "nine_slice");
	scale_opt->add_item(String::utf8("Múltiplo entero"), 6); scale_opt->set_item_metadata(6, "integer");
	scale_opt->add_item("Manual", 7); scale_opt->set_item_metadata(7, "manual");
	scale_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	sc_flow->add_child(scale_opt);
	adjust_manual_btn = memnew(Button("Ajustar manualmente"));
	adjust_manual_btn->set_h_size_flags(Control::SIZE_SHRINK_END);
	adjust_manual_btn->set_custom_minimum_size(Size2(168 * EDSCALE, 32 * EDSCALE));
	adjust_manual_btn->set_clip_text(false);
	adjust_manual_btn->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_adjust_manual_pressed));
	sc_flow->add_child(adjust_manual_btn);
	add_row(tb_img, "Escalado:", sc_flow);

	make_spin(tb_img, "Margen 9-slice:", 0, 128, nine_slice_spin);

	filter_opt = memnew(OptionButton);
	setup_opt(filter_opt);
	filter_opt->add_item("Pixelado (Nearest)", 0); filter_opt->set_item_metadata(0, true);
	filter_opt->add_item("Suave (Lineal)", 1); filter_opt->set_item_metadata(1, false);
	filter_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_img, "Filtro pixel art:", filter_opt);

	static const char *k_bstate_labels[4] = { "Estado normal:", "Estado encima:", "Estado pulsado:", "Estado desactiv.:" };
	for (int si = 0; si < 4; si++) {
		HBoxContainer *bhb = memnew(HBoxContainer);
		bhb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		bhb->add_theme_constant_override("separation", int(6 * EDSCALE));
		btn_state_edits[si] = memnew(LineEdit);
		btn_state_edits[si]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		btn_state_edits[si]->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
		btn_state_edits[si]->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
		bhb->add_child(btn_state_edits[si]);
		Button *bp = memnew(Button(String::utf8("Elegir…")));
		bp->set_h_size_flags(Control::SIZE_SHRINK_END);
		bp->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
		bp->set_clip_text(false);
		bp->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_open_img_picker).bind(si));
		bhb->add_child(bp);
		Button *bc = memnew(Button("X"));
		bc->set_h_size_flags(Control::SIZE_SHRINK_END);
		bc->set_custom_minimum_size(Size2(34 * EDSCALE, 32 * EDSCALE));
		bc->set_clip_text(false);
		bc->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_clear_img_picker).bind(si));
		bhb->add_child(bc);
		add_row(tb_img, k_bstate_labels[si], bhb);
	}

	img_file_dialog = memnew(FileDialog);
	img_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	img_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	img_file_dialog->add_filter("*.png, *.webp, *.jpg, *.svg", String::utf8("Imágenes"));
	img_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_img_file_selected));
	add_child(img_file_dialog);

	// --- Tab 4: Texto ---
	VBoxContainer *tb_txt = make_tab_scroll("Texto");
	text_edit = memnew(TextEdit);
	text_edit->set_custom_minimum_size(Size2(0, 68 * EDSCALE));
	text_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
	text_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed));
	add_row(tb_txt, "Contenido:", text_edit);

	desc_edit = memnew(LineEdit);
	desc_edit->set_placeholder(String::utf8("Descripción de la ranura / detalle"));
	desc_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_txt, String::utf8("Descripción:"), desc_edit);

	HBoxContainer *fh = memnew(HBoxContainer);
	fh->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	fh->add_theme_constant_override("separation", int(6 * EDSCALE));
	font_opt = memnew(OptionButton);
	setup_opt(font_opt);
	font_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	fh->add_child(font_opt);
	Button *f_imp = memnew(Button(String::utf8("Importar…")));
	f_imp->set_h_size_flags(Control::SIZE_SHRINK_END);
	f_imp->set_custom_minimum_size(Size2(104 * EDSCALE, 32 * EDSCALE));
	f_imp->set_clip_text(false);
	f_imp->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_open_font_picker));
	fh->add_child(f_imp);
	add_row(tb_txt, "Fuente:", fh);

	make_spin(tb_txt, String::utf8("Tamaño:"), 8, 96, font_size_spin);

	font_color_btn = memnew(ColorPickerButton);
	font_color_btn->set_edit_alpha(true);
	font_color_btn->set_custom_minimum_size(Size2(48 * EDSCALE, 32 * EDSCALE));
	font_color_btn->connect("color_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_txt, "Color texto:", font_color_btn);

	align_opt = memnew(OptionButton);
	setup_opt(align_opt);
	align_opt->add_item("Izquierda", 0); align_opt->set_item_metadata(0, "left");
	align_opt->add_item("Centro", 1); align_opt->set_item_metadata(1, "center");
	align_opt->add_item("Derecha", 2); align_opt->set_item_metadata(2, "right");
	align_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_txt, String::utf8("Alineación:"), align_opt);

	binding_opt = memnew(OptionButton);
	setup_opt(binding_opt);
	Array b_list = ClockGearsDataManager::get_ui_data_bindings();
	for (int i = 0; i < b_list.size(); i++) {
		Dictionary bd = b_list[i];
		binding_opt->add_item(String(bd.get("label", "")), i);
		binding_opt->set_item_metadata(i, String(bd.get("id", "none")));
	}
	binding_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_txt, String::utf8("Enlace dinámico:"), binding_opt);

	var_edit = memnew(LineEdit);
	var_edit->set_custom_minimum_size(Size2(96 * EDSCALE, 32 * EDSCALE));
	var_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_txt, "Variable:", var_edit);

	font_file_dialog = memnew(FileDialog);
	font_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	font_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	font_file_dialog->add_filter("*.ttf, *.otf, *.woff2", "Fuentes");
	font_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_font_file_selected));
	add_child(font_file_dialog);

	// --- Tab 7: Animación ---
	VBoxContainer *tb_anim = make_tab_scroll(String::utf8("Animación"));
	anim_type_opt = memnew(OptionButton);
	setup_opt(anim_type_opt);
	anim_type_opt->add_item("Giro radial", 0); anim_type_opt->set_item_metadata(0, "radial_spin");
	anim_type_opt->add_item("Fundido", 1); anim_type_opt->set_item_metadata(1, "fade");
	anim_type_opt->add_item("Escala", 2); anim_type_opt->set_item_metadata(2, "scale");
	anim_type_opt->add_item("Desde izquierda", 3); anim_type_opt->set_item_metadata(3, "slide_left");
	anim_type_opt->add_item("Desde derecha", 4); anim_type_opt->set_item_metadata(4, "slide_right");
	anim_type_opt->add_item("Desde arriba", 5); anim_type_opt->set_item_metadata(5, "slide_top");
	anim_type_opt->add_item("Desde abajo", 6); anim_type_opt->set_item_metadata(6, "slide_bottom");
	anim_type_opt->add_item("Ninguna", 7); anim_type_opt->set_item_metadata(7, "none");
	anim_type_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_anim, "Apertura:", anim_type_opt);

	make_spin(tb_anim, String::utf8("Duración (ms):"), 80, 1500, anim_dur_spin);
	anim_dur_spin->set_step(10);

	anim_curve_opt = memnew(OptionButton);
	setup_opt(anim_curve_opt);
	anim_curve_opt->add_item("Suave (Ease Out)", 0); anim_curve_opt->set_item_metadata(0, "ease_out");
	anim_curve_opt->add_item("Lineal", 1); anim_curve_opt->set_item_metadata(1, "linear");
	anim_curve_opt->add_item(String::utf8("Rebote atrás (Back Out)"), 2); anim_curve_opt->set_item_metadata(2, "back_out");
	anim_curve_opt->add_item("Rebote (Bounce)", 3); anim_curve_opt->set_item_metadata(3, "bounce");
	anim_curve_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_anim, "Curva:", anim_curve_opt);

	snd_open_edit = memnew(LineEdit);
	snd_open_edit->set_placeholder("res://audio/sfx/... (al abrir)");
	snd_open_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_anim, "Sonido abrir:", snd_open_edit);

	snd_move_edit = memnew(LineEdit);
	snd_move_edit->set_placeholder("res://audio/sfx/... (al mover cursor)");
	snd_move_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_anim, "Sonido mover:", snd_move_edit);

	snd_accept_edit = memnew(LineEdit);
	snd_accept_edit->set_placeholder("res://audio/sfx/... (al aceptar)");
	snd_accept_edit->connect("text_changed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(tb_anim, "Sonido aceptar:", snd_accept_edit);

	// --- Tab 8: Acciones ---
	actions_host = make_tab_scroll("Acciones");
	quick_action_opt = memnew(OptionButton);
	setup_opt(quick_action_opt);
	quick_action_opt->connect("item_selected", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_any_changed).unbind(1));
	add_row(actions_host, String::utf8("Acción rápida:"), quick_action_opt);

	HBoxContainer *add_act_hb = memnew(HBoxContainer);
	add_act_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_act_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	fw_add_action_opt = memnew(OptionButton);
	setup_opt(fw_add_action_opt);
	Array all_acts = ClockGearsDataManager::get_all_ui_button_action_types();
	for (int i = 0; i < all_acts.size(); i++) {
		Dictionary ad = all_acts[i];
		fw_add_action_opt->add_item(String(ad.get("label", ad.get("id", ""))), i);
		fw_add_action_opt->set_item_metadata(i, String(ad.get("id", "")));
	}
	add_act_hb->add_child(fw_add_action_opt);
	fw_add_action_btn = memnew(Button(String::utf8("+ Añadir")));
	fw_add_action_btn->set_h_size_flags(Control::SIZE_SHRINK_END);
	fw_add_action_btn->set_custom_minimum_size(Size2(92 * EDSCALE, 32 * EDSCALE));
	fw_add_action_btn->set_clip_text(false);
	fw_add_action_btn->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_fw_add_action_pressed));
	add_act_hb->add_child(fw_add_action_btn);
	add_row(actions_host, "Nuevo paso:", add_act_hb);

	fw_actions_list_vbox = memnew(VBoxContainer);
	fw_actions_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	fw_actions_list_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	actions_host->add_child(fw_actions_list_vbox);

	actions_summary_lbl = memnew(Label("Sin lista de comandos configurada."));
	actions_summary_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	actions_host->add_child(actions_summary_lbl);
}

void ClockGearsUiElementFloatingWindow::configure(ClockGearsUiDesignEditor *p_owner, const String &p_screen_id, const String &p_elem_id, const Dictionary &p_elem) {
	creation_mode = false;
	if (create_footer) {
		create_footer->hide();
	}
	if (quick_shape_row) {
		quick_shape_row->hide();
	}
	owner_editor = p_owner;
	target_screen_id = p_screen_id;
	target_elem_id = p_elem_id;
	sync_from_element(p_elem);
}

void ClockGearsUiElementFloatingWindow::open_for_create_button(ClockGearsUiDesignEditor *p_owner, const String &p_screen_id, const Dictionary &p_draft_elem) {
	creation_mode = true;
	owner_editor = p_owner;
	target_screen_id = p_screen_id;
	target_elem_id = String(p_draft_elem.get("id", "__creating_ghost__"));
	if (create_footer) {
		create_footer->show();
	}
	if (quick_shape_row) {
		quick_shape_row->show();
	}
	sync_from_element(p_draft_elem);
	if (tabs) {
		tabs->set_current_tab(0);
	}
	set_title(String::utf8("Crear botón"));
	if (type_lbl) {
		String pid = String(p_draft_elem.get("parent_id", "")).strip_edges();
		type_lbl->set_text(pid.is_empty() ? String::utf8("Botón nuevo") : String::utf8("Botón nuevo (en carpeta seleccionada)"));
	}
}

void ClockGearsUiElementFloatingWindow::confirm_creation() {
	if (!creation_mode || !owner_editor) {
		return;
	}
	_emit_commit();
	Dictionary final_draft = working_elem.duplicate(true);
	creation_mode = false;
	if (create_footer) {
		create_footer->hide();
	}
	if (quick_shape_row) {
		quick_shape_row->hide();
	}
	owner_editor->confirm_create_button_from_draft(final_draft);
}

void ClockGearsUiElementFloatingWindow::cancel_creation() {
	if (!creation_mode) {
		return;
	}
	creation_mode = false;
	if (create_footer) {
		create_footer->hide();
	}
	if (quick_shape_row) {
		quick_shape_row->hide();
	}
	if (owner_editor) {
		owner_editor->cancel_create_button_window();
	}
}

void ClockGearsUiElementFloatingWindow::_on_create_confirm_pressed() {
	confirm_creation();
}

void ClockGearsUiElementFloatingWindow::_on_create_cancel_pressed() {
	cancel_creation();
}

void ClockGearsUiElementFloatingWindow::_on_win_unhandled_key(const Ref<InputEvent> &p_ev) {
	if (!creation_mode) {
		return;
	}
	Ref<InputEventKey> k = p_ev;
	if (k.is_valid() && k->is_pressed() && !k->is_echo() && k->get_keycode() == Key::ESCAPE) {
		set_input_as_handled();
		cancel_creation();
	}
}

void ClockGearsUiElementFloatingWindow::_on_quick_shape_selected(int p_idx) {
	if (updating || !quick_shape_opt) {
		return;
	}
	String kind = String(quick_shape_opt->get_item_metadata(p_idx));
	bool prev = updating;
	updating = true;
	if (kind == "cuadrado") {
		if (shape_opt) s_select_option_meta(shape_opt, "square");
		if (radius_slider) radius_slider->set_value(8);
		if (w_spin) w_spin->set_value(96);
		if (h_spin) h_spin->set_value(96);
	} else if (kind == "redondo") {
		if (shape_opt) s_select_option_meta(shape_opt, "circle");
		if (radius_slider) radius_slider->set_value(48);
		if (w_spin) w_spin->set_value(96);
		if (h_spin) h_spin->set_value(96);
	} else {
		if (shape_opt) s_select_option_meta(shape_opt, "rounded");
		if (radius_slider) radius_slider->set_value(8);
		if (w_spin) w_spin->set_value(180);
		if (h_spin) h_spin->set_value(44);
	}
	updating = prev;
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::sync_from_element(const Dictionary &p_elem) {
	if (p_elem.is_empty()) {
		return;
	}
	working_elem = p_elem.duplicate(true);
	bool prev = updating;
	updating = true;

	String nm = String(p_elem.get("name", target_elem_id));
	String tp = String(p_elem.get("type", "text"));
	String tp_disp = s_ui_elem_type_display_name(tp);
	set_title(creation_mode ? String::utf8("Crear botón") : (nm + String::utf8(" · ") + tp_disp));
	if (name_edit && name_edit->get_text() != nm) name_edit->set_text(nm);
	if (type_lbl && !creation_mode) type_lbl->set_text(tp_disp + " (" + target_elem_id + ")");
	if (quick_shape_opt) {
		Dictionary sh_qs = p_elem.get("shape", Dictionary());
		String st_qs = String(sh_qs.get("shape_type", "rounded")).strip_edges().to_lower();
		if (st_qs == "circle") quick_shape_opt->select(2);
		else if (st_qs == "square") quick_shape_opt->select(1);
		else quick_shape_opt->select(0);
	}
	if (visible_chk) visible_chk->set_pressed(bool(p_elem.get("visible", true)));
	if (locked_chk) locked_chk->set_pressed(bool(p_elem.get("locked", false)));
	if (clip_children_chk) clip_children_chk->set_pressed(bool(p_elem.get("clip_children", false)));

	String cur_anc = String(p_elem.get("anchor", "top_left"));
	static const char *k_ak[9] = { "top_left", "top_center", "top_right", "middle_left", "center", "middle_right", "bottom_left", "bottom_center", "bottom_right" };
	for (int i = 0; i < 9; i++) {
		if (anchor_btns[i]) anchor_btns[i]->set_pressed(cur_anc == k_ak[i]);
	}
	if (ox_spin) ox_spin->set_value(int(p_elem.get("offset_x", 0)));
	if (oy_spin) oy_spin->set_value(int(p_elem.get("offset_y", 0)));
	if (w_spin) w_spin->set_value(int(p_elem.get("width", 120)));
	if (h_spin) h_spin->set_value(int(p_elem.get("height", 36)));
	if (layer_spin) layer_spin->set_value(int(p_elem.get("layer_order", 0)));
	if (slot_vis_mode_opt) s_select_option_meta(slot_vis_mode_opt, String(p_elem.get("visibility_mode", "always")));
	if (slot_vis_sw_edit && slot_vis_sw_edit->get_text() != String(p_elem.get("visibility_switch", ""))) {
		slot_vis_sw_edit->set_text(String(p_elem.get("visibility_switch", "")));
	}
	if (slot_en_mode_opt) s_select_option_meta(slot_en_mode_opt, String(p_elem.get("enabled_mode", "always")));
	if (slot_en_sw_edit && slot_en_sw_edit->get_text() != String(p_elem.get("enabled_switch", ""))) {
		slot_en_sw_edit->set_text(String(p_elem.get("enabled_switch", "")));
	}

	Dictionary lay = (p_elem.has("layout") && p_elem["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(p_elem["layout"]) : Dictionary();
	ClockGearsDataManager *dm_fw = ClockGearsDataManager::get_singleton();
	bool is_rad_btn_child = false;
	if (tp == "button" && dm_fw && !target_screen_id.is_empty()) {
		String pid = String(p_elem.get("parent_id", "")).strip_edges();
		if (!pid.is_empty()) {
			Dictionary pel = dm_fw->get_ui_element(target_screen_id, pid);
			if (String(pel.get("type", "")).strip_edges().to_lower() == "radial") {
				is_rad_btn_child = true;
			}
		}
	}
	bool is_rad = (tp == "radial") || (tp == "slot" && String(p_elem.get("_slot_parent_type", "")) == "radial") || is_rad_btn_child;
	bool is_cont = (tp == "container") || (tp == "slot" && String(p_elem.get("_slot_parent_type", "")) == "container");
	if (dist_radial_box) dist_radial_box->set_visible(is_rad);
	if (dist_grid_box) dist_grid_box->set_visible(is_cont || !is_rad);
	if (rad_count_src_opt) s_select_option_meta(rad_count_src_opt, String(lay.get("radial_slot_count_source", "manual")));
	Array rslots = (lay.has("radial_slots") && lay["radial_slots"].get_type() == Variant::ARRAY) ? Array(lay["radial_slots"]) : Array();
	int child_btn_cnt = 0;
	if (tp == "radial" && dm_fw && !target_screen_id.is_empty()) {
		Array sc_els = dm_fw->get_ui_screen_elements(target_screen_id);
		String tid_low = target_elem_id.to_lower();
		for (int bi = 0; bi < sc_els.size(); bi++) {
			if (sc_els[bi].get_type() != Variant::DICTIONARY) continue;
			Dictionary bd = sc_els[bi];
			if (String(bd.get("type", "")).to_lower() == "button" && String(bd.get("parent_id", "")).to_lower() == tid_low) {
				child_btn_cnt++;
			}
		}
	}
	if (rad_count_spin) rad_count_spin->set_value(child_btn_cnt > 0 ? child_btn_cnt : (rslots.is_empty() ? 6 : rslots.size()));
	if (rad_radius_spin) rad_radius_spin->set_value(int(lay.get("radial_radius", 128)));
	if (rad_start_spin) rad_start_spin->set_value(float(lay.get("radial_start_angle_deg", -90.0)));
	float rsweep = float(lay.get("radial_sweep_angle_deg", 360.0));
	if (rad_full_chk) rad_full_chk->set_pressed(rsweep >= 359.5f);
	if (rad_sweep_spin) {
		rad_sweep_spin->set_value(rsweep);
		rad_sweep_spin->set_editable(!(rad_full_chk && rad_full_chk->is_pressed()));
	}
	if (rad_cw_chk) rad_cw_chk->set_pressed(lay.has("radial_clockwise") ? bool(lay["radial_clockwise"]) : true);
	if (rad_slot_size_spin) rad_slot_size_spin->set_value(int(lay.get("radial_slot_size", 56)));
	if (rad_slot_spacing_spin) rad_slot_spacing_spin->set_value(int(lay.get("radial_slot_spacing", 0)));
	if (rad_slot_shape_opt) s_select_option_meta(rad_slot_shape_opt, String(lay.get("radial_slot_shape", "circle")));
	if (rad_sel_scale_spin) rad_sel_scale_spin->set_value(float(lay.get("radial_selected_scale", 1.28)));
	String rsb = String(lay.get("radial_selected_border_color", "#f5bc54ff"));
	if (rad_sel_border_col_btn && Color::html_is_valid(rsb)) rad_sel_border_col_btn->set_pick_color(Color::html(rsb));
	if (rad_sel_border_w_spin) rad_sel_border_w_spin->set_value(int(lay.get("radial_selected_border_width", 3)));
	if (rad_sel_glow_chk) rad_sel_glow_chk->set_pressed(lay.has("radial_selected_glow") ? bool(lay["radial_selected_glow"]) : true);
	if (rad_sel_img_edit && rad_sel_img_edit->get_text() != String(lay.get("radial_selected_image", ""))) {
		rad_sel_img_edit->set_text(String(lay.get("radial_selected_image", "")));
	}
	if (rad_center_shape_opt) s_select_option_meta(rad_center_shape_opt, String(lay.get("radial_center_shape", "circle")));
	if (rad_center_img_edit && rad_center_img_edit->get_text() != String(lay.get("radial_center_image", ""))) {
		rad_center_img_edit->set_text(String(lay.get("radial_center_image", "")));
	}
	if (rad_center_icon_chk) rad_center_icon_chk->set_pressed(lay.has("radial_center_show_icon") ? bool(lay["radial_center_show_icon"]) : true);
	if (rad_center_title_chk) rad_center_title_chk->set_pressed(lay.has("radial_center_show_title") ? bool(lay["radial_center_show_title"]) : true);
	if (rad_center_desc_chk) rad_center_desc_chk->set_pressed(lay.has("radial_center_show_desc") ? bool(lay["radial_center_show_desc"]) : true);
	if (rad_ring_img_edit && rad_ring_img_edit->get_text() != String(lay.get("radial_ring_image", ""))) {
		rad_ring_img_edit->set_text(String(lay.get("radial_ring_image", "")));
	}
	String rrc = String(lay.get("radial_ring_color", "#1a1e2ad9"));
	if (rad_ring_col_btn && Color::html_is_valid(rrc)) rad_ring_col_btn->set_pick_color(Color::html(rrc));
	if (rad_ring_thick_spin) rad_ring_thick_spin->set_value(int(lay.get("radial_ring_thickness", 0)));
	if (rad_ind_enabled_chk) rad_ind_enabled_chk->set_pressed(lay.has("radial_indicator_enabled") ? bool(lay["radial_indicator_enabled"]) : true);
	String ric = String(lay.get("radial_indicator_color", "#f5bc54ff"));
	if (rad_ind_col_btn && Color::html_is_valid(ric)) rad_ind_col_btn->set_pick_color(Color::html(ric));
	if (rad_ind_img_edit && rad_ind_img_edit->get_text() != String(lay.get("radial_indicator_image", ""))) {
		rad_ind_img_edit->set_text(String(lay.get("radial_indicator_image", "")));
	}
	if (cont_layout_opt) s_select_option_meta(cont_layout_opt, String(lay.get("mode", "grid")));
	if (cont_cols_spin) cont_cols_spin->set_value(int(lay.get("columns", 4)));
	if (cont_rows_spin) cont_rows_spin->set_value(int(lay.get("rows", 0)));
	if (cont_spx_spin) cont_spx_spin->set_value(int(lay.get("spacing_x", 10)));
	if (cont_spy_spin) cont_spy_spin->set_value(int(lay.get("spacing_y", 10)));
	if (cont_scroll_chk) cont_scroll_chk->set_pressed(lay.has("scroll_enabled") ? bool(lay["scroll_enabled"]) : true);

	if (el_data_source_opt) s_select_option_meta(el_data_source_opt, String(p_elem.get("data_source", "manual")));
	if (el_filter_opt) s_select_option_meta(el_filter_opt, String(p_elem.get("data_filter_category", "")));
	if (el_usable_chk) el_usable_chk->set_pressed(bool(p_elem.get("data_only_usable", false)));
	if (el_sort_opt) s_select_option_meta(el_sort_opt, String(p_elem.get("data_sort_by", "id")));
	if (el_equip_slot_spin) el_equip_slot_spin->set_value(int(p_elem.get("data_equip_slot", 0)));
	if (el_show_empty_chk) el_show_empty_chk->set_pressed(p_elem.has("data_show_empty") ? bool(p_elem["data_show_empty"]) : true);
	if (el_empty_cnt_spin) el_empty_cnt_spin->set_value(int(p_elem.get("data_empty_count", 8)));
	if (el_slot_action_opt) s_select_option_meta(el_slot_action_opt, String(p_elem.get("slot_action", "show_detail")));
	if (el_create_tpl_btn) el_create_tpl_btn->set_visible(tp == "container");
	if (el_apply_all_btn) el_apply_all_btn->set_visible(tp == "slot" || tp == "radial" || tp == "container" || is_rad_btn_child);

	if (anim_type_opt) s_select_option_meta(anim_type_opt, String(lay.get("radial_open_anim", p_elem.get("open_anim", "radial_spin"))));
	if (anim_dur_spin) anim_dur_spin->set_value(int(lay.get("radial_open_duration_ms", p_elem.get("open_duration_ms", 220))));
	if (anim_curve_opt) s_select_option_meta(anim_curve_opt, String(lay.get("radial_open_curve", "ease_out")));
	if (snd_open_edit && snd_open_edit->get_text() != String(lay.get("radial_sound_open", p_elem.get("sound_open", "")))) {
		snd_open_edit->set_text(String(lay.get("radial_sound_open", p_elem.get("sound_open", ""))));
	}
	if (snd_move_edit && snd_move_edit->get_text() != String(lay.get("radial_sound_move", p_elem.get("sound_move", "")))) {
		snd_move_edit->set_text(String(lay.get("radial_sound_move", p_elem.get("sound_move", ""))));
	}
	if (snd_accept_edit && snd_accept_edit->get_text() != String(lay.get("radial_sound_accept", p_elem.get("sound_accept", "")))) {
		snd_accept_edit->set_text(String(lay.get("radial_sound_accept", p_elem.get("sound_accept", ""))));
	}

	if (show_bg_chk) show_bg_chk->set_pressed(bool(p_elem.get("show_background", false)));
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (style_opt && dm) {
		style_opt->clear();
		style_opt->add_item("(Por defecto del tema)", 0);
		style_opt->set_item_metadata(0, String(""));
		Array ws_list = dm->get_window_styles();
		for (int i = 0; i < ws_list.size(); i++) {
			Dictionary ws = ws_list[i];
			style_opt->add_item(String(ws.get("name", ws.get("id", ""))), i + 1);
			style_opt->set_item_metadata(i + 1, String(ws.get("id", "")));
		}
		s_select_option_meta(style_opt, String(p_elem.get("window_style_id", "")));
	}
	Dictionary st_ui = s_extract_elem_shape_ui_state(dm, p_elem);
	if (shape_opt) s_select_option_meta(shape_opt, String(st_ui.get("shape_type", "rounded")));
	int crad = int(st_ui.get("corner_radius", 8));
	if (radius_slider) radius_slider->set_value(crad);
	if (radius_val_lbl) radius_val_lbl->set_text(itos(crad) + "px");
	String fc = String(st_ui.get("fill_color", "#232a3aff"));
	if (fill_color_btn && Color::html_is_valid(fc)) fill_color_btn->set_pick_color(Color::html(fc));
	if (grad_chk) grad_chk->set_pressed(bool(st_ui.get("gradient_enabled", false)));
	String gt = String(st_ui.get("gradient_top_color", "#2b3348ff"));
	String gb = String(st_ui.get("gradient_bottom_color", "#181d2aff"));
	if (grad_top_btn && Color::html_is_valid(gt)) grad_top_btn->set_pick_color(Color::html(gt));
	if (grad_bot_btn && Color::html_is_valid(gb)) grad_bot_btn->set_pick_color(Color::html(gb));
	if (border_w_slider) border_w_slider->set_value(int(st_ui.get("border_width", 2)));
	String bc = String(st_ui.get("border_color", "#d8a040ff"));
	if (border_col_btn && Color::html_is_valid(bc)) border_col_btn->set_pick_color(Color::html(bc));
	if (shadow_chk) shadow_chk->set_pressed(bool(st_ui.get("shadow_enabled", false)));
	if (glow_chk) glow_chk->set_pressed(bool(st_ui.get("glow_enabled", false)));
	String gc = String(st_ui.get("glow_color", "#f0b44cff"));
	if (glow_col_btn && Color::html_is_valid(gc)) glow_col_btn->set_pick_color(Color::html(gc));
	int op_pct = int(Math::round(CLAMP(float(p_elem.get("opacity", 1.0)), 0.0f, 1.0f) * 100.0f));
	if (opacity_slider) opacity_slider->set_value(op_pct);
	if (opacity_val_lbl) opacity_val_lbl->set_text(itos(op_pct) + "%");

	String ikey = (tp == "currency" || tp == "item") ? "icon_path" : "image_path";
	String ip = String(p_elem.get(ikey, ""));
	if (ip.is_empty() && tp == "button") ip = String(p_elem.get("icon_path", ""));
	if (image_edit && image_edit->get_text() != ip) image_edit->set_text(ip);
	if (scale_opt) s_select_option_meta(scale_opt, String(p_elem.get("scale_mode", "stretch")));
	s_sync_manual_adjust_btn_state(adjust_manual_btn, String(p_elem.get("scale_mode", "stretch")), s_elem_has_any_image(p_elem));
	if (nine_slice_spin) nine_slice_spin->set_value(int(p_elem.get("nine_slice_margin", 12)));
	bool pa = p_elem.has("pixel_art") ? bool(p_elem["pixel_art"]) : true;
	if (filter_opt) filter_opt->select(pa ? 0 : 1);
	static const char *k_sk[4] = { "image_normal", "image_hover", "image_pressed", "image_disabled" };
	for (int si = 0; si < 4; si++) {
		String sv = String(p_elem.get(k_sk[si], ""));
		if (btn_state_edits[si] && btn_state_edits[si]->get_text() != sv) {
			btn_state_edits[si]->set_text(sv);
		}
	}

	String txt = (tp == "bar") ? String(p_elem.get("label_text", "")) : String(p_elem.get("text", ""));
	if (text_edit && text_edit->get_text() != txt) text_edit->set_text(txt);
	String dsc = String(p_elem.get("description", ""));
	if (desc_edit && desc_edit->get_text() != dsc) desc_edit->set_text(dsc);
	if (font_opt) {
		font_opt->clear();
		font_opt->add_item("(Fuente por defecto del tema)", 0);
		font_opt->set_item_metadata(0, String(""));
		if (owner_editor) {
			if (OptionButton *main_fo = Object::cast_to<OptionButton>(owner_editor->get_ui_handles().get("prop_font_opt", Variant()))) {
				for (int i = 0; i < main_fo->get_item_count(); i++) {
					String mp = String(main_fo->get_item_metadata(i));
					if (mp.is_empty() && i == 0) continue;
					int idx = font_opt->get_item_count();
					font_opt->add_item(main_fo->get_item_text(i), idx);
					font_opt->set_item_metadata(idx, mp);
				}
			}
		}
		s_select_option_meta(font_opt, String(p_elem.get("font_path", "")));
	}
	if (font_size_spin) font_size_spin->set_value(int(p_elem.get("font_size", 16)));
	String fcol = String(p_elem.get("font_color", "#FFFFFFFF"));
	if (font_color_btn && Color::html_is_valid(fcol)) font_color_btn->set_pick_color(Color::html(fcol));
	if (align_opt) s_select_option_meta(align_opt, String(p_elem.get("align", "left")));
	if (binding_opt) s_select_option_meta(binding_opt, String(p_elem.get("data_binding", "none")));
	if (var_edit && var_edit->get_text() != String(p_elem.get("variable_name", ""))) {
		var_edit->set_text(String(p_elem.get("variable_name", "")));
	}

	if (quick_action_opt) {
		quick_action_opt->clear();
		Array act_list = ClockGearsDataManager::get_ui_button_actions(target_screen_id.is_empty() ? String("title") : target_screen_id);
		for (int i = 0; i < act_list.size(); i++) {
			Dictionary ad = act_list[i];
			quick_action_opt->add_item(String(ad.get("label", ad.get("id", ""))), i);
			quick_action_opt->set_item_metadata(i, String(ad.get("id", "")));
		}
		s_select_option_meta(quick_action_opt, String(p_elem.get("action", "")));
	}
	_refresh_fw_actions_list();

	updating = prev;
}

void ClockGearsUiElementFloatingWindow::_on_anchor_pressed(int p_idx) {
	if (updating) return;
	static const char *k_ak[9] = { "top_left", "top_center", "top_right", "middle_left", "center", "middle_right", "bottom_left", "bottom_center", "bottom_right" };
	int ci = CLAMP(p_idx, 0, 8);
	bool prev = updating;
	updating = true;
	for (int i = 0; i < 9; i++) {
		if (anchor_btns[i]) anchor_btns[i]->set_pressed(i == ci);
	}
	updating = prev;
	working_elem["anchor"] = String(k_ak[ci]);
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_on_any_changed() {
	if (updating) return;
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_emit_commit() {
	if (!owner_editor || target_elem_id.is_empty()) {
		return;
	}
	Dictionary el = working_elem.duplicate(true);
	String tp = String(el.get("type", "text"));
	if (name_edit) el["name"] = name_edit->get_text();
	if (visible_chk) el["visible"] = visible_chk->is_pressed();
	if (locked_chk) el["locked"] = locked_chk->is_pressed();
	if (clip_children_chk) el["clip_children"] = clip_children_chk->is_pressed();
	if (ox_spin) el["offset_x"] = int(Math::round(ox_spin->get_value()));
	if (oy_spin) el["offset_y"] = int(Math::round(oy_spin->get_value()));
	if (w_spin) el["width"] = int(Math::round(w_spin->get_value()));
	if (h_spin) el["height"] = int(Math::round(h_spin->get_value()));
	if (layer_spin) el["layer_order"] = int(Math::round(layer_spin->get_value()));
	if (slot_vis_mode_opt && slot_vis_mode_opt->get_selected() >= 0) {
		String vm = String(slot_vis_mode_opt->get_item_metadata(slot_vis_mode_opt->get_selected()));
		bool prev_vis_chk = bool(working_elem.get("visible", true));
		bool cur_vis_chk = visible_chk ? visible_chk->is_pressed() : true;
		String prev_vm = String(working_elem.get("visibility_mode", "always"));
		if (cur_vis_chk != prev_vis_chk) {
			vm = cur_vis_chk ? "always" : "never";
		} else if (vm != prev_vm) {
			el["visible"] = (vm != "never" && vm != "hidden");
		}
		el["visibility_mode"] = vm;
	}
	if (slot_vis_sw_edit) el["visibility_switch"] = slot_vis_sw_edit->get_text().strip_edges();
	if (slot_en_mode_opt && slot_en_mode_opt->get_selected() >= 0) el["enabled_mode"] = String(slot_en_mode_opt->get_item_metadata(slot_en_mode_opt->get_selected()));
	if (slot_en_sw_edit) el["enabled_switch"] = slot_en_sw_edit->get_text().strip_edges();

	bool is_rad_btn_child = false;
	if (tp == "button" && ClockGearsDataManager::get_singleton() && !target_screen_id.is_empty()) {
		String pid = String(el.get("parent_id", "")).strip_edges();
		if (!pid.is_empty()) {
			Dictionary pel = ClockGearsDataManager::get_singleton()->get_ui_element(target_screen_id, pid);
			if (String(pel.get("type", "")).strip_edges().to_lower() == "radial") {
				is_rad_btn_child = true;
			}
		}
	}
	bool is_rad = (tp == "radial") || (tp == "slot" && String(el.get("_slot_parent_type", "")) == "radial") || is_rad_btn_child;
	if (tp == "radial" || tp == "container" || tp == "slot" || is_rad_btn_child) {
		Dictionary lay = (el.has("layout") && el["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(el["layout"]).duplicate(true) : Dictionary();
		if (is_rad) {
			if (rad_count_src_opt && rad_count_src_opt->get_selected() >= 0) lay["radial_slot_count_source"] = String(rad_count_src_opt->get_item_metadata(rad_count_src_opt->get_selected()));
			if (rad_count_spin && tp == "radial") {
				int target_cnt = CLAMP(int(Math::round(rad_count_spin->get_value())), 1, 16);
				Array cur_slots = (lay.has("radial_slots") && lay["radial_slots"].get_type() == Variant::ARRAY) ? Array(lay["radial_slots"]).duplicate(true) : Array();
				if (cur_slots.size() != target_cnt) {
					Array def_slots = ClockGearsDataManager::make_default_radial_slots(target_cnt, target_screen_id);
					Array next_slots;
					for (int i = 0; i < target_cnt; i++) {
						next_slots.push_back(i < cur_slots.size() ? cur_slots[i] : def_slots[i]);
					}
					lay["radial_slots"] = next_slots;
				}
				// Sincronizar también botones hijo reales cuando data_source es manual
				String cur_ds = (el_data_source_opt && el_data_source_opt->get_selected() >= 0)
						? String(el_data_source_opt->get_item_metadata(el_data_source_opt->get_selected()))
						: String(el.get("data_source", "manual"));
				if (ClockGearsDataManager::normalize_ui_data_source(cur_ds) == "manual") {
					if (ClockGearsDataManager *dm_c = ClockGearsDataManager::get_singleton()) {
						Dictionary sd_full = dm_c->get_ui_screen_design(target_screen_id);
						Array sc_els = sd_full.has("elements") && sd_full["elements"].get_type() == Variant::ARRAY
								? Array(sd_full["elements"]).duplicate(true)
								: Array();
						Vector<int> kid_indices;
						String tid_low = target_elem_id.to_lower();
						HashSet<String> used_ids;
						for (int i = 0; i < sc_els.size(); i++) {
							if (sc_els[i].get_type() != Variant::DICTIONARY) continue;
							Dictionary kd = sc_els[i];
							used_ids.insert(String(kd.get("id", "")));
							if (String(kd.get("type", "")).to_lower() == "button" && String(kd.get("parent_id", "")).to_lower() == tid_low) {
								kid_indices.push_back(i);
							}
						}
						if (kid_indices.size() != target_cnt) {
							if (kid_indices.size() > target_cnt) {
								for (int r = kid_indices.size() - 1; r >= target_cnt; r--) {
									sc_els.remove_at(kid_indices[r]);
								}
							} else {
								Array def_s = ClockGearsDataManager::make_default_radial_slots(target_cnt, target_screen_id);
								int base_l = int(el.get("layer_order", 1)) + 1;
								for (int add_i = kid_indices.size(); add_i < target_cnt; add_i++) {
									Dictionary sl = (add_i < def_s.size()) ? Dictionary(def_s[add_i]) : Dictionary();
									Dictionary nb = dm_c->make_default_ui_element("button", target_screen_id);
									String bid = target_elem_id + "_slot_" + itos(add_i + 1);
									int suf = 2;
									while (used_ids.has(bid)) {
										bid = target_elem_id + "_slot_" + itos(add_i + 1) + "_" + itos(suf++);
									}
									used_ids.insert(bid);
									nb["id"] = bid;
									nb["parent_id"] = target_elem_id;
									nb["role"] = "radial_slot";
									nb["slot_index"] = add_i;
									String ttl = String(sl.get("title", vformat("Ranura %d", add_i + 1)));
									nb["name"] = ttl;
									nb["text"] = ttl;
									nb["action"] = String(sl.get("action", ""));
									if (sl.has("actions")) nb["actions"] = Array(sl["actions"]).duplicate(true);
									nb["layer_order"] = base_l + add_i;
									nb["focus_order"] = add_i;
									sc_els.push_back(nb);
								}
							}
							sd_full["elements"] = sc_els;
							dm_c->set_ui_screen_design(target_screen_id, sd_full, false);
						}
					}
				}
			}
			if (rad_radius_spin) lay["radial_radius"] = int(Math::round(rad_radius_spin->get_value()));
			if (rad_start_spin) lay["radial_start_angle_deg"] = float(rad_start_spin->get_value());
			bool full_360 = rad_full_chk ? rad_full_chk->is_pressed() : true;
			if (rad_sweep_spin) {
				rad_sweep_spin->set_editable(!full_360);
				lay["radial_sweep_angle_deg"] = full_360 ? 360.0f : float(rad_sweep_spin->get_value());
			}
			if (rad_cw_chk) lay["radial_clockwise"] = rad_cw_chk->is_pressed();
			if (rad_slot_size_spin) lay["radial_slot_size"] = int(Math::round(rad_slot_size_spin->get_value()));
			if (rad_slot_spacing_spin) lay["radial_slot_spacing"] = int(Math::round(rad_slot_spacing_spin->get_value()));
			if (rad_slot_shape_opt && rad_slot_shape_opt->get_selected() >= 0) lay["radial_slot_shape"] = String(rad_slot_shape_opt->get_item_metadata(rad_slot_shape_opt->get_selected()));
			if (rad_sel_scale_spin) lay["radial_selected_scale"] = float(rad_sel_scale_spin->get_value());
			if (rad_sel_border_col_btn) lay["radial_selected_border_color"] = "#" + rad_sel_border_col_btn->get_pick_color().to_html(true);
			if (rad_sel_border_w_spin) lay["radial_selected_border_width"] = int(Math::round(rad_sel_border_w_spin->get_value()));
			if (rad_sel_glow_chk) lay["radial_selected_glow"] = rad_sel_glow_chk->is_pressed();
			if (rad_sel_img_edit) lay["radial_selected_image"] = rad_sel_img_edit->get_text().strip_edges();
			if (rad_center_shape_opt && rad_center_shape_opt->get_selected() >= 0) lay["radial_center_shape"] = String(rad_center_shape_opt->get_item_metadata(rad_center_shape_opt->get_selected()));
			if (rad_center_img_edit) lay["radial_center_image"] = rad_center_img_edit->get_text().strip_edges();
			bool c_ic = rad_center_icon_chk ? rad_center_icon_chk->is_pressed() : true;
			bool c_tt = rad_center_title_chk ? rad_center_title_chk->is_pressed() : true;
			bool c_ds = rad_center_desc_chk ? rad_center_desc_chk->is_pressed() : true;
			lay["radial_center_show_icon"] = c_ic;
			lay["radial_center_show_title"] = c_tt;
			lay["radial_center_show_desc"] = c_ds;
			lay["radial_show_center_info"] = c_ic || c_tt || c_ds;
			if (rad_ring_img_edit) lay["radial_ring_image"] = rad_ring_img_edit->get_text().strip_edges();
			if (rad_ring_col_btn) lay["radial_ring_color"] = "#" + rad_ring_col_btn->get_pick_color().to_html(true);
			if (rad_ring_thick_spin) lay["radial_ring_thickness"] = int(Math::round(rad_ring_thick_spin->get_value()));
			if (rad_ind_enabled_chk) lay["radial_indicator_enabled"] = rad_ind_enabled_chk->is_pressed();
			if (rad_ind_col_btn) lay["radial_indicator_color"] = "#" + rad_ind_col_btn->get_pick_color().to_html(true);
			if (rad_ind_img_edit) lay["radial_indicator_image"] = rad_ind_img_edit->get_text().strip_edges();
		} else if (tp == "container") {
			if (cont_layout_opt && cont_layout_opt->get_selected() >= 0) lay["mode"] = String(cont_layout_opt->get_item_metadata(cont_layout_opt->get_selected()));
			if (cont_cols_spin) lay["columns"] = int(Math::round(cont_cols_spin->get_value()));
			if (cont_rows_spin) lay["rows"] = int(Math::round(cont_rows_spin->get_value()));
			if (cont_spx_spin) lay["spacing_x"] = int(Math::round(cont_spx_spin->get_value()));
			if (cont_spy_spin) lay["spacing_y"] = int(Math::round(cont_spy_spin->get_value()));
			if (cont_scroll_chk) lay["scroll_enabled"] = cont_scroll_chk->is_pressed();
		}
		if (anim_type_opt && anim_type_opt->get_selected() >= 0) lay["radial_open_anim"] = String(anim_type_opt->get_item_metadata(anim_type_opt->get_selected()));
		if (anim_dur_spin) lay["radial_open_duration_ms"] = int(Math::round(anim_dur_spin->get_value()));
		if (anim_curve_opt && anim_curve_opt->get_selected() >= 0) lay["radial_open_curve"] = String(anim_curve_opt->get_item_metadata(anim_curve_opt->get_selected()));
		if (snd_open_edit) lay["radial_sound_open"] = snd_open_edit->get_text().strip_edges();
		if (snd_move_edit) lay["radial_sound_move"] = snd_move_edit->get_text().strip_edges();
		if (snd_accept_edit) lay["radial_sound_accept"] = snd_accept_edit->get_text().strip_edges();
		el["layout"] = lay;
	}
	if (tp == "radial" || tp == "container") {
		if (el_data_source_opt && el_data_source_opt->get_selected() >= 0) el["data_source"] = String(el_data_source_opt->get_item_metadata(el_data_source_opt->get_selected()));
		if (el_filter_opt && el_filter_opt->get_selected() >= 0) el["data_filter_category"] = String(el_filter_opt->get_item_metadata(el_filter_opt->get_selected()));
		if (el_usable_chk) el["data_only_usable"] = el_usable_chk->is_pressed();
		if (el_sort_opt && el_sort_opt->get_selected() >= 0) el["data_sort_by"] = String(el_sort_opt->get_item_metadata(el_sort_opt->get_selected()));
		if (el_equip_slot_spin) el["data_equip_slot"] = int(Math::round(el_equip_slot_spin->get_value()));
		if (el_show_empty_chk) el["data_show_empty"] = el_show_empty_chk->is_pressed();
		if (el_empty_cnt_spin) el["data_empty_count"] = int(Math::round(el_empty_cnt_spin->get_value()));
		if (el_slot_action_opt && el_slot_action_opt->get_selected() >= 0) el["slot_action"] = String(el_slot_action_opt->get_item_metadata(el_slot_action_opt->get_selected()));
	}
	if (anim_type_opt && anim_type_opt->get_selected() >= 0) el["open_anim"] = String(anim_type_opt->get_item_metadata(anim_type_opt->get_selected()));
	if (anim_dur_spin) el["open_duration_ms"] = int(Math::round(anim_dur_spin->get_value()));
	if (snd_open_edit) el["sound_open"] = snd_open_edit->get_text().strip_edges();
	if (snd_move_edit) el["sound_move"] = snd_move_edit->get_text().strip_edges();
	if (snd_accept_edit) el["sound_accept"] = snd_accept_edit->get_text().strip_edges();
	if (show_bg_chk) el["show_background"] = show_bg_chk->is_pressed();
	if (style_opt && style_opt->get_selected() >= 0) el["window_style_id"] = String(style_opt->get_item_metadata(style_opt->get_selected()));

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary base_sh = s_extract_elem_shape_ui_state(dm, el);
	Dictionary next_sh = base_sh.duplicate(true);
	if (shape_opt && shape_opt->get_selected() >= 0) next_sh["shape_type"] = String(shape_opt->get_item_metadata(shape_opt->get_selected()));
	if (radius_slider) {
		int nr = int(Math::round(radius_slider->get_value()));
		next_sh["corner_radius"] = nr;
		if (radius_val_lbl) radius_val_lbl->set_text(itos(nr) + "px");
	}
	if (fill_color_btn) {
		String fc = "#" + fill_color_btn->get_pick_color().to_html(true);
		next_sh["fill_color"] = fc;
		if (tp == "bar") el["fill_color"] = fc;
	}
	if (grad_chk) next_sh["gradient_enabled"] = grad_chk->is_pressed();
	if (grad_top_btn) next_sh["gradient_top_color"] = "#" + grad_top_btn->get_pick_color().to_html(true);
	if (grad_bot_btn) next_sh["gradient_bottom_color"] = "#" + grad_bot_btn->get_pick_color().to_html(true);
	if (border_w_slider) next_sh["border_width"] = int(Math::round(border_w_slider->get_value()));
	if (border_col_btn) next_sh["border_color"] = "#" + border_col_btn->get_pick_color().to_html(true);
	if (shadow_chk) next_sh["shadow_enabled"] = shadow_chk->is_pressed();
	if (glow_chk) next_sh["glow_enabled"] = glow_chk->is_pressed();
	if (glow_col_btn) next_sh["glow_color"] = "#" + glow_col_btn->get_pick_color().to_html(true);
	bool pa = (filter_opt && filter_opt->get_selected() >= 0) ? bool(filter_opt->get_item_metadata(filter_opt->get_selected())) : true;
	el["pixel_art"] = pa;
	next_sh["pixel_art"] = pa;
	el["shape"] = ClockGearsDataManager::normalize_ui_shape(next_sh);

	if (opacity_slider) {
		int op = CLAMP(int(Math::round(opacity_slider->get_value())), 0, 100);
		el["opacity"] = double(op) / 100.0;
		if (opacity_val_lbl) opacity_val_lbl->set_text(itos(op) + "%");
	}
	String ikey = (tp == "currency" || tp == "item") ? "icon_path" : "image_path";
	if (image_edit) el[ikey] = image_edit->get_text().strip_edges();
	if (scale_opt && scale_opt->get_selected() >= 0) el["scale_mode"] = String(scale_opt->get_item_metadata(scale_opt->get_selected()));
	if (nine_slice_spin) el["nine_slice_margin"] = int(Math::round(nine_slice_spin->get_value()));
	static const char *k_sk[4] = { "image_normal", "image_hover", "image_pressed", "image_disabled" };
	for (int si = 0; si < 4; si++) {
		if (btn_state_edits[si]) el[k_sk[si]] = btn_state_edits[si]->get_text().strip_edges();
	}
	if (text_edit) {
		if (tp == "bar") el["label_text"] = text_edit->get_text();
		else el["text"] = text_edit->get_text();
	}
	if (desc_edit) el["description"] = desc_edit->get_text();
	if (font_opt && font_opt->get_selected() >= 0) el["font_path"] = String(font_opt->get_item_metadata(font_opt->get_selected()));
	if (font_size_spin) el["font_size"] = int(Math::round(font_size_spin->get_value()));
	if (font_color_btn) el["font_color"] = "#" + font_color_btn->get_pick_color().to_html(true);
	if (align_opt && align_opt->get_selected() >= 0) el["align"] = String(align_opt->get_item_metadata(align_opt->get_selected()));
	if (binding_opt && binding_opt->get_selected() >= 0) el["data_binding"] = String(binding_opt->get_item_metadata(binding_opt->get_selected()));
	if (var_edit) el["variable_name"] = var_edit->get_text().strip_edges();
	if (quick_action_opt && quick_action_opt->get_selected() >= 0) el["action"] = String(quick_action_opt->get_item_metadata(quick_action_opt->get_selected()));

	working_elem = el.duplicate(true);
	s_sync_manual_adjust_btn_state(adjust_manual_btn, String(el.get("scale_mode", "stretch")), s_elem_has_any_image(el));
	if (creation_mode) {
		owner_editor->set_ghost_preview_element(working_elem, true);
	} else {
		owner_editor->apply_floating_window_element_update(target_screen_id, target_elem_id, el);
	}
}

void ClockGearsUiElementFloatingWindow::_on_adjust_manual_pressed() {
	if (!owner_editor || working_elem.is_empty()) {
		return;
	}
	owner_editor->open_manual_image_adjust_for_element_dict(
			working_elem,
			callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_adjust_manual_confirmed));
}

void ClockGearsUiElementFloatingWindow::_on_adjust_manual_confirmed(const Dictionary &p_vals) {
	Array keys = p_vals.keys();
	for (int i = 0; i < keys.size(); i++) {
		working_elem[keys[i]] = p_vals[keys[i]];
	}
	working_elem["scale_mode"] = "manual";
	bool prev = updating;
	updating = true;
	if (scale_opt) {
		s_select_option_meta(scale_opt, "manual");
	}
	s_sync_manual_adjust_btn_state(adjust_manual_btn, "manual", s_elem_has_any_image(working_elem));
	updating = prev;
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_on_open_img_picker(int p_target) {
	picking_img_target = p_target;
	if (img_file_dialog) {
		img_file_dialog->popup_centered_ratio(0.6f);
	}
}

void ClockGearsUiElementFloatingWindow::_on_clear_img_picker(int p_target) {
	if (p_target < 0 && image_edit) image_edit->set_text("");
	else if (p_target >= 0 && p_target < 4 && btn_state_edits[p_target]) btn_state_edits[p_target]->set_text("");
	else if (p_target == 4 && rad_sel_img_edit) rad_sel_img_edit->set_text("");
	else if (p_target == 5 && rad_center_img_edit) rad_center_img_edit->set_text("");
	else if (p_target == 6 && rad_ring_img_edit) rad_ring_img_edit->set_text("");
	else if (p_target == 7 && rad_ind_img_edit) rad_ind_img_edit->set_text("");
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_on_img_file_selected(const String &p_path) {
	String imp = p_path;
	if (!p_path.begins_with("res://") && FileAccess::exists(p_path)) {
		String target_dir = "res://img/ui";
		DirAccess::make_dir_recursive_absolute(target_dir);
		String dst = target_dir.path_join(p_path.get_file());
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			da->copy(p_path, ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(dst) : dst);
			imp = dst;
		}
	}
	if (picking_img_target < 0 && image_edit) image_edit->set_text(imp);
	else if (picking_img_target >= 0 && picking_img_target < 4 && btn_state_edits[picking_img_target]) btn_state_edits[picking_img_target]->set_text(imp);
	else if (picking_img_target == 4 && rad_sel_img_edit) rad_sel_img_edit->set_text(imp);
	else if (picking_img_target == 5 && rad_center_img_edit) rad_center_img_edit->set_text(imp);
	else if (picking_img_target == 6 && rad_ring_img_edit) rad_ring_img_edit->set_text(imp);
	else if (picking_img_target == 7 && rad_ind_img_edit) rad_ind_img_edit->set_text(imp);
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_on_apply_all_slots_pressed() {
	if (!owner_editor || target_elem_id.is_empty()) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	String parent_id = target_elem_id;
	int dummy_idx = -1;
	bool is_vslot = s_parse_virtual_slot_id(target_elem_id, parent_id, dummy_idx);
	Dictionary cur_w = owner_editor->get_effective_element(target_elem_id);
	if (!is_vslot && String(cur_w.get("type", "")).to_lower() == "button") {
		String pid = String(cur_w.get("parent_id", "")).strip_edges();
		if (!pid.is_empty()) {
			Dictionary pel = owner_editor->get_effective_element(pid);
			if (String(pel.get("type", "")).strip_edges().to_lower() == "radial") {
				parent_id = pid;
			}
		}
	}
	Dictionary parent_el = owner_editor->get_effective_element(parent_id);
	if (parent_el.is_empty()) {
		return;
	}
	String ptype = String(parent_el.get("type", "")).strip_edges().to_lower();
	Dictionary src = working_elem;
	Dictionary src_sh = (src.has("shape") && src["shape"].get_type() == Variant::DICTIONARY) ? Dictionary(src["shape"]) : Dictionary();
	Dictionary src_lay = (src.has("layout") && src["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(src["layout"]) : Dictionary();
	String slot_shape = String(src_lay.get("radial_slot_shape", "")).strip_edges().to_lower();
	String fill_c = String(src_sh.get("fill_color", "#1c1f29f0"));
	String border_c = String(src_sh.get("border_color", "#f5bc54ff"));
	String font_c = String(src.get("font_color", "#f5f2eaff"));
	String img_n = String(src.get("image_normal", "")).strip_edges();
	String img_h = String(src.get("image_hover", "")).strip_edges();
	String img_p = String(src.get("image_pressed", "")).strip_edges();
	String img_d = String(src.get("image_disabled", "")).strip_edges();

	if (ptype == "radial") {
		Dictionary lay = (parent_el.has("layout") && parent_el["layout"].get_type() == Variant::DICTIONARY)
				? Dictionary(parent_el["layout"]).duplicate(true)
				: Dictionary();
		Array slots = (lay.has("radial_slots") && lay["radial_slots"].get_type() == Variant::ARRAY)
				? Array(lay["radial_slots"]).duplicate(true)
				: ClockGearsDataManager::make_default_radial_slots(6, target_screen_id);
		if (!slot_shape.is_empty()) {
			lay["radial_slot_shape"] = slot_shape;
		}
		for (int i = 0; i < slots.size(); i++) {
			if (slots[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary sd = Dictionary(slots[i]).duplicate(true);
			if (!slot_shape.is_empty()) sd["shape"] = slot_shape;
			sd["fill_color"] = fill_c;
			sd["border_color"] = border_c;
			sd["font_color"] = font_c;
			sd["image_normal"] = img_n;
			sd["image_hover"] = img_h;
			sd["image_pressed"] = img_p;
			sd["image_disabled"] = img_d;
			slots[i] = sd;
		}
		lay["radial_slots"] = slots;
		parent_el["layout"] = lay;
		// Aplicar también a todos los botones hijo reales de esta rueda radial en el diseño
		Dictionary sd_full = dm->get_ui_screen_design(target_screen_id);
		Array sc_els = sd_full.has("elements") && sd_full["elements"].get_type() == Variant::ARRAY
				? Array(sd_full["elements"]).duplicate(true)
				: Array();
		String pid_low = parent_id.to_lower();
		bool any_btn_updated = false;
		for (int bi = 0; bi < sc_els.size(); bi++) {
			if (sc_els[bi].get_type() != Variant::DICTIONARY) continue;
			Dictionary bd = Dictionary(sc_els[bi]).duplicate(true);
			if (String(bd.get("type", "")).to_lower() == "button" && String(bd.get("parent_id", "")).to_lower() == pid_low) {
				if (!slot_shape.is_empty()) {
					Dictionary blay = (bd.has("layout") && bd["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(bd["layout"]).duplicate(true) : Dictionary();
					blay["radial_slot_shape"] = slot_shape;
					bd["layout"] = blay;
				}
				Dictionary bsh = ClockGearsDataManager::normalize_ui_shape(bd.get("shape", Dictionary()));
				bsh["fill_color"] = fill_c;
				bsh["border_color"] = border_c;
				bd["shape"] = bsh;
				bd["font_color"] = font_c;
				bd["image_normal"] = img_n;
				bd["image_hover"] = img_h;
				bd["image_pressed"] = img_p;
				bd["image_disabled"] = img_d;
				sc_els[bi] = bd;
				any_btn_updated = true;
			}
		}
		if (any_btn_updated) {
			sd_full["elements"] = sc_els;
			dm->set_ui_screen_design(target_screen_id, sd_full, false);
		}
	} else if (ptype == "container") {
		int cnt = MAX(1, MIN(32, int(parent_el.get("data_empty_count", 8))));
		if (String(parent_el.get("data_source", "manual")) != "manual") {
			Array entries = dm->resolve_ui_container_entries(parent_el, -1, 0, true);
			cnt = MAX(cnt, CLAMP(int(entries.size()), 1, 24));
		}
		Dictionary ov_map = (parent_el.has("element_overrides") && parent_el["element_overrides"].get_type() == Variant::DICTIONARY)
				? Dictionary(parent_el["element_overrides"]).duplicate(true)
				: Dictionary();
		for (int i = 0; i < cnt; i++) {
			String ks = itos(i);
			Dictionary ov = (ov_map.has(ks) && ov_map[ks].get_type() == Variant::DICTIONARY) ? Dictionary(ov_map[ks]).duplicate(true) : Dictionary();
			ov["fill_color"] = fill_c;
			ov["border_color"] = border_c;
			ov["font_color"] = font_c;
			ov["image_normal"] = img_n;
			ov["image_hover"] = img_h;
			ov["image_pressed"] = img_p;
			ov["image_disabled"] = img_d;
			ov_map[ks] = ov;
		}
		parent_el["element_overrides"] = ov_map;
	} else {
		return;
	}
	owner_editor->apply_floating_window_element_update(target_screen_id, parent_id, parent_el);
	if (is_vslot) {
		sync_from_element(owner_editor->get_effective_element(target_elem_id));
	}
}

void ClockGearsUiElementFloatingWindow::_on_create_tpl_pressed() {
	if (!owner_editor || target_elem_id.is_empty()) {
		return;
	}
	String parent_id = target_elem_id;
	int dummy_idx = -1;
	s_parse_virtual_slot_id(target_elem_id, parent_id, dummy_idx);
	owner_editor->select_element(parent_id);
	owner_editor->_on_prop_create_item_tpl_pressed();
}

void ClockGearsUiElementFloatingWindow::_on_fw_add_action_pressed() {
	if (!fw_add_action_opt) {
		return;
	}
	int sel = fw_add_action_opt->get_selected();
	if (sel < 0) sel = 0;
	String aid = (sel < fw_add_action_opt->get_item_count()) ? String(fw_add_action_opt->get_item_metadata(sel)) : String();
	if (aid.is_empty()) {
		return;
	}
	Array acts = (working_elem.has("actions") && working_elem["actions"].get_type() == Variant::ARRAY)
			? Array(working_elem["actions"]).duplicate(true)
			: Array();
	Dictionary step;
	step["type"] = aid;
	step["label"] = fw_add_action_opt->get_item_text(sel);
	acts.push_back(step);
	working_elem["actions"] = acts;
	_refresh_fw_actions_list();
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_on_fw_remove_action_pressed(int p_idx) {
	Array acts = (working_elem.has("actions") && working_elem["actions"].get_type() == Variant::ARRAY)
			? Array(working_elem["actions"]).duplicate(true)
			: Array();
	if (p_idx < 0 || p_idx >= acts.size()) {
		return;
	}
	acts.remove_at(p_idx);
	working_elem["actions"] = acts;
	_refresh_fw_actions_list();
	_emit_commit();
}

void ClockGearsUiElementFloatingWindow::_refresh_fw_actions_list() {
	Array acts = (working_elem.has("actions") && working_elem["actions"].get_type() == Variant::ARRAY)
			? Array(working_elem["actions"])
			: Array();
	if (fw_actions_list_vbox) {
		while (fw_actions_list_vbox->get_child_count() > 0) {
			Node *ch = fw_actions_list_vbox->get_child(fw_actions_list_vbox->get_child_count() - 1);
			fw_actions_list_vbox->remove_child(ch);
			ch->queue_free();
		}
		for (int i = 0; i < acts.size(); i++) {
			Dictionary st = (acts[i].get_type() == Variant::DICTIONARY) ? Dictionary(acts[i]) : Dictionary();
			String st_type = String(st.get("type", st.get("action", "none")));
			String st_lbl = String(st.get("label", st_type));
			HBoxContainer *hb = memnew(HBoxContainer);
			hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			hb->add_theme_constant_override("separation", int(6 * EDSCALE));
			Label *lbl = memnew(Label(itos(i + 1) + ". " + st_lbl + " (" + st_type + ")"));
			lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			lbl->set_clip_text(true);
			lbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			hb->add_child(lbl);
			Button *rm = memnew(Button("X"));
			rm->set_custom_minimum_size(Size2(30 * EDSCALE, 28 * EDSCALE));
			rm->connect("pressed", callable_mp(this, &ClockGearsUiElementFloatingWindow::_on_fw_remove_action_pressed).bind(i));
			hb->add_child(rm);
			fw_actions_list_vbox->add_child(hb);
		}
	}
	if (actions_summary_lbl) {
		actions_summary_lbl->set_text(acts.is_empty() ? String("Comandos: 0") : (String("Comandos configurados: ") + itos(acts.size())));
	}
}

void ClockGearsUiElementFloatingWindow::_on_open_font_picker() {
	if (font_file_dialog) {
		font_file_dialog->popup_centered_ratio(0.6f);
	}
}

void ClockGearsUiElementFloatingWindow::_on_font_file_selected(const String &p_path) {
	String dst = p_path;
	if (!p_path.begins_with("res://") && FileAccess::exists(p_path)) {
		String target_dir = "res://fonts/ui";
		DirAccess::make_dir_recursive_absolute(target_dir);
		dst = target_dir.path_join(p_path.get_file());
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			da->copy(p_path, ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(dst) : dst);
		}
	}
	if (!dst.is_empty()) {
		working_elem["font_path"] = dst;
		sync_from_element(working_elem);
		_emit_commit();
	}
}

Dictionary ClockGearsUiElementFloatingWindow::get_ui_handles() const {
	Dictionary d;
	d["window"] = this;
	d["tabs"] = tabs;
	d["name_edit"] = name_edit;
	d["visible_chk"] = visible_chk;
	d["locked_chk"] = locked_chk;
	d["clip_children_chk"] = clip_children_chk;
	d["ox_spin"] = ox_spin;
	d["oy_spin"] = oy_spin;
	d["w_spin"] = w_spin;
	d["h_spin"] = h_spin;
	d["layer_spin"] = layer_spin;
	d["slot_vis_mode_opt"] = slot_vis_mode_opt;
	d["slot_vis_sw_edit"] = slot_vis_sw_edit;
	d["slot_en_mode_opt"] = slot_en_mode_opt;
	d["slot_en_sw_edit"] = slot_en_sw_edit;
	d["dist_radial_box"] = dist_radial_box;
	d["dist_grid_box"] = dist_grid_box;
	d["rad_count_src_opt"] = rad_count_src_opt;
	d["rad_count_spin"] = rad_count_spin;
	d["rad_radius_spin"] = rad_radius_spin;
	d["rad_start_spin"] = rad_start_spin;
	d["rad_full_chk"] = rad_full_chk;
	d["rad_sweep_spin"] = rad_sweep_spin;
	d["rad_cw_chk"] = rad_cw_chk;
	d["rad_slot_size_spin"] = rad_slot_size_spin;
	d["rad_slot_spacing_spin"] = rad_slot_spacing_spin;
	d["rad_slot_shape_opt"] = rad_slot_shape_opt;
	d["rad_sel_scale_spin"] = rad_sel_scale_spin;
	d["rad_sel_border_col_btn"] = rad_sel_border_col_btn;
	d["rad_sel_border_w_spin"] = rad_sel_border_w_spin;
	d["rad_sel_glow_chk"] = rad_sel_glow_chk;
	d["rad_sel_img_edit"] = rad_sel_img_edit;
	d["rad_center_shape_opt"] = rad_center_shape_opt;
	d["rad_center_img_edit"] = rad_center_img_edit;
	d["rad_center_icon_chk"] = rad_center_icon_chk;
	d["rad_center_title_chk"] = rad_center_title_chk;
	d["rad_center_desc_chk"] = rad_center_desc_chk;
	d["rad_ring_img_edit"] = rad_ring_img_edit;
	d["rad_ring_col_btn"] = rad_ring_col_btn;
	d["rad_ring_thick_spin"] = rad_ring_thick_spin;
	d["rad_ind_enabled_chk"] = rad_ind_enabled_chk;
	d["rad_ind_col_btn"] = rad_ind_col_btn;
	d["rad_ind_img_edit"] = rad_ind_img_edit;
	d["cont_layout_opt"] = cont_layout_opt;
	d["cont_cols_spin"] = cont_cols_spin;
	d["cont_rows_spin"] = cont_rows_spin;
	d["cont_spx_spin"] = cont_spx_spin;
	d["cont_spy_spin"] = cont_spy_spin;
	d["cont_scroll_chk"] = cont_scroll_chk;
	d["el_data_source_opt"] = el_data_source_opt;
	d["el_filter_opt"] = el_filter_opt;
	d["el_usable_chk"] = el_usable_chk;
	d["el_sort_opt"] = el_sort_opt;
	d["el_equip_slot_spin"] = el_equip_slot_spin;
	d["el_show_empty_chk"] = el_show_empty_chk;
	d["el_empty_cnt_spin"] = el_empty_cnt_spin;
	d["el_slot_action_opt"] = el_slot_action_opt;
	d["el_create_tpl_btn"] = el_create_tpl_btn;
	d["el_apply_all_btn"] = el_apply_all_btn;
	d["desc_edit"] = desc_edit;
	d["anim_type_opt"] = anim_type_opt;
	d["anim_dur_spin"] = anim_dur_spin;
	d["anim_curve_opt"] = anim_curve_opt;
	d["snd_open_edit"] = snd_open_edit;
	d["snd_move_edit"] = snd_move_edit;
	d["snd_accept_edit"] = snd_accept_edit;
	d["fw_add_action_opt"] = fw_add_action_opt;
	d["fw_add_action_btn"] = fw_add_action_btn;
	d["fw_actions_list_vbox"] = fw_actions_list_vbox;
	d["show_bg_chk"] = show_bg_chk;
	d["style_opt"] = style_opt;
	d["shape_opt"] = shape_opt;
	d["radius_slider"] = radius_slider;
	d["opacity_slider"] = opacity_slider;
	d["fill_color_btn"] = fill_color_btn;
	d["grad_chk"] = grad_chk;
	d["grad_top_btn"] = grad_top_btn;
	d["grad_bot_btn"] = grad_bot_btn;
	d["border_w_slider"] = border_w_slider;
	d["border_col_btn"] = border_col_btn;
	d["shadow_chk"] = shadow_chk;
	d["glow_chk"] = glow_chk;
	d["glow_col_btn"] = glow_col_btn;
	d["image_edit"] = image_edit;
	d["scale_opt"] = scale_opt;
	d["adjust_manual_btn"] = adjust_manual_btn;
	d["nine_slice_spin"] = nine_slice_spin;
	d["filter_opt"] = filter_opt;
	d["btn_normal_edit"] = btn_state_edits[0];
	d["btn_hover_edit"] = btn_state_edits[1];
	d["btn_pressed_edit"] = btn_state_edits[2];
	d["btn_disabled_edit"] = btn_state_edits[3];
	d["text_edit"] = text_edit;
	d["font_opt"] = font_opt;
	d["font_size_spin"] = font_size_spin;
	d["font_color_btn"] = font_color_btn;
	d["align_opt"] = align_opt;
	d["binding_opt"] = binding_opt;
	d["var_edit"] = var_edit;
	d["quick_action_opt"] = quick_action_opt;
	d["actions_host"] = actions_host;
	d["quick_shape_opt"] = quick_shape_opt;
	d["create_footer"] = create_footer;
	d["create_confirm_btn"] = create_confirm_btn;
	d["create_cancel_btn"] = create_cancel_btn;
	return d;
}
