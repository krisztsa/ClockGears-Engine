#include "clockgears_ui_design_layer.h"

#include "../core/clockgears_data_manager.h"
#include "clockgears_audio_manager.h"
#include "clockgears_title_screen.h"
#include "clockgears_window_style.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "scene/gui/box_container.h"
#include "scene/gui/nine_patch_rect.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_texture.h"

namespace {
static HashMap<String, Ref<Font>> s_cg_ui_font_cache;
static Color _cg_ui_parse_color(const Variant &p_val, const Color &p_default);

class ClockGearsUiBackdropDraw : public Control {
	GDCLASS(ClockGearsUiBackdropDraw, Control);

	Dictionary bg_cfg;
	Ref<Texture2D> bg_tex;
	Vector2 scroll_offset;

protected:
	void _notification(int p_what) {
		if (p_what != NOTIFICATION_DRAW) {
			return;
		}
		Size2 sz = get_size();
		if (sz.x <= 0.0f || sz.y <= 0.0f) {
			return;
		}
		String bt = String(bg_cfg.get("type", "none")).strip_edges().to_lower();
		if (bt == "color") {
			draw_rect(Rect2(Vector2(), sz), _cg_ui_parse_color(bg_cfg.get("color", "#0d111ae6"), Color(0.05f, 0.07f, 0.1f, 0.9f)));
		} else if (bt == "gradient") {
			Color c1 = _cg_ui_parse_color(bg_cfg.get("gradient_top", "#1b2234ff"), Color(0.11f, 0.13f, 0.2f, 1.0f));
			Color c2 = _cg_ui_parse_color(bg_cfg.get("gradient_bottom", "#0b0e16ff"), Color(0.04f, 0.05f, 0.09f, 1.0f));
			bool horiz = (String(bg_cfg.get("gradient_angle", "vertical")).strip_edges().to_lower() == "horizontal");
			PackedVector2Array pts = { Vector2(0, 0), Vector2(sz.x, 0), Vector2(sz.x, sz.y), Vector2(0, sz.y) };
			PackedColorArray cols = horiz
					? PackedColorArray({ c1, c2, c2, c1 })
					: PackedColorArray({ c1, c1, c2, c2 });
			draw_polygon(pts, cols);
		} else if (bt == "image" && bg_tex.is_valid()) {
			String sm = String(bg_cfg.get("scale_mode", "stretch")).strip_edges().to_lower();
			Vector2 ts = bg_tex->get_size();
			if (ts.x > 0.0f && ts.y > 0.0f) {
				if (sm == "tile" || Math::abs(scroll_offset.x) > 0.01f || Math::abs(scroll_offset.y) > 0.01f ||
						int(bg_cfg.get("scroll_speed_x", 0)) != 0 || int(bg_cfg.get("scroll_speed_y", 0)) != 0) {
					float ox = Math::fposmod(scroll_offset.x, ts.x);
					float oy = Math::fposmod(scroll_offset.y, ts.y);
					for (float y = oy - ts.y; y < sz.y; y += ts.y) {
						for (float x = ox - ts.x; x < sz.x; x += ts.x) {
							draw_texture_rect(bg_tex, Rect2(Vector2(x, y), ts), false);
						}
					}
				} else if (sm == "fit") {
					float s = MIN(sz.x / ts.x, sz.y / ts.y);
					Vector2 dsz = ts * s;
					draw_texture_rect(bg_tex, Rect2((sz - dsz) * 0.5f, dsz), false);
				} else if (sm == "fill") {
					float s = MAX(sz.x / ts.x, sz.y / ts.y);
					Vector2 dsz = ts * s;
					draw_texture_rect(bg_tex, Rect2((sz - dsz) * 0.5f, dsz), false);
				} else if (sm == "integer") {
					int mult = MAX(1, int(Math::floor(MIN(sz.x / ts.x, sz.y / ts.y))));
					Vector2 dsz = ts * float(mult);
					draw_texture_rect(bg_tex, Rect2(((sz - dsz) * 0.5f).floor(), dsz), false);
				} else if (sm == "nine_slice") {
					float m = CLAMP(float(bg_cfg.get("nine_slice_margin", 16)), 0.0f, MIN(ts.x, ts.y) * 0.45f);
					Ref<StyleBoxTexture> sbt;
					sbt.instantiate();
					sbt->set_texture(bg_tex);
					sbt->set_texture_margin_all(m);
					draw_style_box(sbt, Rect2(Vector2(), sz));
				} else {
					draw_texture_rect(bg_tex, Rect2(Vector2(), sz), false);
				}
			}
		}
	}

public:
	void configure(const Dictionary &p_bg, const Ref<Texture2D> &p_tex) {
		bg_cfg = p_bg;
		bg_tex = p_tex;
	bool pa = bg_cfg.has("pixel_art") ? bool(bg_cfg["pixel_art"]) : true;
	String sm = String(bg_cfg.get("scale_mode", "stretch")).strip_edges().to_lower();
	set_texture_filter((pa || sm == "integer") ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
	queue_redraw();
	}
	void set_scroll_offset(const Vector2 &p_off) {
		scroll_offset = p_off;
		queue_redraw();
	}
};

static Color _cg_ui_parse_color(const Variant &p_val, const Color &p_default) {
	if (p_val.get_type() == Variant::COLOR) {
		return Color(p_val);
	}
	String s = String(p_val).strip_edges();
	if (!s.is_empty() && Color::html_is_valid(s)) {
		return Color::html(s);
	}
	return p_default;
}

static int _cg_ui_get_leader_actor_id(ClockGearsDataManager *p_dm, int p_actor_override = 0) {
	if (!p_dm) {
		return 0;
	}
	if (p_actor_override > 0) {
		return p_actor_override;
	}
	Array party = p_dm->get_party_actor_ids();
	if (!party.is_empty()) {
		return int(party[0]);
	}
	Array actors = p_dm->get_category_array(CAT_ACTORS);
	if (!actors.is_empty() && actors[0].get_type() == Variant::DICTIONARY) {
		return int(Dictionary(actors[0]).get("id", 1));
	}
	return 0;
}

static void _cg_draw_thick_arc(Control *p_ctrl, const Vector2 &p_center, float p_radius, float p_start_rad, float p_end_rad, float p_width, const Color &p_color, bool p_antialiased) {
	if (!p_ctrl || p_radius <= 1.0f || p_color.a <= 0.001f) {
		return;
	}
	p_ctrl->draw_arc(p_center, p_radius, p_start_rad, p_end_rad, 48, p_color, p_width, p_antialiased);
}
} // namespace

// ============================================================================
// ClockGearsShapedClipControl & ClockGearsShapedBorderOverlay (3F)
// ============================================================================

void ClockGearsShapedClipControl::configure(const Dictionary &p_shape, float p_scale, bool p_draw_fill) {
	shape_type = String(p_shape.get("shape_type", "rounded")).strip_edges().to_lower();
	if (shape_type.is_empty()) {
		shape_type = "rounded";
	}
	corner_radius = float(MAX(0, int(p_shape.get("corner_radius", 8)))) * p_scale;
	clip_active = p_shape.has("clip_content") ? bool(p_shape["clip_content"]) : (shape_type == "circle" || shape_type == "ellipse" || shape_type == "pill" || shape_type == "rounded");
	fill_color = _cg_ui_parse_color(p_shape.get("fill_color", "#14171ff0"), Color(0.08f, 0.09f, 0.12f, 0.94f));
	if (!p_draw_fill) {
		// Necesitamos alpha=1 en la máscara cuando CLIP_CHILDREN_AND_DRAW recorta hijos sin teñir el contenido si no pidió fondo propio
		fill_color = Color(0.0f, 0.0f, 0.0f, 0.0f);
	}
	gradient_enabled = p_draw_fill && bool(p_shape.get("gradient_enabled", false));
	gradient_top = _cg_ui_parse_color(p_shape.get("gradient_top_color", "#28221aff"), Color(0.16f, 0.13f, 0.10f, 1.0f));
	gradient_bottom = _cg_ui_parse_color(p_shape.get("gradient_bottom_color", "#14110eff"), Color(0.08f, 0.07f, 0.05f, 1.0f));
	gradient_angle = String(p_shape.get("gradient_angle", "vertical")).strip_edges().to_lower();
	pixel_art = p_shape.has("pixel_art") ? bool(p_shape["pixel_art"]) : true;
	String fimg = String(p_shape.get("fill_image_path", "")).strip_edges();
	fill_image_enabled = p_draw_fill && (String(p_shape.get("fill_type", "color")).strip_edges().to_lower() == "image") && !fimg.is_empty();
	fill_texture = fill_image_enabled ? ClockGearsWindowStyleManager::load_texture_safe(fimg) : Ref<Texture2D>();
	set_texture_filter(pixel_art ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
	set_clip_children_mode(clip_active ? CLIP_CHILDREN_AND_DRAW : CLIP_CHILDREN_DISABLED);
	queue_redraw();
}

bool ClockGearsShapedClipControl::contains_local_point(const Vector2 &p_local) const {
	Rect2 r(Vector2(), get_size());
	if (!r.has_point(p_local)) {
		return false;
	}
	Vector<Vector2> poly = ClockGearsWindowStyleBox::build_shape_polygon(r, shape_type, corner_radius, 24);
	if (poly.size() < 3) {
		return r.has_point(p_local);
	}
	return Geometry2D::is_point_in_polygon(p_local, poly);
}

void ClockGearsShapedClipControl::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Rect2 r(Vector2(), get_size());
		if (r.size.x <= 1.0f || r.size.y <= 1.0f) {
			return;
		}
		Vector<Vector2> poly = ClockGearsWindowStyleBox::build_shape_polygon(r, shape_type, corner_radius, 24);
		if (poly.size() < 3) {
			return;
		}
		if (gradient_enabled) {
			Vector<Color> vcols;
			vcols.resize(poly.size());
			bool horiz = (gradient_angle == "horizontal");
			for (int i = 0; i < poly.size(); i++) {
				float t = horiz
						? CLAMP(poly[i].x / MAX(1.0f, r.size.x), 0.0f, 1.0f)
						: CLAMP(poly[i].y / MAX(1.0f, r.size.y), 0.0f, 1.0f);
				vcols.write[i] = gradient_top.lerp(gradient_bottom, t);
			}
			draw_polygon(poly, vcols);
		} else {
			// Nota: para CLIP_CHILDREN_AND_DRAW la máscara usa el alpha dibujado aquí; usar al menos fill_color o sólido blanco opaco si no lleva fondo visible
			Color mask_col = (fill_color.a > 0.005f) ? fill_color : Color(0.08f, 0.09f, 0.12f, 1.0f);
			draw_colored_polygon(poly, mask_col);
		}
		if (fill_image_enabled && fill_texture.is_valid()) {
			draw_texture_rect(fill_texture, r, false);
		}
	}
}

void ClockGearsShapedBorderOverlay::configure(const Dictionary &p_shape, float p_scale) {
	shape_type = String(p_shape.get("shape_type", "rounded")).strip_edges().to_lower();
	if (shape_type.is_empty()) shape_type = "rounded";
	corner_radius = float(MAX(0, int(p_shape.get("corner_radius", 8)))) * p_scale;
	border_width = MAX(0, int(Math::round(float(int(p_shape.get("border_width", 2))) * p_scale)));
	border_color = _cg_ui_parse_color(p_shape.get("border_color", "#e0a040ff"), Color(0.88f, 0.63f, 0.25f, 1.0f));
	shadow_enabled = bool(p_shape.get("shadow_enabled", false));
	shadow_color = _cg_ui_parse_color(p_shape.get("shadow_color", "#00000073"), Color(0, 0, 0, 0.45f));
	shadow_size = float(CLAMP(int(p_shape.get("shadow_size", 4)), 0, 32)) * p_scale;
	shadow_offset = Vector2(float(int(p_shape.get("shadow_offset_x", 0))), float(int(p_shape.get("shadow_offset_y", 2)))) * p_scale;
	glow_enabled = bool(p_shape.get("glow_enabled", false));
	glow_color = _cg_ui_parse_color(p_shape.get("glow_color", "#f5bc5466"), Color(0.96f, 0.74f, 0.33f, 0.40f));
	glow_size = float(CLAMP(int(p_shape.get("glow_size", 4)), 0, 24)) * p_scale;
	pixel_art = p_shape.has("pixel_art") ? bool(p_shape["pixel_art"]) : true;
	queue_redraw();
}

void ClockGearsShapedBorderOverlay::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Rect2 r(Vector2(), get_size());
		if (r.size.x <= 1.0f || r.size.y <= 1.0f) {
			return;
		}
		if (glow_enabled && glow_size > 0.5f && glow_color.a > 0.001f) {
			Rect2 gr = r.grow(glow_size * 0.5f);
			Vector<Vector2> gpts = ClockGearsWindowStyleBox::build_shape_polygon(gr, shape_type, corner_radius + glow_size * 0.5f, 24);
			if (gpts.size() >= 3) {
				Vector<Vector2> closed = gpts;
				closed.push_back(gpts[0]);
				draw_polyline(closed, glow_color, glow_size, !pixel_art);
			}
		}
		if (border_width > 0 && border_color.a > 0.001f) {
			Vector<Vector2> bpts = ClockGearsWindowStyleBox::build_shape_polygon(r, shape_type, corner_radius, 24);
			if (bpts.size() >= 3) {
				Vector<Vector2> closed = bpts;
				closed.push_back(bpts[0]);
				draw_polyline(closed, border_color, float(border_width), !pixel_art);
			}
		}
	}
}

// ============================================================================
// ClockGearsRadialCanvas — Anillo Radial Prioritario (3E)
// ============================================================================

ClockGearsRadialCanvas::ClockGearsRadialCanvas() {
	set_name("RadialMenuCanvas");
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_focus_mode(FOCUS_ALL);
}

void ClockGearsRadialCanvas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_slot_count"), &ClockGearsRadialCanvas::get_slot_count);
	ClassDB::bind_method(D_METHOD("get_selected_index"), &ClockGearsRadialCanvas::get_selected_index);
	ClassDB::bind_method(D_METHOD("select_slot", "index"), &ClockGearsRadialCanvas::select_slot);
	ClassDB::bind_method(D_METHOD("step_selection", "delta"), &ClockGearsRadialCanvas::step_selection);
	ClassDB::bind_method(D_METHOD("select_by_angle_vector", "dir_vec"), &ClockGearsRadialCanvas::select_by_angle_vector);
	ClassDB::bind_method(D_METHOD("select_by_local_point", "local_pos"), &ClockGearsRadialCanvas::select_by_local_point);
	ClassDB::bind_method(D_METHOD("activate_selected_slot"), &ClockGearsRadialCanvas::activate_selected_slot);
	ClassDB::bind_method(D_METHOD("get_slot_local_pos", "index"), &ClockGearsRadialCanvas::get_slot_local_pos);
	ClassDB::bind_method(D_METHOD("get_slot_visual_scale", "index"), &ClockGearsRadialCanvas::get_slot_visual_scale);
	ClassDB::bind_method(D_METHOD("get_center_displayed_info"), &ClockGearsRadialCanvas::get_center_displayed_info);
	ClassDB::bind_method(D_METHOD("get_selected_slot_info"), &ClockGearsRadialCanvas::get_selected_slot_info);

	ADD_SIGNAL(MethodInfo("slot_activated", PropertyInfo(Variant::INT, "slot_index")));
	ADD_SIGNAL(MethodInfo("selection_changed", PropertyInfo(Variant::INT, "slot_index")));
}

void ClockGearsRadialCanvas::configure(const Dictionary &p_layout, const Array &p_fallback_elements, float p_scale, bool p_interactive) {
	layout_cfg = p_layout.duplicate(true);
	ui_scale = MAX(0.25f, p_scale);
	interactive = p_interactive;
	all_slots.clear();

	Array raw_slots = layout_cfg.get("radial_slots", Array());
	if (!raw_slots.is_empty()) {
		for (int i = 0; i < raw_slots.size(); i++) {
			if (raw_slots[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary d = raw_slots[i];
			RadialSlot rs;
			rs.id = String(d.get("id", vformat("slot_%d", i + 1)));
			rs.title = String(d.get("title", d.get("name", rs.id)));
			rs.description = String(d.get("description", ""));
			rs.icon_path = String(d.get("icon_path", d.get("image_path", ""))).strip_edges();
			rs.pixel_art = d.has("pixel_art") ? bool(d["pixel_art"]) : true;
			rs.action = String(d.get("action", "")).strip_edges().to_lower();
			rs.actions = d.has("actions") && d["actions"].get_type() == Variant::ARRAY ? Array(d["actions"]).duplicate(true) : Array();
			rs.enabled_mode = String(d.get("enabled_mode", "always")).strip_edges().to_lower();
			rs.enabled_switch = String(d.get("enabled_switch", "")).strip_edges();
			rs.visibility_mode = String(d.get("visibility_mode", "always")).strip_edges().to_lower();
			rs.visibility_switch = String(d.get("visibility_switch", "")).strip_edges();
			if (!rs.icon_path.is_empty()) {
				rs.cached_icon = ClockGearsWindowStyleManager::load_texture_safe(rs.icon_path);
			}
			all_slots.push_back(rs);
		}
	} else {
		// Tomar botones o elementos interactivos definidos en el menú
		for (int i = 0; i < p_fallback_elements.size(); i++) {
			if (p_fallback_elements[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary d = p_fallback_elements[i];
			String et = String(d.get("type", "")).strip_edges().to_lower();
			if (et != "button") continue;
			RadialSlot rs;
			rs.id = String(d.get("id", vformat("slot_%d", i + 1)));
			rs.title = String(d.get("text", d.get("name", rs.id)));
			rs.description = String(d.get("description", ""));
			rs.icon_path = String(d.get("icon_path", d.get("image_path", ""))).strip_edges();
			rs.pixel_art = d.has("pixel_art") ? bool(d["pixel_art"]) : true;
			rs.action = String(d.get("action", "")).strip_edges().to_lower();
			rs.actions = d.has("actions") && d["actions"].get_type() == Variant::ARRAY ? Array(d["actions"]).duplicate(true) : Array();
			rs.enabled_mode = String(d.get("enabled_mode", "always")).strip_edges().to_lower();
			rs.enabled_switch = String(d.get("enabled_switch", "")).strip_edges();
			rs.visibility_mode = String(d.get("visibility_mode", "always")).strip_edges().to_lower();
			rs.visibility_switch = String(d.get("visibility_switch", "")).strip_edges();
			if (!rs.icon_path.is_empty()) {
				rs.cached_icon = ClockGearsWindowStyleManager::load_texture_safe(rs.icon_path);
			}
			all_slots.push_back(rs);
		}
	}

	refresh_slot_states(ClockGearsDataManager::get_singleton());
	selected_pos = active_slot_indices.is_empty() ? 0 : CLAMP(selected_pos, 0, active_slot_indices.size() - 1);
	slot_visual_scales.resize(active_slot_indices.size());
	float sel_sc = CLAMP(float(layout_cfg.get("radial_selected_scale", 1.28)), 1.0f, 1.8f);
	for (int i = 0; i < active_slot_indices.size(); i++) {
		slot_visual_scales.write[i] = (i == selected_pos) ? sel_sc : 1.0f;
	}
	set_mouse_filter(interactive ? MOUSE_FILTER_STOP : MOUSE_FILTER_IGNORE);
	set_focus_mode(interactive ? FOCUS_ALL : FOCUS_NONE);
	queue_redraw();
}

void ClockGearsRadialCanvas::_rebuild_visible_indices() {
	active_slot_indices.clear();
	for (int i = 0; i < all_slots.size(); i++) {
		if (all_slots[i].visible) {
			active_slot_indices.push_back(i);
		}
	}
	if (active_slot_indices.is_empty()) {
		selected_pos = 0;
	} else {
		selected_pos = CLAMP(selected_pos, 0, active_slot_indices.size() - 1);
	}
}

void ClockGearsRadialCanvas::refresh_slot_states(const ClockGearsDataManager *p_dm) {
	for (int i = 0; i < all_slots.size(); i++) {
		RadialSlot &rs = all_slots.write[i];
		bool vis = true;
		if (p_dm && rs.visibility_mode == "switch" && !rs.visibility_switch.is_empty()) {
			vis = p_dm->get_switch(rs.visibility_switch);
		}
		bool dis = (rs.enabled_mode == "disabled");
		if (!dis && p_dm && rs.enabled_mode == "switch") {
			dis = rs.enabled_switch.is_empty() ? true : !p_dm->get_switch(rs.enabled_switch);
		}
		rs.visible = vis;
		rs.disabled = dis;
	}
	_rebuild_visible_indices();
	if (slot_visual_scales.size() != active_slot_indices.size()) {
		slot_visual_scales.resize(active_slot_indices.size());
		float sel_sc = CLAMP(float(layout_cfg.get("radial_selected_scale", 1.28)), 1.0f, 1.8f);
		for (int i = 0; i < active_slot_indices.size(); i++) {
			slot_visual_scales.write[i] = (i == selected_pos) ? sel_sc : 1.0f;
		}
	}
	queue_redraw();
}

Vector2 ClockGearsRadialCanvas::_compute_center_local() const {
	float cx = float(layout_cfg.get("radial_center_x", 480)) * ui_scale;
	float cy = float(layout_cfg.get("radial_center_y", 270)) * ui_scale;
	Size2 sz = get_size();
	if (sz.x > 16.0f && sz.y > 16.0f) {
		// Mantener proporción respecto al lienzo 960x540 asignado al canvas
		cx = (float(layout_cfg.get("radial_center_x", 480)) / ClockGearsUiDesignLayer::LOGICAL_WIDTH) * sz.x;
		cy = (float(layout_cfg.get("radial_center_y", 270)) / ClockGearsUiDesignLayer::LOGICAL_HEIGHT) * sz.y;
	}
	return Vector2(cx, cy);
}

float ClockGearsRadialCanvas::_compute_radius_px() const {
	float r = float(CLAMP(int(layout_cfg.get("radial_radius", 145)), 48, 250)) * ui_scale;
	Size2 sz = get_size();
	if (sz.x > 16.0f && sz.y > 16.0f) {
		float max_r = MIN(sz.x, sz.y) * 0.38f;
		r = MIN(r, max_r);
	}
	return MAX(32.0f, r);
}

float ClockGearsRadialCanvas::_compute_slot_angle(int p_visible_idx, int p_count) const {
	float start_rad = Math::deg_to_rad(float(layout_cfg.get("radial_start_angle_deg", -90.0)));
	float sweep_deg = CLAMP(float(layout_cfg.get("radial_sweep_angle_deg", 360.0)), 45.0f, 360.0f);
	float sweep_rad = Math::deg_to_rad(sweep_deg);
	bool cw = layout_cfg.has("radial_clockwise") ? bool(layout_cfg["radial_clockwise"]) : true;
	float dir = cw ? 1.0f : -1.0f;
	if (p_count <= 1) {
		return start_rad;
	}
	float step = (sweep_deg >= 359.5f) ? (sweep_rad / float(p_count)) : (sweep_rad / float(p_count - 1));
	return start_rad + dir * float(p_visible_idx) * step;
}

int ClockGearsRadialCanvas::_pick_slot_by_angle(float p_target_angle) const {
	int cnt = active_slot_indices.size();
	if (cnt <= 0) {
		return -1;
	}
	int best_idx = 0;
	float best_diff = 1e9f;
	for (int i = 0; i < cnt; i++) {
		float a = _compute_slot_angle(i, cnt);
		float diff = Math::abs(Math::angle_difference(p_target_angle, a));
		if (diff < best_diff) {
			best_diff = diff;
			best_idx = i;
		}
	}
	return best_idx;
}

bool ClockGearsRadialCanvas::select_slot(int p_index) {
	int cnt = active_slot_indices.size();
	if (cnt <= 0) {
		return false;
	}
	int clamped = CLAMP(p_index, 0, cnt - 1);
	if (clamped == selected_pos) {
		return false;
	}
	selected_pos = clamped;
	anim_active = true;
	set_process(true);
	queue_redraw();
	emit_signal("selection_changed", selected_pos);
	return true;
}

bool ClockGearsRadialCanvas::step_selection(int p_delta) {
	int cnt = active_slot_indices.size();
	if (cnt <= 0 || p_delta == 0) {
		return false;
	}
	int next = ((selected_pos + p_delta) % cnt + cnt) % cnt;
	return select_slot(next);
}

bool ClockGearsRadialCanvas::select_by_angle_vector(const Vector2 &p_dir_vec) {
	if (p_dir_vec.length_squared() < 0.04f || active_slot_indices.is_empty()) {
		return false;
	}
	float target_a = Math::atan2(p_dir_vec.y, p_dir_vec.x);
	int idx = _pick_slot_by_angle(target_a);
	if (idx >= 0) {
		return select_slot(idx);
	}
	return false;
}

bool ClockGearsRadialCanvas::select_by_local_point(const Vector2 &p_local_pos) {
	if (active_slot_indices.is_empty()) {
		return false;
	}
	Vector2 c = _compute_center_local();
	Vector2 delta = p_local_pos - c;
	float r = _compute_radius_px();
	if (delta.length() < r * 0.22f) {
		return false; // Zona muerta central para no parpadear al pasar sobre el centro
	}
	return select_by_angle_vector(delta);
}

bool ClockGearsRadialCanvas::activate_selected_slot() {
	if (selected_pos < 0 || selected_pos >= active_slot_indices.size()) {
		return false;
	}
	const RadialSlot &rs = all_slots[active_slot_indices[selected_pos]];
	if (rs.disabled) {
		return false;
	}
	emit_signal("slot_activated", selected_pos);
	return true;
}

Vector2 ClockGearsRadialCanvas::get_slot_local_pos(int p_index) const {
	int cnt = active_slot_indices.size();
	if (cnt <= 0 || p_index < 0 || p_index >= cnt) {
		return _compute_center_local();
	}
	Vector2 c = _compute_center_local();
	float r = _compute_radius_px();
	float a = _compute_slot_angle(p_index, cnt);
	return c + Vector2(Math::cos(a), Math::sin(a)) * r;
}

float ClockGearsRadialCanvas::get_slot_visual_scale(int p_index) const {
	if (p_index >= 0 && p_index < slot_visual_scales.size()) {
		return slot_visual_scales[p_index];
	}
	return 1.0f;
}

Dictionary ClockGearsRadialCanvas::get_selected_slot_info() const {
	Dictionary d;
	if (selected_pos < 0 || selected_pos >= active_slot_indices.size()) {
		return d;
	}
	const RadialSlot &rs = all_slots[active_slot_indices[selected_pos]];
	d["index"] = selected_pos;
	d["id"] = rs.id;
	d["title"] = rs.title;
	d["description"] = rs.description;
	d["icon_path"] = rs.icon_path;
	d["action"] = rs.action;
	d["actions"] = rs.actions.duplicate(true);
	d["disabled"] = rs.disabled;
	return d;
}

Dictionary ClockGearsRadialCanvas::get_center_displayed_info() const {
	Dictionary info = get_selected_slot_info();
	info["show_center_info"] = layout_cfg.has("radial_show_center_info") ? bool(layout_cfg["radial_show_center_info"]) : true;
	return info;
}

void ClockGearsRadialCanvas::gui_input(const Ref<InputEvent> &p_event) {
	if (!interactive || !p_event.is_valid()) {
		return;
	}
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		select_by_local_point(mm->get_position());
		return;
	}
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		Vector2 c = _compute_center_local();
		float r = _compute_radius_px();
		if ((mb->get_position() - c).length() >= r * 0.22f) {
			select_by_local_point(mb->get_position());
			if (activate_selected_slot()) {
				accept_event();
			}
		} else {
			if (activate_selected_slot()) {
				accept_event();
			}
		}
		return;
	}
}

void ClockGearsRadialCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		if (!anim_active) {
			set_process(false);
			return;
		}
		float dt = float(get_process_delta_time());
		float sel_sc = CLAMP(float(layout_cfg.get("radial_selected_scale", 1.28)), 1.0f, 1.8f);
		float speed = 18.0f * dt;
		bool still_moving = false;
		for (int i = 0; i < slot_visual_scales.size(); i++) {
			float target = (i == selected_pos) ? sel_sc : 1.0f;
			float cur = slot_visual_scales[i];
			float next = Math::lerp(cur, target, MIN(1.0f, speed));
			if (Math::abs(next - target) < 0.005f) {
				next = target;
			} else {
				still_moving = true;
			}
			slot_visual_scales.write[i] = next;
		}
		queue_redraw();
		if (!still_moving) {
			anim_active = false;
			set_process(false);
		}
	} else if (p_what == NOTIFICATION_DRAW) {
		Size2 sz = get_size();
		if (sz.x < 16.0f || sz.y < 16.0f) {
			return;
		}
		Vector2 center = _compute_center_local();
		float radius = _compute_radius_px();
		float base_slot = float(CLAMP(int(layout_cfg.get("radial_slot_size", 64)), 28, 140)) * ui_scale;
		float sweep_deg = CLAMP(float(layout_cfg.get("radial_sweep_angle_deg", 360.0)), 45.0f, 360.0f);
		float start_rad = Math::deg_to_rad(float(layout_cfg.get("radial_start_angle_deg", -90.0)));
		float sweep_rad = Math::deg_to_rad(sweep_deg);
		bool cw = layout_cfg.has("radial_clockwise") ? bool(layout_cfg["radial_clockwise"]) : true;
		Color ring_col = _cg_ui_parse_color(layout_cfg.get("radial_ring_color", "#1a1e2ad9"), Color(0.10f, 0.12f, 0.16f, 0.85f));
		Color accent_col = _cg_ui_parse_color(layout_cfg.get("radial_accent_color", "#f5bc54ff"), Color(0.96f, 0.74f, 0.33f, 1.0f));

		// 1. Pista circular / arco estilo Zelda 3D
		float end_rad = start_rad + (cw ? sweep_rad : -sweep_rad);
		float track_w = base_slot * 0.92f;
		_cg_draw_thick_arc(this, center, radius, MIN(start_rad, end_rad), MAX(start_rad, end_rad), track_w, ring_col, true);
		_cg_draw_thick_arc(this, center, radius + track_w * 0.5f, MIN(start_rad, end_rad), MAX(start_rad, end_rad), MAX(1.5f, 2.0f * ui_scale), Color(accent_col.r, accent_col.g, accent_col.b, 0.42f), true);
		_cg_draw_thick_arc(this, center, MAX(8.0f, radius - track_w * 0.5f), MIN(start_rad, end_rad), MAX(start_rad, end_rad), MAX(1.5f, 2.0f * ui_scale), Color(accent_col.r, accent_col.g, accent_col.b, 0.35f), true);

		int cnt = active_slot_indices.size();
		if (cnt <= 0) {
			return;
		}

		// 2. Rayo selector desde el centro hacia la ranura activa
		Vector2 sel_pos_px = get_slot_local_pos(selected_pos);
		Vector2 dir_u = (sel_pos_px - center);
		float dist = dir_u.length();
		if (dist > 1.0f) {
			dir_u /= dist;
			draw_line(center + dir_u * MAX(18.0f, radius * 0.44f), sel_pos_px - dir_u * (base_slot * 0.55f), Color(accent_col.r, accent_col.g, accent_col.b, 0.75f), MAX(2.0f, 3.0f * ui_scale), true);
		}

		Ref<Font> font = get_theme_default_font();
		int title_fsz = MAX(10, int(Math::round(15.0f * ui_scale)));
		int desc_fsz = MAX(9, int(Math::round(12.0f * ui_scale)));
		int badge_fsz = MAX(8, int(Math::round(10.0f * ui_scale)));

		// 3. Medallones de ranura (no seleccionados primero, activo al final para quedar arriba)
		for (int pass = 0; pass < 2; pass++) {
			for (int i = 0; i < cnt; i++) {
				bool is_sel = (i == selected_pos);
				if ((pass == 0 && is_sel) || (pass == 1 && !is_sel)) {
					continue;
				}
				const RadialSlot &rs = all_slots[active_slot_indices[i]];
				Vector2 sp = get_slot_local_pos(i);
				float sc = (i < slot_visual_scales.size()) ? slot_visual_scales[i] : (is_sel ? 1.28f : 1.0f);
				float srad = (base_slot * 0.46f) * sc;

				// Sombra bajo medallón
				draw_circle(sp + Vector2(0, 3.0f * ui_scale), srad + 2.0f * ui_scale, Color(0, 0, 0, is_sel ? 0.50f : 0.34f));
				if (is_sel) {
					draw_circle(sp, srad + 5.0f * ui_scale, Color(accent_col.r, accent_col.g, accent_col.b, 0.24f));
				}

				Color fill_c = is_sel ? Color(0.20f, 0.16f, 0.11f, 0.98f) : Color(0.11f, 0.12f, 0.16f, 0.94f);
				if (rs.disabled) {
					fill_c = Color(0.08f, 0.08f, 0.10f, 0.65f);
				}
				draw_circle(sp, srad, fill_c);

				Color rim_c = is_sel ? accent_col : Color(accent_col.r, accent_col.g, accent_col.b, rs.disabled ? 0.25f : 0.62f);
				draw_arc(sp, srad, 0.0f, float(Math::TAU), 36, rim_c, is_sel ? MAX(2.0f, 2.5f * ui_scale) : MAX(1.0f, 1.5f * ui_scale), true);

				if (rs.cached_icon.is_valid()) {
					float icon_side = srad * 1.32f;
					if (rs.pixel_art) {
						float tex_w = MAX(1.0f, rs.cached_icon->get_width());
						int int_mult = MAX(1, int(Math::floor(icon_side / tex_w)));
						icon_side = MIN(srad * 1.45f, tex_w * float(int_mult));
					}
					Rect2 icon_r(sp - Vector2(icon_side * 0.5f, icon_side * 0.5f), Vector2(icon_side, icon_side));
					Color icon_mod = rs.disabled ? Color(1, 1, 1, 0.4f) : Color(1, 1, 1, 1);
					draw_texture_rect(rs.cached_icon, icon_r, false, icon_mod);
				} else if (font.is_valid()) {
					String abbr = ClockGearsDataManager::truncate_with_ellipsis(rs.title, 4);
					Vector2 tsz = font->get_string_size(abbr, HORIZONTAL_ALIGNMENT_LEFT, -1, badge_fsz);
					Color tc = rs.disabled ? Color(0.6f, 0.6f, 0.65f, 0.7f) : (is_sel ? accent_col : Color(0.95f, 0.93f, 0.88f, 1.0f));
					draw_string(font, sp + Vector2(-tsz.x * 0.5f, tsz.y * 0.30f), abbr, HORIZONTAL_ALIGNMENT_LEFT, -1, badge_fsz, tc);
				}
			}
		}

		// 4. Núcleo central con icono grande, título y descripción
		bool show_center = layout_cfg.has("radial_show_center_info") ? bool(layout_cfg["radial_show_center_info"]) : true;
		if (show_center) {
			const RadialSlot &active = all_slots[active_slot_indices[selected_pos]];
			float hub_r = CLAMP(radius - base_slot * 0.62f, 36.0f * ui_scale, 108.0f * ui_scale);
			draw_circle(center + Vector2(0, 3.0f * ui_scale), hub_r + 3.0f * ui_scale, Color(0, 0, 0, 0.45f));
			draw_circle(center, hub_r, Color(0.09f, 0.08f, 0.07f, 0.92f));
			draw_arc(center, hub_r, 0.0f, float(Math::TAU), 48, Color(accent_col.r, accent_col.g, accent_col.b, 0.78f), MAX(1.5f, 2.0f * ui_scale), true);

			float y_off = -hub_r * 0.45f;
			if (active.cached_icon.is_valid()) {
				float c_ic = MIN(hub_r * 0.68f, 44.0f * ui_scale);
				Rect2 ic_r(center + Vector2(-c_ic * 0.5f, -hub_r * 0.68f), Vector2(c_ic, c_ic));
				draw_texture_rect(active.cached_icon, ic_r, false, active.disabled ? Color(1, 1, 1, 0.4f) : Color(1, 1, 1, 1));
				y_off = 4.0f * ui_scale;
			}
			if (font.is_valid()) {
				String title_txt = ClockGearsDataManager::truncate_with_ellipsis(active.title, 20);
				float max_tw = hub_r * 1.72f;
				draw_string(font, center + Vector2(-max_tw * 0.5f, y_off + float(title_fsz) * 0.35f),
						title_txt, HORIZONTAL_ALIGNMENT_CENTER, max_tw, title_fsz, accent_col);
				if (!active.description.is_empty()) {
					String desc_txt = ClockGearsDataManager::truncate_with_ellipsis(active.description, 44);
					draw_multiline_string(font, center + Vector2(-max_tw * 0.5f, y_off + float(title_fsz) * 0.95f),
							desc_txt, HORIZONTAL_ALIGNMENT_CENTER, max_tw, desc_fsz, 2, Color(0.92f, 0.89f, 0.82f, 0.96f));
				}
			}
		}
	}
}

// ============================================================================
// ClockGearsMenuCursorOverlay (3G)
// ============================================================================

ClockGearsMenuCursorOverlay::ClockGearsMenuCursorOverlay() {
	set_name("MenuCursorOverlay");
	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
}

void ClockGearsMenuCursorOverlay::configure(const Dictionary &p_cursor_cfg, float p_scale) {
	cursor_cfg = p_cursor_cfg.duplicate(true);
	ui_scale = MAX(0.25f, p_scale);
	String ip = String(cursor_cfg.get("image_path", "")).strip_edges();
	cursor_tex = !ip.is_empty() ? ClockGearsWindowStyleManager::load_texture_safe(ip) : Ref<Texture2D>();
	bool pa = cursor_cfg.has("pixel_art") ? bool(cursor_cfg["pixel_art"]) : true;
	set_texture_filter(pa ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
	bool enabled = bool(cursor_cfg.get("enabled", false));
	String anim = String(cursor_cfg.get("animation", "pulse")).strip_edges().to_lower();
	set_visible(enabled);
	set_process(enabled && anim != "none");
	queue_redraw();
}

void ClockGearsMenuCursorOverlay::set_target_rect(const Rect2 &p_rect, bool p_visible) {
	target_rect = p_rect;
	has_target = p_visible;
	queue_redraw();
}

Dictionary ClockGearsMenuCursorOverlay::get_cursor_state() const {
	Dictionary d;
	d["enabled"] = bool(cursor_cfg.get("enabled", false));
	d["has_target"] = has_target;
	d["target_rect"] = target_rect;
	d["shape"] = String(cursor_cfg.get("shape", "ring"));
	d["animation"] = String(cursor_cfg.get("animation", "pulse"));
	return d;
}

void ClockGearsMenuCursorOverlay::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		if (!is_visible_in_tree() || !has_target) {
			return;
		}
		anim_phase += get_process_delta_time() * 4.5;
		queue_redraw();
	} else if (p_what == NOTIFICATION_DRAW) {
		if (!has_target || !bool(cursor_cfg.get("enabled", false))) {
			return;
		}
		String cshape = String(cursor_cfg.get("shape", "ring")).strip_edges().to_lower();
		String canim = String(cursor_cfg.get("animation", "pulse")).strip_edges().to_lower();
		Color ccol = _cg_ui_parse_color(cursor_cfg.get("color", "#f5bc54ff"), Color(0.96f, 0.74f, 0.33f, 1.0f));
		float stroke_w = float(CLAMP(int(cursor_cfg.get("size", 3)), 1, 24)) * ui_scale;
		Rect2 r = target_rect;
		float pulse = (canim == "pulse") ? (2.0f * float(Math::sin(anim_phase)) * ui_scale) : 0.0f;
		float bob = (canim == "bob") ? (3.0f * float(Math::sin(anim_phase)) * ui_scale) : 0.0f;
		r = r.grow(2.0f * ui_scale + pulse);
		r.position.x += bob;

		if (cshape == "image" && cursor_tex.is_valid()) {
			float isz = CLAMP(r.size.y * 0.75f, 14.0f * ui_scale, 36.0f * ui_scale);
			Rect2 icon_r(Vector2(r.position.x - isz - 4.0f * ui_scale, r.position.y + (r.size.y - isz) * 0.5f), Vector2(isz, isz));
			draw_texture_rect(cursor_tex, icon_r, false, ccol);
			return;
		}
		if (cshape == "arrow") {
			float ah = MIN(r.size.y * 0.45f, 14.0f * ui_scale);
			float cy = r.position.y + r.size.y * 0.5f;
			float tip_x = r.position.x - 4.0f * ui_scale;
			Vector<Vector2> tri;
			tri.push_back(Vector2(tip_x, cy));
			tri.push_back(Vector2(tip_x - ah, cy - ah * 0.7f));
			tri.push_back(Vector2(tip_x - ah, cy + ah * 0.7f));
			draw_colored_polygon(tri, ccol);
			return;
		}
		String poly_shape = (cshape == "ring" || cshape == "rounded") ? String("rounded") : cshape;
		Vector<Vector2> pts = ClockGearsWindowStyleBox::build_shape_polygon(r, poly_shape, 8.0f * ui_scale, 20);
		if (pts.size() >= 3) {
			pts.push_back(pts[0]);
			draw_polyline(pts, ccol, stroke_w, true);
		}
	}
}

// ============================================================================
// ClockGearsUiDesignLayer (3D - 3G)
// ============================================================================

Vector2 ClockGearsUiDesignLayer::compute_anchor_factors(const String &p_anchor) {
	float ax = 0.0f;
	float ay = 0.0f;
	if (p_anchor == "top_center") {
		ax = 0.5f;
		ay = 0.0f;
	} else if (p_anchor == "top_right") {
		ax = 1.0f;
		ay = 0.0f;
	} else if (p_anchor == "middle_left") {
		ax = 0.0f;
		ay = 0.5f;
	} else if (p_anchor == "center") {
		ax = 0.5f;
		ay = 0.5f;
	} else if (p_anchor == "middle_right") {
		ax = 1.0f;
		ay = 0.5f;
	} else if (p_anchor == "bottom_left") {
		ax = 0.0f;
		ay = 1.0f;
	} else if (p_anchor == "bottom_center") {
		ax = 0.5f;
		ay = 1.0f;
	} else if (p_anchor == "bottom_right") {
		ax = 1.0f;
		ay = 1.0f;
	}
	return Vector2(ax, ay);
}

Rect2 ClockGearsUiDesignLayer::compute_stage_rect(const Vector2 &p_viewport_size) {
	float w = MAX(64.0f, p_viewport_size.x);
	float h = MAX(64.0f, p_viewport_size.y);
	float s = MAX(0.25f, MIN(w / LOGICAL_WIDTH, h / LOGICAL_HEIGHT));
	Size2 stage_sz(LOGICAL_WIDTH * s, LOGICAL_HEIGHT * s);
	Vector2 origin((w - stage_sz.x) * 0.5f, (h - stage_sz.y) * 0.5f);
	return Rect2(origin, stage_sz);
}

Rect2 ClockGearsUiDesignLayer::compute_element_rect(const Dictionary &p_elem, const Rect2 &p_stage_rect, float p_scale) {
	String anchor = String(p_elem.get("anchor", "top_left")).strip_edges().to_lower();
	float ew = float(CLAMP(int(p_elem.get("width", 120)), 16, 960));
	float eh = float(CLAMP(int(p_elem.get("height", 36)), 16, 540));
	float ox = float(int(p_elem.get("offset_x", 0)));
	float oy = float(int(p_elem.get("offset_y", 0)));

	Vector2 af = compute_anchor_factors(anchor);
	float base_x = af.x * (LOGICAL_WIDTH - ew) + ox;
	float base_y = af.y * (LOGICAL_HEIGHT - eh) + oy;
	Vector2 pos(Math::round(p_stage_rect.position.x + base_x * p_scale), Math::round(p_stage_rect.position.y + base_y * p_scale));
	Vector2 sz(MAX(4.0f, Math::round(ew * p_scale)), MAX(4.0f, Math::round(eh * p_scale)));
	return Rect2(pos, sz);
}

String ClockGearsUiDesignLayer::resolve_element_text(const Dictionary &p_elem, const ClockGearsDataManager *p_dm) {
	String etype = String(p_elem.get("type", "text")).strip_edges().to_lower();
	String binding = String(p_elem.get("data_binding", "none")).strip_edges().to_lower();
	String text_mode = String(p_elem.get("text_mode", "fixed")).strip_edges().to_lower();
	String var_name = String(p_elem.get("variable_name", "")).strip_edges();

	if ((binding.is_empty() || binding == "none") && (etype == "variable" || text_mode == "variable") && !var_name.is_empty()) {
		binding = "variable";
	} else if ((binding.is_empty() || binding == "none") && etype == "variable") {
		binding = "variable";
	}

	String raw_text = String(p_elem.get("text", ""));
	String prefix = String(p_elem.get("prefix", ""));

	if (binding != "none" && p_dm != nullptr) {
		String resolved = p_dm->resolve_ui_data_binding(binding, p_elem);
		if (raw_text.contains("{value}")) {
			return raw_text.replace("{value}", resolved);
		}
		if (!prefix.is_empty()) {
			return prefix + resolved;
		}
		return resolved;
	}
	return prefix.is_empty() ? raw_text : (prefix + raw_text);
}

Ref<Texture2D> ClockGearsUiDesignLayer::make_builtin_heart_texture(int p_mode) {
	static const char *k_mask[16] = {
		"................",
		"..OOOO....OOOO..",
		".OFFFFO..OFFFFO.",
		"OFFHHFFOOFFFFFFO",
		"OFHHFFFFFFFFFFFO",
		"OFFFFFFFFFFFFFFO",
		"OFFFFFFFFFFFFFFO",
		"OFFFFFFFFFFFFFFO",
		".OFFFFFFFFFFFFO.",
		"..OFFFFFFFFFFO..",
		"...OFFFFFFFFO...",
		"....OFFFFFFO....",
		".....OFFFFO.....",
		"......OFFO......",
		".......OO.......",
		"................"
	};
	Ref<Image> img = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
	Color outline(0.10f, 0.06f, 0.08f, 1.0f);
	Color full_c(0.88f, 0.16f, 0.24f, 1.0f);
	Color hi_c(0.98f, 0.60f, 0.65f, 1.0f);
	Color empty_c(0.18f, 0.20f, 0.26f, 0.85f);

	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			char ch = k_mask[y][x];
			if (ch == '.') {
				img->set_pixel(x, y, Color(0, 0, 0, 0));
			} else if (ch == 'O') {
				img->set_pixel(x, y, outline);
			} else {
				bool filled = (p_mode == 2) || (p_mode == 1 && x < 8);
				img->set_pixel(x, y, filled ? ((ch == 'H') ? hi_c : full_c) : empty_c);
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

Ref<Texture2D> ClockGearsUiDesignLayer::make_builtin_coin_texture() {
	Ref<Image> img = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
	Color rim(0.85f, 0.58f, 0.10f, 1.0f);
	Color gold(0.99f, 0.84f, 0.22f, 1.0f);
	Color shine(1.0f, 0.96f, 0.65f, 1.0f);
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			float dx = (float(x) - 7.5f) / 6.5f;
			float dy = (float(y) - 7.5f) / 6.5f;
			float r2 = dx * dx + dy * dy;
			if (r2 > 1.0f) {
				img->set_pixel(x, y, Color(0, 0, 0, 0));
			} else if (r2 > 0.72f) {
				img->set_pixel(x, y, rim);
			} else if (dx < -0.1f && dy < -0.1f && r2 > 0.12f) {
				img->set_pixel(x, y, shine);
			} else {
				img->set_pixel(x, y, gold);
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

void ClockGearsUiDesignLayer::set_preview_mode(bool p_preview) {
	preview_mode = p_preview;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].button) {
			elements[i].button->set_mouse_filter(p_preview ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
			elements[i].button->set_focus_mode(p_preview ? FOCUS_NONE : FOCUS_ALL);
		}
	}
}

void ClockGearsUiDesignLayer::set_viewport_size_override(const Vector2 &p_size) {
	viewport_size_override = p_size;
	last_layout_size = Vector2(-1, -1);
	_relayout_elements(true);
}

void ClockGearsUiDesignLayer::set_action_disabled(const String &p_action, bool p_disabled) {
	String key = p_action.strip_edges().to_lower();
	if (key.is_empty()) {
		return;
	}
	if (p_disabled) {
		disabled_actions[key] = true;
	} else {
		disabled_actions.erase(key);
	}
	refresh_live_values();
}

void ClockGearsUiDesignLayer::clear_disabled_actions() {
	disabled_actions.clear();
	refresh_live_values();
}

Size2 ClockGearsUiDesignLayer::_resolve_effective_viewport_size() const {
	if (viewport_size_override.x >= 64.0f && viewport_size_override.y >= 64.0f) {
		return viewport_size_override;
	}
	if (is_inside_tree() && get_viewport()) {
		Size2 vr = get_viewport()->get_visible_rect().size;
		if (vr.x >= 64.0f && vr.y >= 64.0f) {
			return vr;
		}
	}
	Size2 self_sz = get_size();
	if (self_sz.x >= 64.0f && self_sz.y >= 64.0f) {
		return self_sz;
	}
	return Size2(LOGICAL_WIDTH, LOGICAL_HEIGHT);
}

void ClockGearsUiDesignLayer::_clear_aux_layout_controls() {
	for (int i = 0; i < layout_aux_buttons.size(); i++) {
		Button *b = layout_aux_buttons[i];
		if (b) {
			if (b->get_parent() == this) {
				remove_child(b);
			}
			memdelete(b);
		}
	}
	layout_aux_buttons.clear();
	if (radial_canvas) {
		if (radial_canvas->get_parent() == this) {
			remove_child(radial_canvas);
		}
		memdelete(radial_canvas);
		radial_canvas = nullptr;
	}
}

void ClockGearsUiDesignLayer::clear_elements() {
	_clear_aux_layout_controls();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].root) {
			if (elements[i].root->get_parent() == this) {
				remove_child(elements[i].root);
			}
			memdelete(elements[i].root);
			elements.write[i].root = nullptr;
		}
	}
	elements.clear();
	if (cursor_overlay) {
		cursor_overlay->set_target_rect(Rect2(), false);
	}
}

Ref<Font> ClockGearsUiDesignLayer::_load_cached_font(const String &p_path) {
	String p = p_path.strip_edges();
	if (p.is_empty()) {
		return Ref<Font>();
	}
	if (s_cg_ui_font_cache.has(p)) {
		return s_cg_ui_font_cache[p];
	}
	Ref<Font> f;
	if (ResourceLoader::exists(p)) {
		f = ResourceLoader::load(p);
	}
	if (f.is_null() && FileAccess::exists(p)) {
		Ref<FontFile> ff;
		ff.instantiate();
		if (ff->load_dynamic_font(p) == OK) {
			f = ff;
		}
	}
	if (f.is_valid()) {
		s_cg_ui_font_cache[p] = f;
	}
	return f;
}

void ClockGearsUiDesignLayer::_ensure_background_nodes() {
	if (!bg_blur_rect) {
		bg_blur_rect = memnew(TextureRect);
		bg_blur_rect->set_name("MenuBgBlur");
		bg_blur_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bg_blur_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		bg_blur_rect->set_stretch_mode(TextureRect::STRETCH_SCALE);
		bg_blur_rect->set_texture_filter(TEXTURE_FILTER_LINEAR);
		bg_blur_rect->set_z_index(-210);
		bg_blur_rect->hide();
		add_child(bg_blur_rect);
	}
	if (!bg_backdrop_ctrl) {
		ClockGearsUiBackdropDraw *bd = memnew(ClockGearsUiBackdropDraw);
		bd->set_name("MenuBgBackdrop");
		bd->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bd->set_z_index(-195);
		bd->hide();
		add_child(bd);
		bg_backdrop_ctrl = bd;
	}
	if (!bg_veil_rect) {
		bg_veil_rect = memnew(ColorRect);
		bg_veil_rect->set_name("MenuBgVeil");
		bg_veil_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bg_veil_rect->set_z_index(-185);
		bg_veil_rect->hide();
		add_child(bg_veil_rect);
	}
}

void ClockGearsUiDesignLayer::capture_backdrop_from_viewport() {
	if (preview_mode || !is_inside_tree() || !get_viewport()) {
		return;
	}
	Ref<Texture2D> vtex = get_viewport()->get_texture();
	if (vtex.is_valid()) {
		Ref<Image> img = vtex->get_image();
		if (img.is_valid() && !img->is_empty()) {
			captured_backdrop_img = img->duplicate();
			last_blur_intensity = -1; // Forzar regeneración única con la captura nueva
		}
	}
}

void ClockGearsUiDesignLayer::_rebuild_cached_blur_texture(int p_intensity) {
	int inten = CLAMP(p_intensity, 0, 100);
	if (cached_blur_texture.is_valid() && last_blur_intensity == inten && last_blur_preview_mode == preview_mode) {
		return;
	}
	last_blur_intensity = inten;
	last_blur_preview_mode = preview_mode;

	Ref<Image> src;
	if (!preview_mode && captured_backdrop_img.is_valid() && !captured_backdrop_img->is_empty()) {
		src = captured_backdrop_img->duplicate();
	} else {
		// Muestra estilizada cacheada del juego para ver el desenfoque real en el lienzo
		src = Image::create_empty(160, 90, false, Image::FORMAT_RGBA8);
		for (int y = 0; y < 90; y++) {
			for (int x = 0; x < 160; x++) {
				bool checker = (((x / 12) + (y / 12)) % 2) == 0;
				Color c = checker ? Color(0.18f, 0.34f, 0.24f, 1.0f) : Color(0.13f, 0.26f, 0.20f, 1.0f);
				if ((x - 80) * (x - 80) + (y - 45) * (y - 45) < 420) {
					c = Color(0.55f, 0.42f, 0.24f, 1.0f);
				} else if (x > 110 && y < 35) {
					c = Color(0.24f, 0.40f, 0.62f, 1.0f);
				}
				src->set_pixel(x, y, c);
			}
		}
	}
	int sw = src->get_width();
	int sh = src->get_height();
	if (inten > 0 && sw > 8 && sh > 8) {
		int div = CLAMP(1 + (inten * 20) / 100, 1, 22);
		int dw = MAX(10, sw / div);
		int dh = MAX(6, sh / div);
		src->resize(dw, dh, Image::INTERPOLATE_BILINEAR);
		src->resize(MIN(320, sw), MIN(180, sh), Image::INTERPOLATE_BILINEAR);
	}
	if (cached_blur_texture.is_null()) {
		cached_blur_texture = ImageTexture::create_from_image(src);
	} else {
		cached_blur_texture->set_image(src);
	}
}

void ClockGearsUiDesignLayer::_apply_screen_background(const Dictionary &p_design, const Size2 &p_vp) {
	_ensure_background_nodes();
	Dictionary bg = (p_design.has("background") && p_design["background"].get_type() == Variant::DICTIONARY)
			? Dictionary(p_design["background"])
			: Dictionary();
	bool blur_en = bool(bg.get("blur_enabled", false));
	int blur_int = CLAMP(int(bg.get("blur_intensity", 45)), 0, 100);
	if (bg_blur_rect) {
		bg_blur_rect->set_visible(blur_en);
		if (blur_en) {
			_rebuild_cached_blur_texture(blur_int);
			bg_blur_rect->set_texture(cached_blur_texture);
			bg_blur_rect->set_position(Vector2());
			bg_blur_rect->set_size(p_vp);
		}
	}
	bool veil = bool(bg.get("veil_enabled", bg.get("dark_veil", false)));
	if (bg_veil_rect) {
		bg_veil_rect->set_visible(veil);
		if (veil) {
			bg_veil_rect->set_position(Vector2());
			bg_veil_rect->set_size(p_vp);
			bg_veil_rect->set_color(_cg_ui_parse_color(bg.get("veil_color", "#05070ca6"), Color(0.02f, 0.03f, 0.05f, 0.65f)));
		}
	}
	String bt = String(bg.get("type", "none")).strip_edges().to_lower();
	if (bt == "image") {
		String ip = String(bg.get("image_path", "")).strip_edges();
		if (ip != cached_bg_path) {
			cached_bg_path = ip;
			cached_bg_texture = ip.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(ip);
		}
	} else {
		cached_bg_texture.unref();
		cached_bg_path = "";
	}
	if (ClockGearsUiBackdropDraw *bd = Object::cast_to<ClockGearsUiBackdropDraw>(bg_backdrop_ctrl)) {
		bool show_bd = (bt == "color" || bt == "gradient" || (bt == "image" && cached_bg_texture.is_valid()));
		bd->set_visible(show_bd);
		bd->set_position(Vector2());
		bd->set_size(p_vp);
		bd->configure(bg, cached_bg_texture);
		bd->set_scroll_offset(bg_scroll_offset);
	}
}

float ClockGearsUiDesignLayer::_eval_transition_curve(float p_t) const {
	float t = CLAMP(p_t, 0.0f, 1.0f);
	if (transition_curve == "linear") {
		return t;
	}
	if (transition_curve == "ease_in_out") {
		return t * t * (3.0f - 2.0f * t);
	}
	if (transition_curve == "back_out") {
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		float u = t - 1.0f;
		return 1.0f + c3 * u * u * u + c1 * u * u;
	}
	// ease_out
	float u = 1.0f - t;
	return 1.0f - u * u * u;
}

void ClockGearsUiDesignLayer::_apply_transition_pose(float p_progress) {
	float v = _eval_transition_curve(p_progress);
	if (transition_is_out) {
		v = 1.0f - v;
	}
	Size2 vp = _resolve_effective_viewport_size();
	set_pivot_offset(vp * 0.5f);
	set_rotation(0.0f);
	set_scale(Vector2(1, 1));
	set_position(Vector2());
	set_modulate(Color(1, 1, 1, 1));

	if (transition_type == "fade") {
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	} else if (transition_type == "slide_left") {
		set_position(Vector2((v - 1.0f) * vp.x * 0.35f, 0.0f));
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	} else if (transition_type == "slide_right") {
		set_position(Vector2((1.0f - v) * vp.x * 0.35f, 0.0f));
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	} else if (transition_type == "slide_top") {
		set_position(Vector2(0.0f, (v - 1.0f) * vp.y * 0.35f));
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	} else if (transition_type == "slide_bottom") {
		set_position(Vector2(0.0f, (1.0f - v) * vp.y * 0.35f));
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	} else if (transition_type == "scale") {
		float sc = Math::lerp(0.78f, 1.0f, v);
		set_scale(Vector2(sc, sc));
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	} else if (transition_type == "radial_spin") {
		float sc = Math::lerp(0.65f, 1.0f, v);
		set_scale(Vector2(sc, sc));
		if (radial_canvas) {
			radial_canvas->set_pivot_offset(radial_canvas->get_size() * 0.5f);
			radial_canvas->set_rotation(Math::deg_to_rad((1.0f - v) * -55.0f));
		}
		set_modulate(Color(1, 1, 1, CLAMP(v, 0.0f, 1.0f)));
	}
}

void ClockGearsUiDesignLayer::play_transition_in() {
	Dictionary tr = (active_design.has("transition") && active_design["transition"].get_type() == Variant::DICTIONARY)
			? Dictionary(active_design["transition"])
			: Dictionary();
	String it = String(tr.get("in_type", "none")).strip_edges().to_lower();
	double dur = double(tr.get("duration_sec", 0.16));
	if (it == "none" || it.is_empty() || dur <= 0.005 || preview_mode) {
		transition_active = false;
		transition_type = "none";
		_apply_transition_pose(1.0f);
		return;
	}
	transition_type = it;
	transition_curve = String(tr.get("curve", "ease_out")).strip_edges().to_lower();
	transition_duration = dur;
	transition_elapsed = 0.0;
	transition_is_out = false;
	transition_active = true;
	_apply_transition_pose(0.0f);
}

void ClockGearsUiDesignLayer::play_transition_out() {
	Dictionary tr = (active_design.has("transition") && active_design["transition"].get_type() == Variant::DICTIONARY)
			? Dictionary(active_design["transition"])
			: Dictionary();
	String ot = String(tr.get("out_type", "none")).strip_edges().to_lower();
	double dur = double(tr.get("duration_sec", 0.16));
	if (ot == "none" || ot.is_empty() || dur <= 0.005 || preview_mode) {
		transition_active = false;
		transition_type = "none";
		return;
	}
	transition_type = ot;
	transition_curve = String(tr.get("curve", "ease_out")).strip_edges().to_lower();
	transition_duration = dur;
	transition_elapsed = 0.0;
	transition_is_out = true;
	transition_active = true;
	_apply_transition_pose(0.0f);
}

void ClockGearsUiDesignLayer::apply_screen_design(const String &p_screen_id, const Dictionary &p_design_override) {
	screen_id = ClockGearsDataManager::normalize_menu_id(p_screen_id);
	Dictionary design;
	if (!p_design_override.is_empty()) {
		design = ClockGearsDataManager::normalize_ui_screen_design(screen_id, p_design_override);
	} else if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		design = dm->get_ui_screen_design(screen_id);
	} else {
		design = ClockGearsDataManager::get_default_ui_screen_design(screen_id);
	}
	apply_prebuilt_screen_design(screen_id, design);
}

void ClockGearsUiDesignLayer::apply_prebuilt_screen_design(const String &p_screen_id, const Dictionary &p_norm_design) {
	screen_id = ClockGearsDataManager::normalize_menu_id(p_screen_id);
	active_design = p_norm_design;
	current_layout_page = 0;
	Dictionary lay = p_norm_design.get("layout", Dictionary());
	current_layout_tab = MAX(0, int(lay.get("active_tab", 0)));
	current_carousel_index = 0;
	_rebuild_from_design(p_norm_design);
	last_layout_size = Vector2(-1, -1);
	_relayout_elements(true);
	refresh_live_values();
}

void ClockGearsUiDesignLayer::_rebuild_from_design(const Dictionary &p_design) {
	_clear_aux_layout_controls();
	Dictionary lay = p_design.get("layout", Dictionary());
	String lmode = String(lay.get("mode", "free")).strip_edges().to_lower();

	// Reutilización en sitio de nodos simples existentes (text / frame sin recorte) para cambios instantáneos (5C)
	Vector<ElementEntry> reusable = elements;
	elements.clear();
	Vector<bool> used;
	used.resize(reusable.size());
	for (int i = 0; i < used.size(); i++) {
		used.write[i] = false;
	}

	Array raw = p_design.get("elements", Array());
	if (lmode == "radial") {
		radial_canvas = memnew(ClockGearsRadialCanvas);
		radial_canvas->connect("slot_activated", callable_mp(this, &ClockGearsUiDesignLayer::_on_radial_slot_activated));
		radial_canvas->connect("selection_changed", callable_mp(this, &ClockGearsUiDesignLayer::_on_radial_selection_changed));
		add_child(radial_canvas);
		radial_canvas->configure(lay, raw, MAX(0.25f, current_ui_scale), !preview_mode);
	}

	for (int i = 0; i < raw.size(); i++) {
		if (raw[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary cfg = raw[i];
		String etype = String(cfg.get("type", "text")).strip_edges().to_lower();
		// En modo radial dibujado por anillo propio, las ranuras (botones) las dibuja ClockGearsRadialCanvas (0 nodos por ranura)
		if (lmode == "radial" && etype == "button") {
			continue;
		}
		ElementEntry entry;
		entry.id = String(cfg.get("id", vformat("elem_%d", i + 1)));
		entry.name = String(cfg.get("name", entry.id));
		entry.type = etype;
		entry.anchor = String(cfg.get("anchor", "top_left"));
		entry.offset_x = int(cfg.get("offset_x", 0));
		entry.offset_y = int(cfg.get("offset_y", 0));
		entry.width = CLAMP(int(cfg.get("width", 120)), 16, 960);
		entry.height = CLAMP(int(cfg.get("height", 36)), 16, 540);
		entry.layer_order = int(cfg.get("layer_order", 0));
		entry.visible = bool(cfg.get("visible", true));
		entry.window_style_id = String(cfg.get("window_style_id", "")).strip_edges().to_lower();
		entry.data_binding = String(cfg.get("data_binding", "none")).strip_edges().to_lower();
		entry.widget_kind = String(cfg.get("widget_kind", "")).strip_edges().to_lower();
		entry.action = String(cfg.get("action", "")).strip_edges().to_lower();
		entry.actions = cfg.has("actions") && cfg["actions"].get_type() == Variant::ARRAY ? Array(cfg["actions"]).duplicate(true) : Array();
		entry.focus_order = int(cfg.get("focus_order", i));
		entry.array_order = i;
		entry.tab_index = MAX(0, int(cfg.get("tab_index", 0)));
		entry.config = cfg;

		bool has_shape = cfg.has("shape") && cfg["shape"].get_type() == Variant::DICTIONARY;
		bool can_reuse_frame = (etype == "frame" && String(cfg.get("image_path", "")).strip_edges().is_empty());
		bool reused_ok = false;
		if (!has_shape && (etype == "text" || can_reuse_frame)) {
			for (int r = 0; r < reusable.size(); r++) {
				if (!used[r] && reusable[r].root && reusable[r].type == etype && !reusable[r].clip_host && (etype != "frame" || String(reusable[r].config.get("image_path", "")).strip_edges().is_empty())) {
					used.write[r] = true;
					entry.root = reusable[r].root;
					entry.bg_panel = reusable[r].bg_panel;
					entry.content_root = reusable[r].content_root;
					entry.label = reusable[r].label;
					String target_name = vformat("UiElem_%s", entry.id);
					if (entry.root->get_name() != target_name) {
						entry.root->set_name(target_name);
					}
					if (entry.root->get_z_index() != entry.layer_order) {
						entry.root->set_z_index(entry.layer_order);
					}
					if (entry.root->is_visible() != entry.visible) {
						entry.root->set_visible(entry.visible);
					}
					if (entry.bg_panel) {
						Color bg_mod(1, 1, 1, etype == "frame" ? CLAMP(float(cfg.get("opacity", 1.0)), 0.0f, 1.0f) : 1.0f);
						if (entry.bg_panel->get_modulate() != bg_mod) {
							entry.bg_panel->set_modulate(bg_mod);
						}
					}
					if (entry.label) {
						String al = String(cfg.get("align", "left"));
						HorizontalAlignment ha = (al == "center") ? HORIZONTAL_ALIGNMENT_CENTER : ((al == "right") ? HORIZONTAL_ALIGNMENT_RIGHT : HORIZONTAL_ALIGNMENT_LEFT);
						if (entry.label->get_horizontal_alignment() != ha) {
							entry.label->set_horizontal_alignment(ha);
						}
						Color fcol = _cg_ui_parse_color(cfg.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1));
						if (!entry.label->has_theme_color_override("font_color") || entry.label->get_theme_color("font_color") != fcol) {
							entry.label->add_theme_color_override("font_color", fcol);
						}
					}
					reused_ok = true;
					break;
				}
			}
		}
		if (!reused_ok) {
			_build_element_entry(entry);
		}
		elements.push_back(entry);
	}

	for (int r = 0; r < reusable.size(); r++) {
		if (!used[r] && reusable[r].root) {
			if (reusable[r].root->get_parent() == this) {
				remove_child(reusable[r].root);
			}
			memdelete(reusable[r].root);
		}
	}

	if (!cursor_overlay) {
		cursor_overlay = memnew(ClockGearsMenuCursorOverlay);
		cursor_overlay->set_z_index(1050);
		add_child(cursor_overlay);
	} else {
		move_child(cursor_overlay, get_child_count() - 1);
	}
	cursor_overlay->configure(p_design.get("cursor", Dictionary()), MAX(0.25f, current_ui_scale));
}

void ClockGearsUiDesignLayer::_build_element_entry(ElementEntry &r_entry) {
	Control *root = memnew(Control);
	root->set_name(vformat("UiElem_%s", r_entry.id));
	const bool allow_pass = (r_entry.type == "button") || (r_entry.type == "widget" && !preview_mode);
	root->set_mouse_filter(allow_pass ? MOUSE_FILTER_PASS : MOUSE_FILTER_IGNORE);
	root->set_z_index(r_entry.layer_order);
	root->set_visible(r_entry.visible);
	add_child(root);
	r_entry.root = root;

	const bool has_custom_shape = r_entry.config.has("shape") && r_entry.config["shape"].get_type() == Variant::DICTIONARY;
	Dictionary shape_cfg = has_custom_shape ? Dictionary(r_entry.config["shape"]) : Dictionary();
	String shape_t = has_custom_shape ? String(shape_cfg.get("shape_type", "rounded")).strip_edges().to_lower() : String();
	bool want_clip = has_custom_shape && (shape_cfg.has("clip_content") ? bool(shape_cfg["clip_content"]) : (shape_t == "circle" || shape_t == "ellipse" || shape_t == "pill"));

	PanelContainer *bg = memnew(PanelContainer);
	bg->set_name("BgPanel");
	bg->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	root->add_child(bg);
	r_entry.bg_panel = bg;

	Control *content_parent = root;
	if (want_clip && shape_t != "image") {
		ClockGearsShapedClipControl *clip = memnew(ClockGearsShapedClipControl);
		clip->set_name("ShapedClip");
		clip->set_mouse_filter(r_entry.type == "button" ? MOUSE_FILTER_PASS : MOUSE_FILTER_IGNORE);
		clip->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		root->add_child(clip);
		r_entry.clip_host = clip;
		content_parent = clip;

		ClockGearsShapedBorderOverlay *bov = memnew(ClockGearsShapedBorderOverlay);
		bov->set_name("ShapedBorderOverlay");
		bov->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bov->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		bov->set_z_index(1);
		root->add_child(bov);
		r_entry.border_overlay = bov;
	}

	Control *content = memnew(Control);
	content->set_name("ContentRoot");
	content->set_mouse_filter(allow_pass ? MOUSE_FILTER_PASS : MOUSE_FILTER_IGNORE);
	content_parent->add_child(content);
	r_entry.content_root = content;

	TextureFilter tf = bool(r_entry.config.get("pixel_art", true)) ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR;

	if (r_entry.type == "frame") {
		float op = CLAMP(float(r_entry.config.get("opacity", 1.0)), 0.0f, 1.0f);
		bg->set_modulate(Color(1, 1, 1, op));
		String fip = String(r_entry.config.get("image_path", "")).strip_edges();
		if (!fip.is_empty()) {
			Ref<Texture2D> ftex = ClockGearsWindowStyleManager::load_texture_safe(fip);
			if (ftex.is_valid()) {
				String sm = String(r_entry.config.get("scale_mode", "stretch")).strip_edges().to_lower();
				int margin = CLAMP(int(r_entry.config.get("nine_slice_margin", 12)), 0, 128);
				if (sm == "nine_slice") {
					NinePatchRect *np = memnew(NinePatchRect);
					np->set_name("FrameNinePatch");
					np->set_mouse_filter(MOUSE_FILTER_IGNORE);
					np->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
					np->set_texture(ftex);
					np->set_patch_margin(SIDE_LEFT, margin);
					np->set_patch_margin(SIDE_TOP, margin);
					np->set_patch_margin(SIDE_RIGHT, margin);
					np->set_patch_margin(SIDE_BOTTOM, margin);
					np->set_texture_filter(tf);
					np->set_modulate(Color(1, 1, 1, op));
					content->add_child(np);
				} else {
					TextureRect *fimg = memnew(TextureRect);
					fimg->set_name("FrameFillImage");
					fimg->set_mouse_filter(MOUSE_FILTER_IGNORE);
					fimg->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
					fimg->set_texture_filter(tf);
					fimg->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
					fimg->set_stretch_mode(sm == "fit" ? TextureRect::STRETCH_KEEP_ASPECT_CENTERED : (sm == "fill" ? TextureRect::STRETCH_KEEP_ASPECT_COVERED : (sm == "tile" ? TextureRect::STRETCH_TILE : TextureRect::STRETCH_SCALE)));
					fimg->set_modulate(Color(1, 1, 1, op));
					fimg->set_texture(ftex);
					content->add_child(fimg);
					r_entry.image_rect = fimg;
				}
			}
		}
	} else if (r_entry.type == "button") {
		Button *btn = memnew(Button);
		btn->set_name("ActionButton");
		btn->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		btn->set_text(String(r_entry.config.get("text", r_entry.name)));
		btn->set_clip_text(true);
		btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		btn->set_mouse_filter(preview_mode ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
		btn->set_focus_mode(preview_mode ? FOCUS_NONE : FOCUS_ALL);
		btn->set_meta("ui_elem_id", r_entry.id);
		btn->set_meta("ui_action", r_entry.action);
		btn->set_meta("focus_order", r_entry.focus_order);
		String ip = String(r_entry.config.get("icon_path", r_entry.config.get("image_path", ""))).strip_edges();
		if (!ip.is_empty()) {
			Ref<Texture2D> itex = ClockGearsWindowStyleManager::load_texture_safe(ip);
			if (itex.is_valid()) {
				btn->set_button_icon(itex);
			}
		}
		btn->set_texture_filter(tf);
		btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_button_pressed).bind(r_entry.action, r_entry.id), CONNECT_DEFERRED);
		btn->connect("mouse_entered", callable_mp(this, &ClockGearsUiDesignLayer::_on_button_hover).bind(r_entry.id));
		btn->connect("focus_entered", callable_mp(this, &ClockGearsUiDesignLayer::_on_button_focus_changed).bind(btn->get_instance_id(), r_entry.id));
		content->add_child(btn);
		r_entry.button = btn;
	} else if (r_entry.type == "image") {
		String sm = String(r_entry.config.get("scale_mode", "fit")).strip_edges().to_lower();
		float op = CLAMP(float(r_entry.config.get("opacity", 1.0)), 0.0f, 1.0f);
		String ip = String(r_entry.config.get("image_path", "")).strip_edges();
		Ref<Texture2D> tex = ip.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(ip);
		if (sm == "nine_slice") {
			int margin = CLAMP(int(r_entry.config.get("nine_slice_margin", 12)), 0, 128);
			NinePatchRect *np = memnew(NinePatchRect);
			np->set_name("Image");
			np->set_mouse_filter(MOUSE_FILTER_IGNORE);
			np->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			np->set_texture_filter(tf);
			np->set_patch_margin(SIDE_LEFT, margin);
			np->set_patch_margin(SIDE_TOP, margin);
			np->set_patch_margin(SIDE_RIGHT, margin);
			np->set_patch_margin(SIDE_BOTTOM, margin);
			np->set_modulate(Color(1, 1, 1, op));
			if (tex.is_valid()) np->set_texture(tex);
			content->add_child(np);
		} else {
			TextureRect *img = memnew(TextureRect);
			img->set_name("Image");
			img->set_mouse_filter(MOUSE_FILTER_IGNORE);
			img->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			img->set_texture_filter(tf);
			if (sm == "stretch") {
				img->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
				img->set_stretch_mode(TextureRect::STRETCH_SCALE);
			} else if (sm == "fill") {
				img->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
				img->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_COVERED);
			} else if (sm == "tile") {
				img->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
				img->set_stretch_mode(TextureRect::STRETCH_TILE);
			} else if (sm == "native") {
				img->set_expand_mode(TextureRect::EXPAND_KEEP_SIZE);
				img->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
			} else {
				img->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
				img->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			}
			img->set_modulate(Color(1, 1, 1, op));
			if (tex.is_valid()) {
				img->set_texture(tex);
			}
			content->add_child(img);
			r_entry.image_rect = img;
		}
	} else if (r_entry.type == "bar") {
		ColorRect *bg_r = memnew(ColorRect);
		bg_r->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bg_r->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		bg_r->set_color(_cg_ui_parse_color(r_entry.config.get("bg_color", "#1A1E2ECC"), Color(0.1f, 0.12f, 0.18f, 0.8f)));
		content->add_child(bg_r);
		r_entry.bar_bg_rect = bg_r;

		Control *clip = memnew(Control);
		clip->set_mouse_filter(MOUSE_FILTER_IGNORE);
		clip->set_clip_contents(true);
		content->add_child(clip);
		r_entry.bar_fill_clip = clip;

		ColorRect *fill_r = memnew(ColorRect);
		fill_r->set_mouse_filter(MOUSE_FILTER_IGNORE);
		fill_r->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		fill_r->set_color(_cg_ui_parse_color(r_entry.config.get("fill_color", "#D93B3BFF"), Color(0.85f, 0.23f, 0.23f, 1.0f)));
		clip->add_child(fill_r);
		r_entry.bar_fill_rect = fill_r;

		Label *lbl = memnew(Label);
		lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
		lbl->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		lbl->add_theme_color_override("font_color", _cg_ui_parse_color(r_entry.config.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1)));
		lbl->add_theme_color_override("font_outline_color", Color(0.05f, 0.05f, 0.08f, 0.95f));
		lbl->add_theme_constant_override("outline_size", 2);
		lbl->set_visible(bool(r_entry.config.get("show_numbers", true)));
		content->add_child(lbl);
		r_entry.bar_label = lbl;
	} else if (r_entry.type == "hearts") {
		Control *hc = memnew(Control);
		hc->set_mouse_filter(MOUSE_FILTER_IGNORE);
		hc->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		hc->set_clip_contents(true);
		content->add_child(hc);
		r_entry.hearts_container = hc;
	} else if (r_entry.type == "currency" || r_entry.type == "item") {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_mouse_filter(MOUSE_FILTER_IGNORE);
		hb->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		hb->add_theme_constant_override("separation", 6);
		content->add_child(hb);

		TextureRect *ic = memnew(TextureRect);
		ic->set_mouse_filter(MOUSE_FILTER_IGNORE);
		ic->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		ic->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		ic->set_texture_filter(tf);
		hb->add_child(ic);
		r_entry.icon_rect = ic;

		Label *lbl = memnew(Label);
		lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
		lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		lbl->add_theme_color_override("font_color", _cg_ui_parse_color(r_entry.config.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1)));
		lbl->add_theme_color_override("font_outline_color", Color(0.05f, 0.05f, 0.08f, 0.95f));
		lbl->add_theme_constant_override("outline_size", 2);
		hb->add_child(lbl);
		r_entry.label = lbl;
	} else if (r_entry.type == "widget") {
		if (preview_mode) {
			PanelContainer *card = memnew(PanelContainer);
			card->set_mouse_filter(MOUSE_FILTER_IGNORE);
			card->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			ClockGearsWindowStyleManager::apply_window_style(card, window_type_fallback);
			card->set_modulate(Color(1, 1, 1, 0.92f));
			content->add_child(card);

			VBoxContainer *vb = memnew(VBoxContainer);
			vb->set_mouse_filter(MOUSE_FILTER_IGNORE);
			vb->add_theme_constant_override("separation", 4);
			card->add_child(vb);

			Label *hdr = memnew(Label);
			hdr->set_mouse_filter(MOUSE_FILTER_IGNORE);
			hdr->set_text(r_entry.name.is_empty() ? String("Panel") : r_entry.name);
			hdr->add_theme_font_size_override("font_size", 13);
			hdr->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.55f, 1.0f));
			vb->add_child(hdr);

			String wk = r_entry.widget_kind;
			String hint = String::utf8("Contenido dinámico");
			if (wk == "party_list") {
				hint = String::utf8("• Héroe  Nv.1  PV 100/100  PM 30/30");
			} else if (wk == "items_target" || wk == "equip_target" || wk == "status_target") {
				hdr->set_visible(false);
				hint = String::utf8("Objetivo: <  Héroe  >");
			} else if (wk == "items_list") {
				hint = String::utf8("> Poción menor  (x3)\n  Éter simple  (x2)\n  Llave oxidada  (x1)");
			} else if (wk == "item_preview" || wk == "equip_preview") {
				hint = String::utf8("[ Vista previa 64×64 ]");
			} else if (wk == "item_details" || wk == "equip_desc") {
				hint = String::utf8("Nombre del objeto\nCantidad: 3 / 99\nDescripción del objeto…");
			} else if (wk == "equip_slots") {
				hint = String::utf8("> Arma: Espada corta\n  Armadura: Cota de cuero\n  Accesorio: ---");
			} else if (wk == "equip_candidates") {
				hint = String::utf8("> Espada larga (x1)\n  Daga de latón (x2)");
			} else if (wk == "equip_params") {
				hint = String::utf8("ATQ: 18 -> 24  (+6)\nDEF: 12 -> 12  (=)");
			} else if (wk == "status_basic") {
				hint = String::utf8("[Retrato]  Héroe · Nv. 1\nPV 100/100 · PM 30/30 · EXP 0");
			} else if (wk == "status_equips") {
				hint = String::utf8("Arma: Espada corta\nArmadura: Cota de cuero");
			} else if (wk == "status_params") {
				hint = String::utf8("ATQ 18 · DEF 12 · VEL 14\nAfinidades elementales");
			} else if (wk == "save_slots") {
				hint = String::utf8("Ranura 1 — Mapa 001  00:12:40\nRanura 2 — (Vacía)\nRanura 3 — (Vacía)");
			} else if (wk == "options_controls") {
				hint = String::utf8("Música  [========--] 80%\nEfectos [========--] 80%\nBrillo  [==========] 100%\n[ ] Pantalla completa");
			}
			Label *body = memnew(Label);
			body->set_mouse_filter(MOUSE_FILTER_IGNORE);
			body->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
			body->add_theme_font_size_override("font_size", 12);
			body->add_theme_color_override("font_color", Color(0.84f, 0.88f, 0.95f, 0.92f));
			body->set_text(hint);
			vb->add_child(body);
		}
	} else if (r_entry.type == "radial") {
		PanelContainer *card = memnew(PanelContainer);
		card->set_mouse_filter(MOUSE_FILTER_IGNORE);
		card->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		ClockGearsWindowStyleManager::apply_window_style(card, window_type_fallback);
		content->add_child(card);

		Label *rlbl = memnew(Label);
		rlbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
		rlbl->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		rlbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		rlbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		rlbl->set_text(String::utf8("◎ Rueda radial"));
		rlbl->add_theme_color_override("font_color", Color(0.96f, 0.80f, 0.40f, 1.0f));
		card->add_child(rlbl);
		r_entry.label = rlbl;
	} else {
		Label *lbl = memnew(Label);
		lbl->set_name("TextLabel");
		lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
		lbl->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		String al = String(r_entry.config.get("align", "left"));
		lbl->set_horizontal_alignment((al == "center") ? HORIZONTAL_ALIGNMENT_CENTER : ((al == "right") ? HORIZONTAL_ALIGNMENT_RIGHT : HORIZONTAL_ALIGNMENT_LEFT));
		lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		lbl->add_theme_color_override("font_color", _cg_ui_parse_color(r_entry.config.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1)));
		lbl->add_theme_color_override("font_outline_color", Color(0.05f, 0.05f, 0.08f, 0.95f));
		lbl->add_theme_constant_override("outline_size", 2);
		content->add_child(lbl);
		r_entry.label = lbl;
	}
}

void ClockGearsUiDesignLayer::_apply_element_style(ElementEntry &r_entry, float p_scale) {
	if (!r_entry.bg_panel || !r_entry.content_root || !r_entry.root) {
		return;
	}
	const bool has_shape = r_entry.config.has("shape") && r_entry.config["shape"].get_type() == Variant::DICTIONARY;
	Dictionary shape_cfg = has_shape ? Dictionary(r_entry.config["shape"]) : Dictionary();

	if (r_entry.clip_host && has_shape) {
		r_entry.bg_panel->set_visible(false);
		r_entry.clip_host->set_position(Vector2());
		r_entry.clip_host->set_size(r_entry.root->get_size());
		bool draw_fill = (r_entry.type == "frame" || r_entry.type == "button" || !r_entry.window_style_id.is_empty());
		if (r_entry.type == "image") {
			draw_fill = false; // Para imagen recortada, solo la máscara recorta (y mantenemos el alpha del sprite si es transparente o dibujamos su contenido encima)
		}
		r_entry.clip_host->configure(shape_cfg, p_scale, draw_fill);
		if (r_entry.border_overlay) {
			r_entry.border_overlay->set_position(Vector2());
			r_entry.border_overlay->set_size(r_entry.root->get_size());
			r_entry.border_overlay->configure(shape_cfg, p_scale);
		}
		r_entry.content_root->set_position(Vector2());
		r_entry.content_root->set_size(r_entry.root->get_size());
		if (r_entry.button) {
			Ref<StyleBoxEmpty> empty_sb;
			empty_sb.instantiate();
			r_entry.button->add_theme_style_override("normal", empty_sb);
			r_entry.button->add_theme_style_override("hover", empty_sb);
			r_entry.button->add_theme_style_override("pressed", empty_sb);
			r_entry.button->add_theme_style_override("hover_pressed", empty_sb);
			r_entry.button->add_theme_style_override("focus", empty_sb);
			r_entry.button->add_theme_style_override("disabled", empty_sb);
			int fsz = MAX(8, int(Math::round(float(r_entry.config.get("font_size", 16)) * p_scale)));
			r_entry.button->add_theme_font_size_override("font_size", fsz);
		}
		return;
	}

	if (r_entry.type == "button" && r_entry.button) {
		Dictionary b_over;
		if (!r_entry.window_style_id.is_empty() && r_entry.window_style_id != "none") {
			if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
				b_over = dm->get_window_style(r_entry.window_style_id);
			}
		}
		if (has_shape) {
			Array keys = shape_cfg.keys();
			for (int k = 0; k < keys.size(); k++) {
				b_over[keys[k]] = shape_cfg[keys[k]];
			}
		}
		ClockGearsWindowStyleManager::apply_button_style(r_entry.button, window_type_fallback, b_over);
		String img_norm = String(r_entry.config.get("image_normal", "")).strip_edges();
		String img_hov = String(r_entry.config.get("image_hover", "")).strip_edges();
		String img_prs = String(r_entry.config.get("image_pressed", "")).strip_edges();
		String img_dis = String(r_entry.config.get("image_disabled", "")).strip_edges();
		if (!img_norm.is_empty() || !img_hov.is_empty() || !img_prs.is_empty() || !img_dis.is_empty()) {
			auto apply_btn_tex_state = [&](const char *p_state, const String &p_path, const String &p_fb) {
				String use_p = p_path.is_empty() ? p_fb : p_path;
				if (use_p.is_empty()) return;
				Ref<Texture2D> t = ClockGearsWindowStyleManager::load_texture_safe(use_p);
				if (t.is_valid()) {
					Ref<StyleBoxTexture> sbt;
					sbt.instantiate();
					sbt->set_texture(t);
					sbt->set_content_margin_all(4.0f * p_scale);
					r_entry.button->add_theme_style_override(p_state, sbt);
				}
			};
			apply_btn_tex_state("normal", img_norm, img_hov);
			apply_btn_tex_state("hover", img_hov, img_norm);
			apply_btn_tex_state("pressed", img_prs, img_hov.is_empty() ? img_norm : img_hov);
			apply_btn_tex_state("disabled", img_dis, img_norm);
		}
		int fsz = MAX(8, int(Math::round(float(r_entry.config.get("font_size", 16)) * p_scale)));
		r_entry.button->add_theme_font_size_override("font_size", fsz);
		r_entry.bg_panel->set_visible(false);
		r_entry.content_root->set_position(Vector2());
		r_entry.content_root->set_size(r_entry.root->get_size());
		return;
	}

	String sid = r_entry.window_style_id;
	bool force_panel = (r_entry.type == "frame") || has_shape;
	if (!force_panel && (sid.is_empty() || sid == "none")) {
		if (r_entry.bg_panel->is_visible()) {
			r_entry.bg_panel->set_visible(false);
		}
		if (r_entry.content_root->get_position() != Vector2()) {
			r_entry.content_root->set_position(Vector2());
		}
		Size2 rsz = r_entry.root->get_size();
		if (r_entry.content_root->get_size() != rsz) {
			r_entry.content_root->set_size(rsz);
		}
		if (r_entry.image_rect && String(r_entry.config.get("scale_mode", "")).strip_edges().to_lower() == "integer") {
			Ref<Texture2D> it = r_entry.image_rect->get_texture();
			if (it.is_valid()) {
				Vector2 ts = it->get_size();
				if (ts.x > 0.0f && ts.y > 0.0f) {
					int mult = MAX(1, int(Math::floor(MIN(rsz.x / ts.x, rsz.y / ts.y))));
					Vector2 dsz = ts * float(mult);
					r_entry.image_rect->set_anchors_preset(PRESET_TOP_LEFT);
					r_entry.image_rect->set_position(((rsz - dsz) * 0.5f).floor());
					r_entry.image_rect->set_size(dsz);
				}
			}
		}
		return;
	}

	r_entry.bg_panel->set_visible(true);
	Dictionary surface_overrides;
	if (!sid.is_empty() && sid != "none" && sid != "default") {
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Dictionary specific = dm->get_window_style(sid);
			if (!specific.is_empty()) {
				surface_overrides = specific;
			}
		}
	}
	if (has_shape) {
		Array keys = shape_cfg.keys();
		for (int k = 0; k < keys.size(); k++) {
			surface_overrides[keys[k]] = shape_cfg[keys[k]];
		}
	}
	ClockGearsWindowStyleManager::apply_window_style(r_entry.bg_panel, window_type_fallback, surface_overrides);
	Ref<StyleBox> sb = r_entry.bg_panel->get_theme_stylebox("panel");
	float pad_l = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_LEFT)) : (4.0f * p_scale);
	float pad_t = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_TOP)) : (4.0f * p_scale);
	float pad_r = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_RIGHT)) : (4.0f * p_scale);
	float pad_b = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_BOTTOM)) : (4.0f * p_scale);
	Size2 rsz = r_entry.root->get_size();
	r_entry.content_root->set_position(Vector2(pad_l, pad_t));
	Size2 csz(MAX(4.0f, rsz.x - pad_l - pad_r), MAX(4.0f, rsz.y - pad_t - pad_b));
	r_entry.content_root->set_size(csz);
	if (r_entry.image_rect && String(r_entry.config.get("scale_mode", "")).strip_edges().to_lower() == "integer") {
		Ref<Texture2D> it = r_entry.image_rect->get_texture();
		if (it.is_valid()) {
			Vector2 ts = it->get_size();
			if (ts.x > 0.0f && ts.y > 0.0f) {
				int mult = MAX(1, int(Math::floor(MIN(csz.x / ts.x, csz.y / ts.y))));
				Vector2 dsz = ts * float(mult);
				r_entry.image_rect->set_anchors_preset(PRESET_TOP_LEFT);
				r_entry.image_rect->set_position(((csz - dsz) * 0.5f).floor());
				r_entry.image_rect->set_size(dsz);
			}
		}
	}
}

void ClockGearsUiDesignLayer::_apply_container_layout(const Dictionary &p_layout, float p_scale) {
	// Limpiar botones auxiliares previos de paginación/pestañas/carrusel antes de reconstruir si cambian
	for (int i = 0; i < layout_aux_buttons.size(); i++) {
		Button *b = layout_aux_buttons[i];
		if (b) {
			if (b->get_parent() == this) remove_child(b);
			memdelete(b);
		}
	}
	layout_aux_buttons.clear();

	String mode = String(p_layout.get("mode", "free")).strip_edges().to_lower();
	if (mode == "free") {
		return;
	}
	if (mode == "radial") {
		if (radial_canvas) {
			radial_canvas->set_position(current_stage_rect.position);
			radial_canvas->set_size(current_stage_rect.size);
			radial_canvas->configure(p_layout, active_design.get("elements", Array()), p_scale, !preview_mode);
		}
		return;
	}

	Dictionary area_desc;
	area_desc["anchor"] = p_layout.get("origin_anchor", "center");
	area_desc["offset_x"] = p_layout.get("offset_x", 0);
	area_desc["offset_y"] = p_layout.get("offset_y", 0);
	area_desc["width"] = p_layout.get("width", 420);
	area_desc["height"] = p_layout.get("height", 320);
	Rect2 box = compute_element_rect(area_desc, current_stage_rect, p_scale);

	float spx = float(int(p_layout.get("spacing_x", 10))) * p_scale;
	float spy = float(int(p_layout.get("spacing_y", 10))) * p_scale;

	Vector<int> item_indices;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].type == "button" || mode == "tabs") {
			item_indices.push_back(i);
		}
	}
	if (item_indices.is_empty()) {
		return;
	}

	if (mode == "vertical_list" || mode == "horizontal_list" || mode == "grid") {
		int per_page = MAX(0, int(p_layout.get("page_size", 0)));
		int cols = (mode == "vertical_list") ? 1 : ((mode == "horizontal_list") ? MAX(1, item_indices.size()) : MAX(1, int(p_layout.get("columns", 3))));
		int rows = MAX(0, int(p_layout.get("rows", 0)));
		if (mode == "grid" && per_page <= 0 && rows > 0) {
			per_page = cols * rows;
		}
		int total_items = item_indices.size();
		int start_i = 0;
		int end_i = total_items;
		int total_pages = 1;
		float footer_h = 0.0f;
		if (per_page > 0 && total_items > per_page) {
			total_pages = MAX(1, int(Math::ceil(float(total_items) / float(per_page))));
			current_layout_page = CLAMP(current_layout_page, 0, total_pages - 1);
			start_i = current_layout_page * per_page;
			end_i = MIN(total_items, start_i + per_page);
			footer_h = 34.0f * p_scale;
		}
		Rect2 avail = Rect2(box.position, Size2(box.size.x, MAX(24.0f, box.size.y - footer_h)));
		int vis_cnt = MAX(1, end_i - start_i);
		int used_rows = MAX(1, int(Math::ceil(float(vis_cnt) / float(cols))));
		float cell_w = (avail.size.x - spx * float(cols - 1)) / float(cols);
		float cell_h = (mode == "horizontal_list") ? avail.size.y : ((avail.size.y - spy * float(used_rows - 1)) / float(used_rows));

		for (int k = 0; k < total_items; k++) {
			ElementEntry &e = elements.write[item_indices[k]];
			if (!e.root) continue;
			if (k < start_i || k >= end_i) {
				e.root->set_visible(false);
				continue;
			}
			int local_k = k - start_i;
			int col = local_k % cols;
			int row = local_k / cols;
			Rect2 cr(avail.position + Vector2(float(col) * (cell_w + spx), float(row) * (cell_h + spy)), Size2(MAX(16.0f, cell_w), MAX(16.0f, cell_h)));
			e.root->set_visible(true);
			e.root->set_position(cr.position);
			e.root->set_size(cr.size);
			_apply_element_style(e, p_scale);
		}

		if (total_pages > 1) {
			float bw = 76.0f * p_scale;
			float bh = 28.0f * p_scale;
			float by = box.position.y + box.size.y - bh;
			Button *prev_b = memnew(Button);
			prev_b->set_text("< Ant.");
			prev_b->set_position(Vector2(box.position.x, by));
			prev_b->set_size(Size2(bw, bh));
			prev_b->set_disabled(current_layout_page <= 0);
			prev_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_layout_page_delta).bind(-1));
			ClockGearsWindowStyleManager::apply_button_style(prev_b, window_type_fallback);
			add_child(prev_b);
			layout_aux_buttons.push_back(prev_b);

			Button *next_b = memnew(Button);
			next_b->set_text("Sig. >");
			next_b->set_position(Vector2(box.position.x + box.size.x - bw, by));
			next_b->set_size(Size2(bw, bh));
			next_b->set_disabled(current_layout_page >= total_pages - 1);
			next_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_layout_page_delta).bind(1));
			ClockGearsWindowStyleManager::apply_button_style(next_b, window_type_fallback);
			add_child(next_b);
			layout_aux_buttons.push_back(next_b);
		}
	} else if (mode == "tabs") {
		Array labels = p_layout.get("tab_labels", Array());
		int max_tab = MAX(0, int(labels.size()) - 1);
		for (int i = 0; i < elements.size(); i++) {
			max_tab = MAX(max_tab, elements[i].tab_index);
		}
		int ntabs = MAX(1, max_tab + 1);
		current_layout_tab = CLAMP(current_layout_tab, 0, ntabs - 1);
		float th = 32.0f * p_scale;
		float tw = (box.size.x - spx * float(ntabs - 1)) / float(ntabs);
		for (int t = 0; t < ntabs; t++) {
			Button *tb = memnew(Button);
			String tname = (t < labels.size()) ? String(labels[t]) : vformat(String::utf8("Pestaña %d"), t + 1);
			tb->set_text(tname);
			tb->set_clip_text(true);
			tb->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			tb->set_toggle_mode(true);
			tb->set_pressed_no_signal(t == current_layout_tab);
			tb->set_position(box.position + Vector2(float(t) * (tw + spx), 0.0f));
			tb->set_size(Size2(MAX(36.0f, tw), th));
			tb->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_layout_tab_selected).bind(t));
			ClockGearsWindowStyleManager::apply_button_style(tb, window_type_fallback);
			add_child(tb);
			layout_aux_buttons.push_back(tb);
		}
		for (int i = 0; i < elements.size(); i++) {
			ElementEntry &e = elements.write[i];
			if (e.root) {
				e.root->set_visible(e.visible && e.tab_index == current_layout_tab);
			}
		}
	} else if (mode == "carousel") {
		int cnt = item_indices.size();
		current_carousel_index = CLAMP(current_carousel_index, 0, cnt - 1);
		float card_w = box.size.x * 0.44f;
		float card_h = box.size.y * 0.68f;
		Vector2 mid_c = box.position + box.size * 0.5f;
		for (int k = 0; k < cnt; k++) {
			ElementEntry &e = elements.write[item_indices[k]];
			if (!e.root) continue;
			int rel = k - current_carousel_index;
			if (Math::abs(rel) > 1) {
				e.root->set_visible(false);
				continue;
			}
			float sc = (rel == 0) ? 1.0f : 0.78f;
			Size2 csz(card_w * sc, card_h * sc);
			Vector2 cpos(mid_c.x + float(rel) * (card_w * 0.85f) - csz.x * 0.5f, mid_c.y - csz.y * 0.5f);
			e.root->set_visible(true);
			e.root->set_position(cpos);
			e.root->set_size(csz);
			e.root->set_modulate(Color(1, 1, 1, (rel == 0) ? 1.0f : 0.72f));
			e.root->set_z_index((rel == 0) ? 10 : 5);
			_apply_element_style(e, p_scale);
		}
		if (cnt > 1) {
			float bh = 32.0f * p_scale;
			float bw = 36.0f * p_scale;
			Button *bl = memnew(Button);
			bl->set_text("<");
			bl->set_position(Vector2(box.position.x, mid_c.y - bh * 0.5f));
			bl->set_size(Size2(bw, bh));
			bl->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_carousel_step).bind(-1));
			ClockGearsWindowStyleManager::apply_button_style(bl, window_type_fallback);
			add_child(bl);
			layout_aux_buttons.push_back(bl);

			Button *br = memnew(Button);
			br->set_text(">");
			br->set_position(Vector2(box.position.x + box.size.x - bw, mid_c.y - bh * 0.5f));
			br->set_size(Size2(bw, bh));
			br->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_carousel_step).bind(1));
			ClockGearsWindowStyleManager::apply_button_style(br, window_type_fallback);
			add_child(br);
			layout_aux_buttons.push_back(br);
		}
	}
}

void ClockGearsUiDesignLayer::_relayout_elements(bool p_refresh_styles) {
	Size2 vp_sz = _resolve_effective_viewport_size();
	if (!p_refresh_styles && vp_sz == last_layout_size) {
		return;
	}
	last_layout_size = vp_sz;
	current_stage_rect = compute_stage_rect(vp_sz);
	current_ui_scale = MAX(0.25f, MIN(vp_sz.x / LOGICAL_WIDTH, vp_sz.y / LOGICAL_HEIGHT));
	_apply_screen_background(active_design, vp_sz);

	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) {
			continue;
		}
		Rect2 er = compute_element_rect(e.config, current_stage_rect, current_ui_scale);
		if (e.root->get_position() != er.position) {
			e.root->set_position(er.position);
		}
		if (e.root->get_size() != er.size) {
			e.root->set_size(er.size);
		}
		if (e.root->get_z_index() != e.layer_order) {
			e.root->set_z_index(e.layer_order);
		}
		_apply_element_style(e, current_ui_scale);

		String fp = String(e.config.get("font_path", "")).strip_edges();
		Ref<Font> cfont = !fp.is_empty() ? _load_cached_font(fp) : Ref<Font>();

		if (e.label) {
			int fsz = MAX(8, int(Math::round(float(e.config.get("font_size", 16)) * current_ui_scale)));
			if (!e.label->has_theme_font_size_override("font_size") || e.label->get_theme_font_size("font_size") != fsz) {
				e.label->add_theme_font_size_override("font_size", fsz);
			}
			if (cfont.is_valid()) e.label->add_theme_font_override("font", cfont);
		}
		if (e.button) {
			if (cfont.is_valid()) e.button->add_theme_font_override("font", cfont);
			if (e.config.has("font_color")) {
				Color fc = _cg_ui_parse_color(e.config["font_color"], Color(1, 1, 1, 1));
				e.button->add_theme_color_override("font_color", fc);
				e.button->add_theme_color_override("font_hover_color", fc);
				e.button->add_theme_color_override("font_focus_color", fc);
			}
		}
		if (e.bar_label) {
			int fsz = MAX(8, int(Math::round(float(e.config.get("font_size", 13)) * current_ui_scale)));
			e.bar_label->add_theme_font_size_override("font_size", fsz);
			if (cfont.is_valid()) e.bar_label->add_theme_font_override("font", cfont);
		}
		if (e.icon_rect) {
			Size2 csz = e.content_root ? e.content_root->get_size() : er.size;
			float icon_sz = CLAMP(csz.y - 4.0f, 12.0f, 40.0f);
			e.icon_rect->set_custom_minimum_size(Size2(icon_sz, icon_sz));
		}
	}

	Dictionary lay = active_design.get("layout", Dictionary());
	_apply_container_layout(lay, current_ui_scale);

	if (cursor_overlay) {
		cursor_overlay->configure(active_design.get("cursor", Dictionary()), current_ui_scale);
	}
}

bool ClockGearsUiDesignLayer::update_element_geometry_fast(const String &p_elem_id, const Dictionary &p_elem) {
	String target = p_elem_id.strip_edges().to_lower();
	if (target.is_empty() || p_elem.is_empty()) {
		return false;
	}
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id.to_lower() == target) {
			ElementEntry &e = elements.write[i];
			bool need_style = (int(e.config.get("corner_radius", 8)) != int(p_elem.get("corner_radius", 8)) ||
					String(e.config.get("shape", "rect")) != String(p_elem.get("shape", "rect")) ||
					String(e.config.get("window_style_id", "")) != String(p_elem.get("window_style_id", "")) ||
					String(e.config.get("fill_color", "")) != String(p_elem.get("fill_color", "")) ||
					int(e.config.get("border_width", 2)) != int(p_elem.get("border_width", 2)));
			e.config = p_elem.duplicate(true);
			e.anchor = String(p_elem.get("anchor", e.anchor));
			e.offset_x = int(p_elem.get("offset_x", e.offset_x));
			e.offset_y = int(p_elem.get("offset_y", e.offset_y));
			e.width = CLAMP(int(p_elem.get("width", e.width)), 16, 960);
			e.height = CLAMP(int(p_elem.get("height", e.height)), 16, 540);
			e.layer_order = int(p_elem.get("layer_order", e.layer_order));
			e.visible = bool(p_elem.get("visible", e.visible));
			if (e.root) {
				Rect2 er = compute_element_rect(e.config, current_stage_rect, current_ui_scale);
				e.root->set_position(er.position);
				e.root->set_size(er.size);
				e.root->set_z_index(e.layer_order);
				e.root->set_visible(e.visible);
				if (need_style) {
					_apply_element_style(e, current_ui_scale);
				}
			}
			return true;
		}
	}
	return false;
}

void ClockGearsUiDesignLayer::refresh_live_values() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (radial_canvas) {
		radial_canvas->refresh_slot_states(dm);
	}
	Dictionary lay = active_design.get("layout", Dictionary());
	String lmode = String(lay.get("mode", "free")).strip_edges().to_lower();

	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) {
			continue;
		}
		bool vis = e.visible;
		if (vis && lmode == "tabs" && e.tab_index != current_layout_tab) {
			vis = false;
		}
		if (vis && dm && String(e.config.get("visibility_mode", "always")) == "switch") {
			String sw = String(e.config.get("visibility_switch", "")).strip_edges();
			if (!sw.is_empty()) {
				bool sw_val = dm->get_switch(sw);
				if (bool(e.config.get("visibility_invert", false))) {
					sw_val = !sw_val;
				}
				if (!sw_val) {
					vis = false;
				}
			}
		}

		if (e.type == "text" || e.type == "variable") {
			if (e.label) {
				String nt = resolve_element_text(e.config, dm);
				if (e.label->get_text() != nt) {
					e.label->set_text(nt);
				}
			}
		} else if (e.type == "button") {
			if (e.button) {
				String btxt = (e.data_binding != "none" && !e.data_binding.is_empty())
						? resolve_element_text(e.config, dm)
						: String(e.config.get("text", e.name));
				if (e.button->get_text() != btxt) {
					e.button->set_text(btxt);
				}
				String emode = String(e.config.get("enabled_mode", "always")).strip_edges().to_lower();
				bool is_dis = (emode == "disabled") || bool(e.config.get("disabled", false)) ||
						(!e.action.is_empty() && bool(disabled_actions.get(e.action, false)));
				if (!is_dis && emode == "switch") {
					String esw = String(e.config.get("enabled_switch", "")).strip_edges();
					bool sw_on = (dm && !esw.is_empty()) ? dm->get_switch(esw) : false;
					if (bool(e.config.get("enabled_invert", false))) {
						sw_on = !sw_on;
					}
					is_dis = !sw_on;
				}
				e.button->set_disabled(is_dis);
				e.button->set_focus_mode((preview_mode || is_dis) ? FOCUS_NONE : FOCUS_ALL);
			}
		} else if (e.type == "currency") {
			int gold = dm ? dm->get_gold() : 0;
			String unit = dm ? dm->get_currency_unit() : String("G");
			bool show_name = bool(e.config.get("show_currency_name", true));
			if (e.label) {
				e.label->set_text(show_name ? vformat("%d %s", gold, unit) : itos(gold));
			}
			if (e.icon_rect) {
				bool show_ic = bool(e.config.get("show_icon", true));
				e.icon_rect->set_visible(show_ic);
				if (show_ic) {
					String ip = String(e.config.get("icon_path", "")).strip_edges();
					Ref<Texture2D> ic_tex = !ip.is_empty() ? ClockGearsWindowStyleManager::load_texture_safe(ip) : Ref<Texture2D>();
					if (ic_tex.is_null()) {
						ic_tex = make_builtin_coin_texture();
					}
					e.icon_rect->set_texture(ic_tex);
				}
			}
		} else if (e.type == "item") {
			String ik = String(e.config.get("item_key", "")).strip_edges();
			int qty = (dm && !ik.is_empty()) ? dm->get_item_qty(ik) : 0;
			if (bool(e.config.get("hide_when_zero", false)) && qty <= 0) {
				vis = false;
			}
			if (e.label) {
				if (bool(e.config.get("show_name", false)) && dm && !ik.is_empty()) {
					Dictionary rec = dm->find_inventory_record(ik);
					String iname = String(rec.get("name", ik));
					e.label->set_text(vformat("%s x%d", iname, qty));
				} else {
					e.label->set_text(vformat("x%d", qty));
				}
			}
			if (e.icon_rect) {
				String ip = String(e.config.get("icon_path", "")).strip_edges();
				if (ip.is_empty() && dm && !ik.is_empty()) {
					ip = String(dm->find_inventory_record(ik).get("icon_path", "")).strip_edges();
				}
				Ref<Texture2D> ic_tex = !ip.is_empty() ? ClockGearsWindowStyleManager::load_texture_safe(ip) : Ref<Texture2D>();
				e.icon_rect->set_visible(ic_tex.is_valid());
				e.icon_rect->set_texture(ic_tex);
			}
		} else if (e.type == "bar") {
			String vs = String(e.config.get("value_source", "hp"));
			int64_t cur = 0;
			int64_t mx = 100;
			if (dm) {
				int aid = _cg_ui_get_leader_actor_id(dm, int(e.config.get("actor_id", 0)));
				if (vs == "variable") {
					cur = dm->get_variable(String(e.config.get("variable_name", "")));
					mx = MAX(1, int(e.config.get("variable_max", 100)));
				} else if (vs == "mp" && aid > 0) {
					cur = dm->get_actor_runtime_mp(aid);
					mx = MAX(1, dm->get_actor_runtime_max_mp(aid));
				} else if (vs == "exp" && aid > 0) {
					cur = dm->get_actor_runtime_exp(aid);
					mx = 100;
				} else if (aid > 0) {
					cur = dm->get_actor_runtime_hp(aid);
					mx = MAX(1, dm->get_actor_runtime_max_hp(aid));
				}
			}
			float ratio = CLAMP(float(cur) / float(MAX(int64_t(1), mx)), 0.0f, 1.0f);
			if (e.bar_fill_clip && e.content_root) {
				Size2 csz = e.content_root->get_size();
				e.bar_fill_clip->set_position(Vector2(0, 0));
				e.bar_fill_clip->set_size(Vector2(csz.x * ratio, csz.y));
			}
			if (e.bar_label) {
				String pfx = String(e.config.get("label_text", "")).strip_edges();
				String nums = vformat("%s/%s", String::num_int64(cur), String::num_int64(mx));
				e.bar_label->set_text(pfx.is_empty() ? nums : (pfx + " " + nums));
			}
		} else if (e.type == "hearts" && e.hearts_container) {
			int hp = 100;
			int mhp = 100;
			if (dm) {
				int aid = _cg_ui_get_leader_actor_id(dm, int(e.config.get("actor_id", 0)));
				if (aid > 0) {
					hp = dm->get_actor_runtime_hp(aid);
					mhp = dm->get_actor_runtime_max_hp(aid);
				}
			}
			int per_heart = MAX(1, int(e.config.get("hp_per_heart", 20)));
			int total_hearts = MAX(1, int(Math::ceil(float(MAX(1, mhp)) / float(per_heart))));
			int max_per_row = CLAMP(int(e.config.get("max_hearts_per_row", 10)), 1, 50);
			int max_rows = CLAMP(int(e.config.get("max_rows", 2)), 1, 20);
			total_hearts = MIN(total_hearts, max_per_row * max_rows);

			int key = hp * 10007 + total_hearts * 31 + int(Math::round(current_ui_scale * 100.0f));
			if (e.last_hearts_key != key) {
				e.last_hearts_key = key;
				for (int c = e.hearts_container->get_child_count() - 1; c >= 0; c--) {
					Node *ch = e.hearts_container->get_child(c);
					e.hearts_container->remove_child(ch);
					memdelete(ch);
				}
				Ref<Texture2D> full_t = ClockGearsWindowStyleManager::load_texture_safe(String(e.config.get("full_image_path", "")));
				if (full_t.is_null()) full_t = make_builtin_heart_texture(2);
				Ref<Texture2D> half_t = ClockGearsWindowStyleManager::load_texture_safe(String(e.config.get("half_image_path", "")));
				if (half_t.is_null()) half_t = make_builtin_heart_texture(1);
				Ref<Texture2D> empty_t = ClockGearsWindowStyleManager::load_texture_safe(String(e.config.get("empty_image_path", "")));
				if (empty_t.is_null()) empty_t = make_builtin_heart_texture(0);

				float hsz = float(int(e.config.get("heart_size", 20))) * current_ui_scale;
				float spc = float(int(e.config.get("spacing", 4))) * current_ui_scale;
				for (int hi = 0; hi < total_hearts; hi++) {
					int h_min = hi * per_heart;
					int rem = hp - h_min;
					Ref<Texture2D> use_t = (rem >= per_heart) ? full_t : ((rem >= (per_heart / 2) && rem > 0) ? half_t : empty_t);
					TextureRect *hr = memnew(TextureRect);
					hr->set_mouse_filter(MOUSE_FILTER_IGNORE);
					hr->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
					hr->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
					hr->set_texture(use_t);
					int row = hi / max_per_row;
					int col = hi % max_per_row;
					hr->set_position(Vector2(col * (hsz + spc), row * (hsz + spc)));
					hr->set_size(Size2(hsz, hsz));
					e.hearts_container->add_child(hr);
				}
			}
		}

		e.root->set_visible(vis);
	}
}

void ClockGearsUiDesignLayer::_on_button_pressed(const String &p_action, const String &p_elem_id) {
	if (preview_mode) {
		return;
	}
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_elem_id) {
			String sc = String(elements[i].config.get("sound_click", "")).strip_edges();
			if (!sc.is_empty()) {
				if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
					am->play_se(sc, 1.0f, 1.0f);
				}
			}
			break;
		}
	}
	emit_signal("action_triggered", p_action, p_elem_id);
}

void ClockGearsUiDesignLayer::_on_button_hover(const String &p_elem_id) {
	if (preview_mode) {
		return;
	}
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_elem_id) {
			String sh = String(elements[i].config.get("sound_hover", "")).strip_edges();
			if (!sh.is_empty()) {
				if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
					am->play_se(sh, 1.0f, 1.0f);
				}
			}
			break;
		}
	}
}

void ClockGearsUiDesignLayer::_on_button_focus_changed(ObjectID p_btn_id, const String &p_elem_id) {
	Button *b = Object::cast_to<Button>(ObjectDB::get_instance(p_btn_id));
	update_cursor_for_button(b);
	_on_button_hover(p_elem_id);
}

void ClockGearsUiDesignLayer::_on_radial_slot_activated(int p_slot_idx) {
	if (!radial_canvas || preview_mode) {
		return;
	}
	Dictionary info = radial_canvas->get_selected_slot_info();
	String act = String(info.get("action", "")).strip_edges().to_lower();
	String sid = String(info.get("id", ""));
	if (act.is_empty()) {
		Array acts = info.get("actions", Array());
		if (!acts.is_empty() && acts[0].get_type() == Variant::DICTIONARY) {
			act = String(Dictionary(acts[0]).get("type", "")).strip_edges().to_lower();
		}
	}
	emit_signal("action_triggered", act, sid);
}

void ClockGearsUiDesignLayer::_on_radial_selection_changed(int p_slot_idx) {
	emit_signal("radial_selection_changed", p_slot_idx);
}

void ClockGearsUiDesignLayer::_on_layout_page_delta(int p_delta) {
	current_layout_page = MAX(0, current_layout_page + p_delta);
	last_layout_size = Vector2(-1, -1);
	_relayout_elements(true);
	refresh_live_values();
}

void ClockGearsUiDesignLayer::_on_layout_tab_selected(int p_tab_idx) {
	current_layout_tab = MAX(0, p_tab_idx);
	last_layout_size = Vector2(-1, -1);
	_relayout_elements(true);
	refresh_live_values();
}

void ClockGearsUiDesignLayer::_on_carousel_step(int p_delta) {
	int cnt = 0;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].type == "button") cnt++;
	}
	if (cnt <= 0) return;
	current_carousel_index = ((current_carousel_index + p_delta) % cnt + cnt) % cnt;
	last_layout_size = Vector2(-1, -1);
	_relayout_elements(true);
	refresh_live_values();
}

void ClockGearsUiDesignLayer::update_cursor_for_button(Button *p_button) {
	if (!cursor_overlay) {
		return;
	}
	if (!p_button || !p_button->is_visible_in_tree()) {
		cursor_overlay->set_target_rect(Rect2(), false);
		return;
	}
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].button == p_button && elements[i].root) {
			cursor_overlay->set_target_rect(Rect2(elements[i].root->get_position(), elements[i].root->get_size()), true);
			return;
		}
	}
	cursor_overlay->set_target_rect(Rect2(p_button->get_position(), p_button->get_size()), true);
}

Vector<Button *> ClockGearsUiDesignLayer::get_focus_ordered_buttons() const {
	Vector<const ElementEntry *> sorted;
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.type == "button" && e.button && e.root && e.root->is_visible() && !e.button->is_disabled()) {
			sorted.push_back(&e);
		}
	}
	struct FocusCmp {
		bool operator()(const ElementEntry *a, const ElementEntry *b) const {
			if (a->focus_order != b->focus_order) {
				return a->focus_order < b->focus_order;
			}
			if (a->layer_order != b->layer_order) {
				return a->layer_order < b->layer_order;
			}
			return a->array_order < b->array_order;
		}
	};
	sorted.sort_custom<FocusCmp>();

	Vector<Button *> out;
	for (int i = 0; i < sorted.size(); i++) {
		out.push_back(sorted[i]->button);
	}
	return out;
}

Button *ClockGearsUiDesignLayer::find_next_focus_button(Button *p_current, int p_dir) const {
	Vector<Button *> btns = get_focus_ordered_buttons();
	if (btns.is_empty()) {
		return nullptr;
	}
	if (!p_current || !btns.has(p_current)) {
		return btns[0];
	}

	// 1. Comprobar enlace manual explícito (focus_next_up/down/left/right)
	const ElementEntry *cur_entry = nullptr;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].button == p_current) {
			cur_entry = &elements[i];
			break;
		}
	}
	if (cur_entry) {
		const char *keys[4] = { "focus_next_up", "focus_next_right", "focus_next_down", "focus_next_left" };
		int k_idx = CLAMP(p_dir, 0, 3);
		String target_id = String(cur_entry->config.get(keys[k_idx], "")).strip_edges().to_lower();
		if (!target_id.is_empty()) {
			for (int i = 0; i < elements.size(); i++) {
				if (elements[i].id.to_lower() == target_id && elements[i].button &&
						elements[i].root && elements[i].root->is_visible() && !elements[i].button->is_disabled()) {
					return elements[i].button;
				}
			}
		}
	}

	// 2. Navegación espacial geométrica por posición (3G)
	Vector2 dir_v;
	if (p_dir == 0) dir_v = Vector2(0, -1);      // Arriba
	else if (p_dir == 1) dir_v = Vector2(1, 0);  // Derecha
	else if (p_dir == 2) dir_v = Vector2(0, 1);  // Abajo
	else dir_v = Vector2(-1, 0);                 // Izquierda

	auto btn_center = [](Button *b) -> Vector2 {
		Node *p = b->get_parent();
		while (p) {
			Control *r = Object::cast_to<Control>(p);
			if (r && String(r->get_name()).begins_with("UiElem_")) {
				return r->get_position() + r->get_size() * 0.5f;
			}
			p = p->get_parent();
		}
		return b->get_global_position() + b->get_size() * 0.5f;
	};

	Vector2 origin = btn_center(p_current);
	Button *best = nullptr;
	float best_score = 1e9f;
	for (int i = 0; i < btns.size(); i++) {
		Button *cand = btns[i];
		if (cand == p_current) continue;
		Vector2 d = btn_center(cand) - origin;
		float along = d.dot(dir_v);
		if (along <= 4.0f) continue;
		float perp = Math::abs(d.x * dir_v.y - d.y * dir_v.x);
		float score = along + perp * 2.2f;
		if (score < best_score) {
			best_score = score;
			best = cand;
		}
	}
	if (best) {
		return best;
	}
	// Wrap cíclico en lista ordenada si llega al borde
	int cur_idx = btns.find(p_current);
	int delta = (p_dir == 1 || p_dir == 2) ? 1 : -1;
	return btns[((cur_idx + delta) % btns.size() + btns.size()) % btns.size()];
}

Array ClockGearsUiDesignLayer::get_focus_order_actions() const {
	Vector<Button *> btns = get_focus_ordered_buttons();
	Array out;
	for (int i = 0; i < btns.size(); i++) {
		out.push_back(String(btns[i]->get_meta("ui_action", "")));
	}
	return out;
}

Control *ClockGearsUiDesignLayer::get_element_control(const String &p_elem_id) const {
	String target = p_elem_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id.to_lower() == target) {
			return elements[i].root;
		}
	}
	return nullptr;
}

Array ClockGearsUiDesignLayer::get_element_actions(const String &p_elem_id) const {
	String target = p_elem_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id.to_lower() == target) {
			if (!elements[i].actions.is_empty()) {
				return elements[i].actions.duplicate(true);
			}
			String act = elements[i].action.strip_edges().to_lower();
			if (!act.is_empty() && act != "none") {
				Dictionary step = elements[i].config.duplicate(true);
				step["type"] = act;
				Array single;
				single.push_back(step);
				return single;
			}
			return Array();
		}
	}
	if (radial_canvas) {
		Dictionary info = radial_canvas->get_selected_slot_info();
		if (String(info.get("id", "")).to_lower() == target) {
			return Array(info.get("actions", Array())).duplicate(true);
		}
	}
	return Array();
}

String ClockGearsUiDesignLayer::get_element_displayed_text(const String &p_elem_id) const {
	String target = p_elem_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id.to_lower() == target) {
			if (elements[i].button) {
				return elements[i].button->get_text();
			}
			if (elements[i].bar_label) {
				return elements[i].bar_label->get_text();
			}
			if (elements[i].label) {
				return elements[i].label->get_text();
			}
		}
	}
	return String();
}

Dictionary ClockGearsUiDesignLayer::get_layout_state_info() const {
	Dictionary d;
	Dictionary lay = active_design.get("layout", Dictionary());
	d["mode"] = String(lay.get("mode", "free"));
	d["current_page"] = current_layout_page;
	d["current_tab"] = current_layout_tab;
	d["current_carousel_index"] = current_carousel_index;
	d["is_radial"] = (radial_canvas != nullptr && radial_canvas->is_visible());
	d["radial_child_count"] = radial_canvas ? radial_canvas->get_child_count() : -1;
	d["radial_slot_count"] = radial_canvas ? radial_canvas->get_slot_count() : 0;
	d["cursor"] = cursor_overlay ? cursor_overlay->get_cursor_state() : Dictionary();
	return d;
}

Control *ClockGearsUiDesignLayer::get_widget_mount(const String &p_widget_kind) const {
	String wk = p_widget_kind.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.type == "widget" && e.widget_kind == wk && e.content_root) {
			return e.content_root;
		}
	}
	return nullptr;
}

bool ClockGearsUiDesignLayer::has_button_action(const String &p_action) const {
	String act = p_action.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.type == "button" && e.visible) {
			if (e.action == act) {
				return true;
			}
			for (int j = 0; j < e.actions.size(); j++) {
				if (e.actions[j].get_type() == Variant::DICTIONARY &&
						String(Dictionary(e.actions[j]).get("type", "")).strip_edges().to_lower() == act) {
					return true;
				}
			}
		}
	}
	return false;
}

bool ClockGearsUiDesignLayer::trigger_action(const String &p_action) {
	String act = p_action.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.type == "button" && e.action == act && e.button && e.root && e.root->is_visible() && !e.button->is_disabled()) {
			emit_signal("action_triggered", e.action, e.id);
			return true;
		}
	}
	return false;
}

Dictionary ClockGearsUiDesignLayer::get_layout_bounds_info(const Vector2 &p_viewport_size) const {
	Size2 vp = p_viewport_size;
	if (vp.x < 64.0f || vp.y < 64.0f) {
		vp = _resolve_effective_viewport_size();
	}
	ClockGearsUiDesignLayer *mut = const_cast<ClockGearsUiDesignLayer *>(this);
	Size2 prev_ov = viewport_size_override;
	mut->viewport_size_override = vp;
	mut->last_layout_size = Vector2(-1, -1);
	mut->_relayout_elements(true);

	Rect2 vp_rect(Vector2(0, 0), vp);
	Dictionary elem_rects;
	Array btn_rects;
	bool clips = false;
	bool btn_overlap = false;

	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (!e.root || !e.root->is_visible()) {
			continue;
		}
		Rect2 r(e.root->get_position(), e.root->get_size());
		elem_rects[e.id] = r;
		if (!vp_rect.grow(1.5f).encloses(r)) {
			clips = true;
		}
		if (e.type == "button") {
			for (int j = 0; j < btn_rects.size(); j++) {
				if (Rect2(btn_rects[j]).intersects(r)) {
					btn_overlap = true;
					break;
				}
			}
			btn_rects.push_back(r);
		}
	}

	Dictionary info;
	info["screen_id"] = screen_id;
	info["viewport"] = vp;
	info["stage_rect"] = current_stage_rect;
	info["ui_scale"] = current_ui_scale;
	info["element_count"] = elements.size();
	info["element_rects"] = elem_rects;
	info["button_rects"] = btn_rects;
	info["clips"] = clips;
	info["button_overlap"] = btn_overlap;
	info["any_overlap"] = btn_overlap;
	info["all_within_viewport"] = !clips;

	mut->viewport_size_override = prev_ov;
	mut->last_layout_size = Vector2(-1, -1);
	return info;
}

void ClockGearsUiDesignLayer::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		set_process(true);
		_relayout_elements(true);
	} else if (p_what == NOTIFICATION_RESIZED) {
		_relayout_elements(false);
	} else if (p_what == NOTIFICATION_PROCESS) {
		if (!is_visible()) {
			return;
		}
		double dt = get_process_delta_time();
		if (transition_active) {
			transition_elapsed += dt;
			float t = (transition_duration > 0.0001) ? float(transition_elapsed / transition_duration) : 1.0f;
			if (t >= 1.0f) {
				t = 1.0f;
				transition_active = false;
			}
			_apply_transition_pose(t);
		}
		Dictionary bg = (active_design.has("background") && active_design["background"].get_type() == Variant::DICTIONARY)
				? Dictionary(active_design["background"])
				: Dictionary();
		int sx = int(bg.get("scroll_speed_x", 0));
		int sy = int(bg.get("scroll_speed_y", 0));
		if ((sx != 0 || sy != 0) && bg_backdrop_ctrl) {
			bg_scroll_offset += Vector2(float(sx), float(sy)) * float(dt);
			if (ClockGearsUiBackdropDraw *bd = Object::cast_to<ClockGearsUiBackdropDraw>(bg_backdrop_ctrl)) {
				bd->set_scroll_offset(bg_scroll_offset);
			}
		}
		if (!preview_mode) {
			refresh_live_values();
		}
	}
}

void ClockGearsUiDesignLayer::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsUiDesignLayer", D_METHOD("compute_anchor_factors", "anchor"), &ClockGearsUiDesignLayer::compute_anchor_factors);
	ClassDB::bind_static_method("ClockGearsUiDesignLayer", D_METHOD("compute_stage_rect", "viewport_size"), &ClockGearsUiDesignLayer::compute_stage_rect);
	ClassDB::bind_static_method("ClockGearsUiDesignLayer", D_METHOD("compute_element_rect", "elem", "stage_rect", "scale"), &ClockGearsUiDesignLayer::compute_element_rect);
	ClassDB::bind_method(D_METHOD("set_screen_id", "screen_id"), &ClockGearsUiDesignLayer::set_screen_id);
	ClassDB::bind_method(D_METHOD("get_screen_id"), &ClockGearsUiDesignLayer::get_screen_id);
	ClassDB::bind_method(D_METHOD("set_window_type_fallback", "win_type"), &ClockGearsUiDesignLayer::set_window_type_fallback);
	ClassDB::bind_method(D_METHOD("get_window_type_fallback"), &ClockGearsUiDesignLayer::get_window_type_fallback);
	ClassDB::bind_method(D_METHOD("set_preview_mode", "preview"), &ClockGearsUiDesignLayer::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsUiDesignLayer::is_preview_mode);
	ClassDB::bind_method(D_METHOD("set_viewport_size_override", "size"), &ClockGearsUiDesignLayer::set_viewport_size_override);
	ClassDB::bind_method(D_METHOD("get_viewport_size_override"), &ClockGearsUiDesignLayer::get_viewport_size_override);
	ClassDB::bind_method(D_METHOD("set_action_disabled", "action", "disabled"), &ClockGearsUiDesignLayer::set_action_disabled);
	ClassDB::bind_method(D_METHOD("clear_disabled_actions"), &ClockGearsUiDesignLayer::clear_disabled_actions);
	ClassDB::bind_method(D_METHOD("apply_screen_design", "screen_id", "design_override"), &ClockGearsUiDesignLayer::apply_screen_design, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("capture_backdrop_from_viewport"), &ClockGearsUiDesignLayer::capture_backdrop_from_viewport);
	ClassDB::bind_method(D_METHOD("update_element_geometry_fast", "elem_id", "elem"), &ClockGearsUiDesignLayer::update_element_geometry_fast);
	ClassDB::bind_method(D_METHOD("play_transition_in"), &ClockGearsUiDesignLayer::play_transition_in);
	ClassDB::bind_method(D_METHOD("play_transition_out"), &ClockGearsUiDesignLayer::play_transition_out);
	ClassDB::bind_method(D_METHOD("is_transition_active"), &ClockGearsUiDesignLayer::is_transition_active);
	ClassDB::bind_method(D_METHOD("clear_elements"), &ClockGearsUiDesignLayer::clear_elements);
	ClassDB::bind_method(D_METHOD("refresh_live_values"), &ClockGearsUiDesignLayer::refresh_live_values);
	ClassDB::bind_method(D_METHOD("get_focus_order_actions"), &ClockGearsUiDesignLayer::get_focus_order_actions);
	ClassDB::bind_method(D_METHOD("get_element_control", "elem_id"), &ClockGearsUiDesignLayer::get_element_control);
	ClassDB::bind_method(D_METHOD("get_widget_mount", "widget_kind"), &ClockGearsUiDesignLayer::get_widget_mount);
	ClassDB::bind_method(D_METHOD("has_button_action", "action"), &ClockGearsUiDesignLayer::has_button_action);
	ClassDB::bind_method(D_METHOD("get_element_actions", "elem_id"), &ClockGearsUiDesignLayer::get_element_actions);
	ClassDB::bind_method(D_METHOD("get_element_displayed_text", "elem_id"), &ClockGearsUiDesignLayer::get_element_displayed_text);
	ClassDB::bind_method(D_METHOD("get_element_count"), &ClockGearsUiDesignLayer::get_element_count);
	ClassDB::bind_method(D_METHOD("get_radial_canvas"), &ClockGearsUiDesignLayer::get_radial_canvas);
	ClassDB::bind_method(D_METHOD("is_radial_mode"), &ClockGearsUiDesignLayer::is_radial_mode);
	ClassDB::bind_method(D_METHOD("get_layout_state_info"), &ClockGearsUiDesignLayer::get_layout_state_info);
	ClassDB::bind_method(D_METHOD("trigger_action", "action"), &ClockGearsUiDesignLayer::trigger_action);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsUiDesignLayer::get_layout_bounds_info, DEFVAL(Vector2()));

	ADD_SIGNAL(MethodInfo("action_triggered", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::STRING, "element_id")));
	ADD_SIGNAL(MethodInfo("radial_selection_changed", PropertyInfo(Variant::INT, "slot_index")));
}

ClockGearsUiDesignLayer::ClockGearsUiDesignLayer() {
	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_process_unhandled_input(true);
}

void ClockGearsUiDesignLayer::unhandled_input(const Ref<InputEvent> &p_event) {
	if (preview_mode || !is_visible_in_tree() || p_event.is_null() || !p_event->is_pressed() || p_event->is_echo()) {
		return;
	}
	Ref<InputEventKey> ek = p_event;
	Ref<InputEventJoypadButton> ej = p_event;
	if (ek.is_null() && ej.is_null()) {
		return;
	}
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.type != "button" || !e.button || !e.root || !e.root->is_visible() || e.button->is_disabled()) {
			continue;
		}
		if (!e.config.has("shortcut") || e.config["shortcut"].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sc = e.config["shortcut"];
		String kind = String(sc.get("kind", "key")).strip_edges().to_lower();
		bool hit = false;
		if (kind == "key" && ek.is_valid()) {
			int64_t kc = int64_t(sc.get("keycode", 0));
			if (kc > 0 && ((int64_t)ek->get_keycode() == kc || (int64_t)ek->get_physical_keycode() == kc)) {
				hit = true;
			}
		} else if (kind == "joy_button" && ej.is_valid()) {
			if (ej->get_button_index() == (JoyButton)int(sc.get("joy_button", -1))) {
				hit = true;
			}
		}
		if (hit) {
			get_viewport()->set_input_as_handled();
			_on_button_pressed(e.action, e.id);
			return;
		}
	}
}
