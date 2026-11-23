#include "clockgears_ui_design_layer.h"

#include "../core/clockgears_data_manager.h"
#include "clockgears_audio_manager.h"
#include "clockgears_title_screen.h"
#include "clockgears_window_style.h"
#include "core/input/input.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "scene/gui/box_container.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/nine_patch_rect.h"
#include "scene/gui/scroll_container.h"
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
				} else if (sm == "manual") {
					Rect2 dr = ClockGearsUiDesignLayer::compute_manual_image_rect(Rect2(Vector2(), sz), ts, bg_cfg);
					draw_texture_rect(bg_tex, dr, false);
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
	bool int_sc = bool(bg_cfg.get("manual_integer_scale", false));
	set_texture_filter((pa || sm == "integer" || (sm == "manual" && int_sc)) ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
	queue_redraw();
	}
	void set_scroll_offset(const Vector2 &p_off) {
		scroll_offset = p_off;
		queue_redraw();
	}
};

class UiButtonManualImageHost : public Control {
	GDCLASS(UiButtonManualImageHost, Control);

	Button *button_ref = nullptr;
	Dictionary cfg;
	Ref<Texture2D> tex_normal;
	Ref<Texture2D> tex_hover;
	Ref<Texture2D> tex_pressed;
	Ref<Texture2D> tex_disabled;
	Ref<Texture2D> tex_fallback;

protected:
	void _notification(int p_what) {
		if (p_what != NOTIFICATION_DRAW) {
			return;
		}
		Size2 sz = get_size();
		if (sz.x <= 0.0f || sz.y <= 0.0f) {
			return;
		}
		Ref<Texture2D> active_tex = tex_normal;
		if (button_ref) {
			if (button_ref->is_disabled() && tex_disabled.is_valid()) {
				active_tex = tex_disabled;
			} else if (button_ref->is_pressed() && tex_pressed.is_valid()) {
				active_tex = tex_pressed;
			} else if (button_ref->is_hovered() && tex_hover.is_valid()) {
				active_tex = tex_hover;
			}
		}
		if (active_tex.is_null()) {
			active_tex = tex_normal.is_valid() ? tex_normal : (tex_hover.is_valid() ? tex_hover : tex_fallback);
		}
		if (active_tex.is_null()) {
			return;
		}
		Vector2 ts = active_tex->get_size();
		if (ts.x <= 0.0f || ts.y <= 0.0f) {
			return;
		}
		Rect2 dr = ClockGearsUiDesignLayer::compute_manual_image_rect(Rect2(Vector2(), sz), ts, cfg);
		draw_texture_rect(active_tex, dr, false);
	}

public:
	void configure(Button *p_btn, const Dictionary &p_cfg, const Ref<Texture2D> &p_norm, const Ref<Texture2D> &p_hov, const Ref<Texture2D> &p_prs, const Ref<Texture2D> &p_dis, const Ref<Texture2D> &p_fb) {
		button_ref = p_btn;
		cfg = p_cfg;
		tex_normal = p_norm;
		tex_hover = p_hov;
		tex_pressed = p_prs;
		tex_disabled = p_dis;
		tex_fallback = p_fb;
		bool pa = cfg.has("pixel_art") ? bool(cfg["pixel_art"]) : true;
		bool int_sc = bool(cfg.get("manual_integer_scale", false));
		set_texture_filter((pa || int_sc) ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
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
	if (clip_active) {
		set_clip_children_mode(p_draw_fill ? CLIP_CHILDREN_AND_DRAW : CLIP_CHILDREN_ONLY);
	} else {
		set_clip_children_mode(CLIP_CHILDREN_DISABLED);
	}
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
		if (shadow_enabled && shadow_size > 0.5f && shadow_color.a > 0.001f) {
			float sh_expand = float(border_width) * 0.5f + shadow_size * 0.5f;
			Rect2 sr = r.grow(sh_expand);
			sr.position += shadow_offset;
			Vector<Vector2> spts = ClockGearsWindowStyleBox::build_shape_polygon(sr, shape_type, corner_radius + sh_expand, 24);
			if (spts.size() >= 3) {
				Vector<Vector2> closed = spts;
				closed.push_back(spts[0]);
				draw_polyline(closed, shadow_color, shadow_size * 1.25f, !pixel_art);
			}
		}
		if (glow_enabled && glow_size > 0.5f && glow_color.a > 0.001f) {
			float gl_expand = float(border_width) * 0.5f + glow_size * 0.6f;
			Rect2 gr = r.grow(gl_expand);
			Vector<Vector2> gpts = ClockGearsWindowStyleBox::build_shape_polygon(gr, shape_type, corner_radius + gl_expand, 24);
			if (gpts.size() >= 3) {
				Vector<Vector2> closed = gpts;
				closed.push_back(gpts[0]);
				draw_polyline(closed, glow_color, glow_size * 1.4f, !pixel_art);
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
	ClassDB::bind_method(D_METHOD("get_radial_layout_config"), &ClockGearsRadialCanvas::get_radial_layout_config);
	ClassDB::bind_method(D_METHOD("get_radial_slot_visual_config", "index"), &ClockGearsRadialCanvas::get_radial_slot_visual_config);

	ADD_SIGNAL(MethodInfo("slot_activated", PropertyInfo(Variant::INT, "slot_index")));
	ADD_SIGNAL(MethodInfo("selection_changed", PropertyInfo(Variant::INT, "slot_index")));
}

void ClockGearsRadialCanvas::set_data_context(int p_actor_id, int p_equip_slot_idx, bool p_test_data) {
	runtime_actor_id = p_actor_id;
	runtime_equip_slot_idx = p_equip_slot_idx;
	use_editor_test_data = p_test_data;
}

void ClockGearsRadialCanvas::configure(const Dictionary &p_layout, const Array &p_fallback_elements, float p_scale, bool p_interactive) {
	layout_cfg = p_layout.duplicate(true);
	ui_scale = MAX(0.25f, p_scale);
	interactive = p_interactive;
	open_anim = String(layout_cfg.get("radial_open_anim", "radial_spin")).strip_edges().to_lower();
	all_slots.clear();

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String ds = ClockGearsDataManager::normalize_ui_data_source(layout_cfg.get("data_source", "manual"));
	String count_src = String(layout_cfg.get("radial_slot_count_source", "manual")).strip_edges().to_lower();
	Array raw_slots = layout_cfg.get("radial_slots", Array());

	auto load_opt_tex = [&](const Variant &p_path) -> Ref<Texture2D> {
		String sp = String(p_path).strip_edges();
		return sp.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(sp);
	};
	cached_ring_tex = load_opt_tex(layout_cfg.get("radial_ring_image", ""));
	cached_center_tex = load_opt_tex(layout_cfg.get("radial_center_image", ""));
	cached_selected_tex = load_opt_tex(layout_cfg.get("radial_selected_image", ""));
	cached_indicator_tex = load_opt_tex(layout_cfg.get("radial_indicator_image", ""));

	if (ds != "manual" && dm) {
		const bool preview_or_test = (!interactive || use_editor_test_data || dm->get_editor_test_data_count() >= 0);
		Array entries = dm->resolve_ui_container_entries(layout_cfg, runtime_actor_id, runtime_equip_slot_idx, preview_or_test);
		int max_slots = (count_src == "manual")
				? CLAMP(int(layout_cfg.get("data_empty_count", layout_cfg.get("slot_count", 8))), 1, 16)
				: 16;
		int take_cnt = MIN(max_slots, entries.size());
		for (int i = 0; i < take_cnt; i++) {
			if (entries[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary ed = entries[i];
			RadialSlot rs;
			rs.is_empty = bool(ed.get("is_empty", false));
			rs.quantity = int(ed.get("qty", 0));
			rs.id = String(ed.get("key", vformat("data_%d", i + 1)));
			rs.title = String(ed.get("name", ""));
			if (rs.title.is_empty() && rs.is_empty) {
				rs.title = vformat(String::utf8("Vacía %d"), i + 1);
			}
			rs.description = String(ed.get("description", ed.get("desc", "")));
			rs.icon_path = String(ed.get("icon_path", "")).strip_edges();
			rs.pixel_art = layout_cfg.has("pixel_art") ? bool(layout_cfg["pixel_art"]) : true;
			rs.action = String(layout_cfg.get("slot_action", ed.get("slot_action", "show_detail"))).strip_edges().to_lower();
			rs.actions = layout_cfg.has("slot_actions") && layout_cfg["slot_actions"].get_type() == Variant::ARRAY ? Array(layout_cfg["slot_actions"]).duplicate(true) : Array();
			rs.disabled = bool(ed.get("disabled", false)) || rs.is_empty;
			rs.visible = true;
			if (!rs.is_empty) {
				rs.cached_icon = dm->get_entry_icon_texture(ed, MAX(24, int(layout_cfg.get("radial_slot_size", 56))));
			}
			all_slots.push_back(rs);
		}
		if (all_slots.is_empty() && bool(layout_cfg.get("data_show_empty", true))) {
			int emp_cnt = CLAMP(int(layout_cfg.get("data_empty_count", 8)), 1, 16);
			for (int i = 0; i < emp_cnt; i++) {
				RadialSlot rs;
				rs.id = vformat("empty_%d", i + 1);
				rs.title = vformat(String::utf8("Vacía %d"), i + 1);
				rs.is_empty = true;
				rs.disabled = true;
				rs.visible = true;
				all_slots.push_back(rs);
			}
		}
	} else if (!p_fallback_elements.is_empty()) {
		// Botones hijo reales del árbol (prioritarios frente a raw_slots heredados)
		Vector<Dictionary> btn_slots;
		for (int i = 0; i < p_fallback_elements.size(); i++) {
			if (p_fallback_elements[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary d = p_fallback_elements[i];
			String et = String(d.get("type", "")).strip_edges().to_lower();
			if (et != "button") continue;
			btn_slots.push_back(d);
		}
		if (!btn_slots.is_empty()) {
			for (int i = 0; i < btn_slots.size() && i < 16; i++) {
				const Dictionary &d = btn_slots[i];
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
				String vm = String(d.get("visibility_mode", "always")).strip_edges().to_lower();
				if (d.has("visible") && !bool(d["visible"])) {
					vm = "never";
				}
				rs.visibility_mode = vm;
				rs.visibility_switch = String(d.get("visibility_switch", "")).strip_edges();
				Dictionary dlay = (d.has("layout") && d["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(d["layout"]) : Dictionary();
				rs.shape = String(dlay.get("radial_slot_shape", d.get("slot_shape", ""))).strip_edges().to_lower();
				rs.image_normal = String(d.get("image_normal", "")).strip_edges();
				rs.image_hover = String(d.get("image_hover", "")).strip_edges();
				rs.image_pressed = String(d.get("image_pressed", "")).strip_edges();
				rs.image_disabled = String(d.get("image_disabled", "")).strip_edges();
				Dictionary dsh = (d.has("shape") && d["shape"].get_type() == Variant::DICTIONARY) ? Dictionary(d["shape"]) : Dictionary();
				if (dsh.has("fill_color")) {
					rs.has_fill_color = true;
					rs.fill_color = _cg_ui_parse_color(dsh["fill_color"], Color(0.11f, 0.12f, 0.16f, 0.94f));
				}
				if (dsh.has("border_color")) {
					rs.has_border_color = true;
					rs.border_color = _cg_ui_parse_color(dsh["border_color"], Color(0.96f, 0.74f, 0.33f, 1.0f));
				}
				if (d.has("font_color")) {
					rs.has_font_color = true;
					rs.font_color = _cg_ui_parse_color(d["font_color"], Color(0.95f, 0.93f, 0.88f, 1.0f));
				}
				if (!rs.icon_path.is_empty()) {
					rs.cached_icon = ClockGearsWindowStyleManager::load_texture_safe(rs.icon_path);
				}
				rs.tex_normal = load_opt_tex(rs.image_normal);
				rs.tex_hover = load_opt_tex(rs.image_hover);
				rs.tex_pressed = load_opt_tex(rs.image_pressed);
				rs.tex_disabled = load_opt_tex(rs.image_disabled);
				all_slots.push_back(rs);
			}
		}
	}
	if (ds == "manual" && all_slots.is_empty() && !raw_slots.is_empty()) {
		for (int i = 0; i < raw_slots.size() && i < 16; i++) {
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
			rs.shape = String(d.get("shape", "")).strip_edges().to_lower();
			rs.image_normal = String(d.get("image_normal", "")).strip_edges();
			rs.image_hover = String(d.get("image_hover", "")).strip_edges();
			rs.image_pressed = String(d.get("image_pressed", "")).strip_edges();
			rs.image_disabled = String(d.get("image_disabled", "")).strip_edges();
			rs.has_fill_color = d.has("fill_color");
			if (rs.has_fill_color) rs.fill_color = _cg_ui_parse_color(d["fill_color"], Color(0.11f, 0.12f, 0.16f, 0.94f));
			rs.has_border_color = d.has("border_color");
			if (rs.has_border_color) rs.border_color = _cg_ui_parse_color(d["border_color"], Color(0.96f, 0.74f, 0.33f, 1.0f));
			rs.has_font_color = d.has("font_color");
			if (rs.has_font_color) rs.font_color = _cg_ui_parse_color(d["font_color"], Color(0.95f, 0.93f, 0.88f, 1.0f));
			rs.slot_scale = d.has("slot_scale") ? CLAMP(float(d["slot_scale"]), 0.5f, 2.0f) : 1.0f;
			if (!rs.icon_path.is_empty()) {
				rs.cached_icon = ClockGearsWindowStyleManager::load_texture_safe(rs.icon_path);
			}
			rs.tex_normal = load_opt_tex(rs.image_normal);
			rs.tex_hover = load_opt_tex(rs.image_hover);
			rs.tex_pressed = load_opt_tex(rs.image_pressed);
			rs.tex_disabled = load_opt_tex(rs.image_disabled);
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
		if (rs.visibility_mode == "never" || rs.visibility_mode == "hidden") {
			vis = false;
		} else if (p_dm && rs.visibility_mode == "switch" && !rs.visibility_switch.is_empty()) {
			vis = p_dm->get_switch(rs.visibility_switch);
		}
		bool dis = (rs.enabled_mode == "disabled") || rs.is_empty;
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

void ClockGearsRadialCanvas::play_open_anim() {
	if (interactive) {
		String sopen = String(layout_cfg.get("radial_sound_open", "")).strip_edges();
		if (!sopen.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_se(sopen, 1.0f, 1.0f);
			}
		}
	}
	if (!interactive || open_anim == "none" || open_anim.is_empty()) {
		open_anim_progress = 1.0f;
		open_anim_playing = false;
		set_modulate(Color(1, 1, 1, 1));
		queue_redraw();
		return;
	}
	open_anim_progress = 0.0f;
	open_anim_playing = true;
	set_process(true);
	queue_redraw();
}

Array ClockGearsRadialCanvas::get_slot_actions_by_id(const String &p_id) const {
	String tid = p_id.strip_edges().to_lower();
	for (int i = 0; i < all_slots.size(); i++) {
		if (all_slots[i].id.to_lower() == tid) {
			if (!all_slots[i].actions.is_empty()) {
				return all_slots[i].actions.duplicate(true);
			}
			String act = all_slots[i].action.strip_edges().to_lower();
			if (!act.is_empty() && act != "none") {
				Dictionary step;
				step["type"] = act;
				Array single;
				single.push_back(step);
				return single;
			}
			return Array();
		}
	}
	return Array();
}

float ClockGearsRadialCanvas::_compute_radius_px() const {
	float r = float(CLAMP(int(layout_cfg.get("radial_radius", 145)), 24, 300)) * ui_scale;
	Size2 sz = get_size();
	if (sz.x > 16.0f && sz.y > 16.0f) {
		bool is_elem = bool(layout_cfg.get("_is_element", false));
		float slot_sz = float(CLAMP(int(layout_cfg.get("radial_slot_size", 64)), 28, 140)) * ui_scale;
		float max_r = is_elem ? MAX(24.0f, MIN(sz.x, sz.y) * 0.5f - slot_sz * 0.62f) : (MIN(sz.x, sz.y) * 0.38f);
		r = MIN(r, max_r);
	}
	if (open_anim_playing && (open_anim == "scale" || open_anim == "radial_spin")) {
		r *= Math::lerp(0.35f, 1.0f, open_anim_progress);
	}
	return MAX(16.0f, r);
}

float ClockGearsRadialCanvas::_compute_slot_angle(int p_visible_idx, int p_count) const {
	float start_rad = Math::deg_to_rad(float(layout_cfg.get("radial_start_angle_deg", -90.0)));
	if (open_anim_playing && open_anim == "radial_spin") {
		start_rad += (1.0f - open_anim_progress) * -0.9f;
	}
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
	if (interactive) {
		String smove = String(layout_cfg.get("radial_sound_move", "")).strip_edges();
		if (!smove.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_se(smove, 1.0f, 1.0f);
			}
		}
	}
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
	if (interactive) {
		String sacc = String(layout_cfg.get("radial_sound_accept", "")).strip_edges();
		if (!sacc.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_se(sacc, 1.0f, 1.0f);
			}
		}
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

String ClockGearsRadialCanvas::get_tooltip(const Point2 &p_pos) const {
	float base_slot = float(CLAMP(int(layout_cfg.get("radial_slot_size", 64)), 28, 140)) * ui_scale;
	float hit_r = base_slot * 0.65f;
	for (int i = 0; i < active_slot_indices.size(); i++) {
		Vector2 sp = get_slot_local_pos(i);
		if (p_pos.distance_to(sp) <= hit_r) {
			const RadialSlot &rs = all_slots[active_slot_indices[i]];
			String tip = rs.title;
			if (!rs.description.is_empty()) {
				tip += "\n" + rs.description;
			}
			return tip;
		}
	}
	return Control::get_tooltip(p_pos);
}

Dictionary ClockGearsRadialCanvas::get_center_displayed_info() const {
	Dictionary info = get_selected_slot_info();
	info["show_center_info"] = layout_cfg.has("radial_show_center_info") ? bool(layout_cfg["radial_show_center_info"]) : true;
	return info;
}

Dictionary ClockGearsRadialCanvas::get_radial_slot_visual_config(int p_index) const {
	Dictionary d;
	if (p_index < 0 || p_index >= active_slot_indices.size()) {
		return d;
	}
	const RadialSlot &rs = all_slots[active_slot_indices[p_index]];
	String def_shape = String(layout_cfg.get("radial_slot_shape", "circle")).strip_edges().to_lower();
	d["id"] = rs.id;
	d["title"] = rs.title;
	d["description"] = rs.description;
	d["shape"] = rs.shape.is_empty() ? def_shape : rs.shape;
	d["icon_path"] = rs.icon_path;
	d["image_normal"] = rs.image_normal;
	d["image_hover"] = rs.image_hover;
	d["image_pressed"] = rs.image_pressed;
	d["image_disabled"] = rs.image_disabled;
	d["action"] = rs.action;
	d["actions"] = rs.actions.duplicate(true);
	d["disabled"] = rs.disabled;
	d["visible"] = rs.visible;
	return d;
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
		if (!anim_active && !open_anim_playing) {
			set_process(false);
			return;
		}
		float dt = float(get_process_delta_time());
		if (open_anim_playing) {
			float dur_s = MAX(0.05f, float(layout_cfg.get("radial_open_duration_ms", 220)) / 1000.0f);
			float lin = MIN(1.0f, open_anim_progress + dt / dur_s);
			String curve = String(layout_cfg.get("radial_open_curve", "ease_out")).strip_edges().to_lower();
			float eased = lin;
			if (curve == "ease_out") {
				float inv = 1.0f - lin;
				eased = 1.0f - inv * inv * inv;
			} else if (curve == "back_out") {
				float inv = lin - 1.0f;
				eased = 1.0f + 2.70158f * inv * inv * inv + 1.70158f * inv * inv;
			} else if (curve == "bounce") {
				eased = lin < 0.7f ? (lin / 0.7f) : (1.0f + Math::sin((lin - 0.7f) / 0.3f * Math::PI) * 0.08f);
			}
			open_anim_progress = lin;
			if (open_anim == "fade" || open_anim == "scale" || open_anim == "radial_spin") {
				set_modulate(Color(1, 1, 1, CLAMP(eased, 0.0f, 1.0f)));
			}
			if (open_anim_progress >= 1.0f) {
				open_anim_playing = false;
				set_modulate(Color(1, 1, 1, 1));
			}
		}
		float sel_sc = CLAMP(float(layout_cfg.get("radial_selected_scale", 1.28)), 1.0f, 1.8f);
		float speed = 18.0f * dt;
		bool still_moving = open_anim_playing;
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
		float slot_sp = float(CLAMP(int(layout_cfg.get("radial_slot_spacing", 0)), 0, 64)) * ui_scale;
		float base_slot = float(CLAMP(int(layout_cfg.get("radial_slot_size", 64)), 28, 140)) * ui_scale;
		float sweep_deg = CLAMP(float(layout_cfg.get("radial_sweep_angle_deg", 360.0)), 45.0f, 360.0f);
		float start_rad = Math::deg_to_rad(float(layout_cfg.get("radial_start_angle_deg", -90.0)));
		float sweep_rad = Math::deg_to_rad(sweep_deg);
		bool cw = layout_cfg.has("radial_clockwise") ? bool(layout_cfg["radial_clockwise"]) : true;
		Color ring_col = _cg_ui_parse_color(layout_cfg.get("radial_ring_color", "#1a1e2ad9"), Color(0.10f, 0.12f, 0.16f, 0.85f));
		Color accent_col = _cg_ui_parse_color(layout_cfg.get("radial_accent_color", "#f5bc54ff"), Color(0.96f, 0.74f, 0.33f, 1.0f));
		Color sel_bord_col = _cg_ui_parse_color(layout_cfg.get("radial_selected_border_color", layout_cfg.get("radial_accent_color", "#f5bc54ff")), accent_col);
		float sel_bord_w = float(CLAMP(int(layout_cfg.get("radial_selected_border_width", 3)), 0, 12)) * ui_scale;
		bool sel_glow = layout_cfg.has("radial_selected_glow") ? bool(layout_cfg["radial_selected_glow"]) : true;
		Color sel_glow_col = _cg_ui_parse_color(layout_cfg.get("radial_selected_glow_color", "#f5bc5466"), Color(accent_col.r, accent_col.g, accent_col.b, 0.35f));
		String default_slot_shape = String(layout_cfg.get("radial_slot_shape", "circle")).strip_edges().to_lower();

		// 1. Pista circular / imagen de fondo del anillo
		int ring_th = int(layout_cfg.get("radial_ring_thickness", 0));
		float track_w = (ring_th > 0) ? (float(ring_th) * ui_scale) : (base_slot * 0.92f + slot_sp * 0.35f);
		if (cached_ring_tex.is_valid()) {
			float r_side = (radius + track_w * 0.65f) * 2.0f;
			Rect2 r_rect(center - Vector2(r_side * 0.5f, r_side * 0.5f), Vector2(r_side, r_side));
			draw_texture_rect(cached_ring_tex, r_rect, false, Color(1, 1, 1, ring_col.a));
		} else {
			float end_rad = start_rad + (cw ? sweep_rad : -sweep_rad);
			_cg_draw_thick_arc(this, center, radius, MIN(start_rad, end_rad), MAX(start_rad, end_rad), track_w, ring_col, true);
			_cg_draw_thick_arc(this, center, radius + track_w * 0.5f, MIN(start_rad, end_rad), MAX(start_rad, end_rad), MAX(1.5f, 2.0f * ui_scale), Color(accent_col.r, accent_col.g, accent_col.b, 0.42f), true);
			_cg_draw_thick_arc(this, center, MAX(8.0f, radius - track_w * 0.5f), MIN(start_rad, end_rad), MAX(start_rad, end_rad), MAX(1.5f, 2.0f * ui_scale), Color(accent_col.r, accent_col.g, accent_col.b, 0.35f), true);
		}

		int cnt = active_slot_indices.size();
		if (cnt <= 0) {
			return;
		}

		// 2. Indicador o cursor que gira hacia la ranura seleccionada
		bool ind_on = layout_cfg.has("radial_indicator_enabled") ? bool(layout_cfg["radial_indicator_enabled"]) : true;
		if (ind_on) {
			Color ind_col = _cg_ui_parse_color(layout_cfg.get("radial_indicator_color", layout_cfg.get("radial_accent_color", "#f5bc54ff")), accent_col);
			Vector2 sel_pos_px = get_slot_local_pos(selected_pos);
			Vector2 dir_u = (sel_pos_px - center);
			float dist = dir_u.length();
			if (dist > 1.0f) {
				dir_u /= dist;
				if (cached_indicator_tex.is_valid()) {
					float ang = Math::atan2(dir_u.y, dir_u.x);
					Vector2 ind_pos = center + dir_u * MAX(20.0f, radius * 0.52f);
					float isz = MAX(18.0f * ui_scale, base_slot * 0.42f);
					draw_set_transform(ind_pos, ang, Vector2(1, 1));
					draw_texture_rect(cached_indicator_tex, Rect2(-isz * 0.5f, -isz * 0.5f, isz, isz), false, ind_col);
					draw_set_transform(Vector2(), 0.0f, Vector2(1, 1));
				} else {
					Vector2 p0 = center + dir_u * MAX(18.0f, radius * 0.44f);
					Vector2 p1 = sel_pos_px - dir_u * (base_slot * 0.55f);
					draw_line(p0, p1, Color(ind_col.r, ind_col.g, ind_col.b, 0.85f), MAX(2.0f, 3.0f * ui_scale), true);
					Vector2 perp(-dir_u.y, dir_u.x);
					Vector<Vector2> tri;
					tri.push_back(p1 + dir_u * (7.0f * ui_scale));
					tri.push_back(p1 - dir_u * (3.0f * ui_scale) + perp * (5.0f * ui_scale));
					tri.push_back(p1 - dir_u * (3.0f * ui_scale) - perp * (5.0f * ui_scale));
					draw_colored_polygon(tri, ind_col);
				}
			}
		}

		Ref<Font> font = get_theme_default_font();
		int title_fsz = MAX(10, int(Math::round(15.0f * ui_scale)));
		int desc_fsz = MAX(9, int(Math::round(12.0f * ui_scale)));
		int badge_fsz = MAX(8, int(Math::round(10.0f * ui_scale)));

		// 3. Ranuras personalizables (formas: circle, sector, rectangle, image + imágenes de estado)
		float half_step = (cnt > 1) ? ((sweep_deg >= 359.5f) ? (sweep_rad / float(cnt)) : (sweep_rad / float(cnt - 1))) * 0.44f : 0.45f;
		for (int pass = 0; pass < 2; pass++) {
			for (int i = 0; i < cnt; i++) {
				bool is_sel = (i == selected_pos);
				if ((pass == 0 && is_sel) || (pass == 1 && !is_sel)) {
					continue;
				}
				const RadialSlot &rs = all_slots[active_slot_indices[i]];
				Vector2 sp = get_slot_local_pos(i);
				float sc = ((i < slot_visual_scales.size()) ? slot_visual_scales[i] : (is_sel ? 1.28f : 1.0f)) * rs.slot_scale;
				float srad = (base_slot * 0.46f) * sc;
				String eff_shape = rs.shape.is_empty() ? default_slot_shape : rs.shape;

				Color fill_c = rs.has_fill_color ? rs.fill_color : (is_sel ? Color(0.20f, 0.16f, 0.11f, 0.98f) : Color(0.11f, 0.12f, 0.16f, 0.94f));
				if (rs.disabled) {
					fill_c = Color(0.08f, 0.08f, 0.10f, 0.65f);
				}
				Color rim_c = rs.has_border_color ? rs.border_color : (is_sel ? sel_bord_col : Color(accent_col.r, accent_col.g, accent_col.b, rs.disabled ? 0.25f : 0.62f));
				float rim_w = is_sel ? MAX(1.0f, sel_bord_w) : MAX(1.0f, 1.5f * ui_scale);

				Ref<Texture2D> state_tex;
				if (rs.disabled && rs.tex_disabled.is_valid()) state_tex = rs.tex_disabled;
				else if (is_sel && rs.tex_hover.is_valid()) state_tex = rs.tex_hover;
				else if (rs.tex_normal.is_valid()) state_tex = rs.tex_normal;
				else if (is_sel && cached_selected_tex.is_valid()) state_tex = cached_selected_tex;

				if (eff_shape == "sector") {
					float sa = _compute_slot_angle(i, cnt);
					float r_in = MAX(18.0f * ui_scale, radius - srad * 0.95f);
					float r_out = radius + srad * 0.95f;
					Vector<Vector2> poly;
					const int segs = 10;
					for (int k = 0; k <= segs; k++) {
						float ang = Math::lerp(sa - half_step, sa + half_step, float(k) / float(segs));
						poly.push_back(center + Vector2(Math::cos(ang), Math::sin(ang)) * r_out);
					}
					for (int k = segs; k >= 0; k--) {
						float ang = Math::lerp(sa - half_step, sa + half_step, float(k) / float(segs));
						poly.push_back(center + Vector2(Math::cos(ang), Math::sin(ang)) * r_in);
					}
					if (is_sel && sel_glow) {
						draw_circle(sp, srad + 6.0f * ui_scale, sel_glow_col);
					}
					draw_colored_polygon(poly, fill_c);
					poly.push_back(poly[0]);
					draw_polyline(poly, rim_c, rim_w, true);
				} else if (eff_shape == "rectangle") {
					Rect2 rr(sp - Vector2(srad, srad), Vector2(srad * 2.0f, srad * 2.0f));
					if (is_sel && sel_glow) {
						draw_rect(rr.grow(4.0f * ui_scale), sel_glow_col);
					}
					draw_rect(rr, fill_c);
					draw_rect(rr, rim_c, false, rim_w);
				} else if (eff_shape == "image" && state_tex.is_valid()) {
					if (is_sel && sel_glow) {
						draw_circle(sp, srad + 5.0f * ui_scale, sel_glow_col);
					}
					Rect2 ir(sp - Vector2(srad, srad), Vector2(srad * 2.0f, srad * 2.0f));
					draw_texture_rect(state_tex, ir, false, rs.disabled ? Color(1, 1, 1, 0.45f) : Color(1, 1, 1, 1));
					if (is_sel && sel_bord_w > 0.5f) {
						draw_rect(ir, rim_c, false, rim_w);
					}
				} else {
					draw_circle(sp + Vector2(0, 3.0f * ui_scale), srad + 2.0f * ui_scale, Color(0, 0, 0, is_sel ? 0.50f : 0.34f));
					if (is_sel && sel_glow) {
						draw_circle(sp, srad + 5.0f * ui_scale, sel_glow_col);
					}
					draw_circle(sp, srad, fill_c);
					if (state_tex.is_valid()) {
						Rect2 ir(sp - Vector2(srad * 0.9f, srad * 0.9f), Vector2(srad * 1.8f, srad * 1.8f));
						draw_texture_rect(state_tex, ir, false, rs.disabled ? Color(1, 1, 1, 0.45f) : Color(1, 1, 1, 1));
					}
					if (rim_w > 0.1f) {
						draw_arc(sp, srad, 0.0f, float(Math::TAU), 36, rim_c, rim_w, true);
					}
				}

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
				} else if (font.is_valid() && !(eff_shape == "image" && state_tex.is_valid())) {
					float max_w = MAX(16.0f, srad * 1.72f);
					int min_fsz = MAX(8, int(Math::round(8.0f * ui_scale)));
					int fit_fsz = badge_fsz;
					String lbl = rs.title;
					bool single_ok = false;
					for (int f = badge_fsz; f >= min_fsz; f--) {
						if (font->get_string_size(lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, f).x <= max_w) {
							fit_fsz = f;
							single_ok = true;
							break;
						}
					}
					Color tc = rs.has_font_color ? rs.font_color : (rs.disabled ? Color(0.6f, 0.6f, 0.65f, 0.7f) : (is_sel ? accent_col : Color(0.95f, 0.93f, 0.88f, 1.0f)));
					auto fit_trunc = [&](const String &p_s, int p_f) -> String {
						if (font->get_string_size(p_s, HORIZONTAL_ALIGNMENT_LEFT, -1, p_f).x <= max_w) {
							return p_s;
						}
						String ell = String::utf8("…");
						int len = p_s.length();
						while (len > 1 && font->get_string_size(p_s.substr(0, len) + ell, HORIZONTAL_ALIGNMENT_LEFT, -1, p_f).x > max_w) {
							len--;
						}
						return p_s.substr(0, MAX(1, len)) + ell;
					};
					if (single_ok) {
						Vector2 tsz = font->get_string_size(lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, fit_fsz);
						draw_string(font, sp + Vector2(-tsz.x * 0.5f, tsz.y * 0.30f), lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, fit_fsz, tc);
					} else {
						PackedStringArray words = lbl.split(" ", false);
						String l1, l2;
						if (words.size() >= 2) {
							int best_k = 1;
							float best_diff = 1e9f;
							for (int wk = 1; wk < words.size(); wk++) {
								String a = String(" ").join(words.slice(0, wk));
								String b = String(" ").join(words.slice(wk));
								float diff = Math::abs(a.length() - b.length());
								if (diff < best_diff) {
									best_diff = diff;
									best_k = wk;
								}
							}
							l1 = String(" ").join(words.slice(0, best_k));
							l2 = String(" ").join(words.slice(best_k));
						} else {
							int mid = MAX(1, lbl.length() / 2);
							l1 = lbl.substr(0, mid);
							l2 = lbl.substr(mid);
						}
						int two_fsz = min_fsz;
						for (int f = MAX(min_fsz, badge_fsz - 1); f >= min_fsz; f--) {
							if (font->get_string_size(l1, HORIZONTAL_ALIGNMENT_LEFT, -1, f).x <= max_w &&
									font->get_string_size(l2, HORIZONTAL_ALIGNMENT_LEFT, -1, f).x <= max_w) {
								two_fsz = f;
								break;
							}
						}
						l1 = fit_trunc(l1, two_fsz);
						l2 = fit_trunc(l2, two_fsz);
						Vector2 s1 = font->get_string_size(l1, HORIZONTAL_ALIGNMENT_LEFT, -1, two_fsz);
						Vector2 s2 = font->get_string_size(l2, HORIZONTAL_ALIGNMENT_LEFT, -1, two_fsz);
						float line_h = MAX(s1.y, s2.y) * 0.92f;
						draw_string(font, sp + Vector2(-s1.x * 0.5f, -line_h * 0.18f), l1, HORIZONTAL_ALIGNMENT_LEFT, -1, two_fsz, tc);
						draw_string(font, sp + Vector2(-s2.x * 0.5f, line_h * 0.85f), l2, HORIZONTAL_ALIGNMENT_LEFT, -1, two_fsz, tc);
					}
				}
				if (font.is_valid() && !rs.is_empty && rs.quantity > 1) {
					String qtxt = "x" + itos(rs.quantity);
					int qfsz = MAX(8, int(Math::round(9.0f * ui_scale)));
					Vector2 qsz = font->get_string_size(qtxt, HORIZONTAL_ALIGNMENT_LEFT, -1, qfsz);
					Rect2 qbg(sp + Vector2(srad - qsz.x - 4.0f * ui_scale, srad - qsz.y - 2.0f * ui_scale), qsz + Vector2(4.0f * ui_scale, 2.0f * ui_scale));
					draw_rect(qbg, Color(0.05f, 0.06f, 0.09f, 0.82f));
					draw_string(font, qbg.position + Vector2(2.0f * ui_scale, qsz.y * 0.80f), qtxt, HORIZONTAL_ALIGNMENT_LEFT, -1, qfsz, accent_col);
				}
			}
		}

		// 4. Núcleo central (forma, imagen, icono grande, título y descripción configurables)
		bool show_center = layout_cfg.has("radial_show_center_info") ? bool(layout_cfg["radial_show_center_info"]) : true;
		String cshape = String(layout_cfg.get("radial_center_shape", "circle")).strip_edges().to_lower();
		if (show_center && cshape != "none") {
			const RadialSlot &active = all_slots[active_slot_indices[selected_pos]];
			float hub_r = CLAMP(radius - base_slot * 0.62f, 36.0f * ui_scale, 108.0f * ui_scale);
			if (cshape == "rounded") {
				Rect2 hbox(center - Vector2(hub_r, hub_r * 0.82f), Vector2(hub_r * 2.0f, hub_r * 1.64f));
				draw_rect(Rect2(hbox.position + Vector2(0, 3.0f * ui_scale), hbox.size), Color(0, 0, 0, 0.42f));
				draw_rect(hbox, Color(0.09f, 0.08f, 0.07f, 0.92f));
				draw_rect(hbox, Color(accent_col.r, accent_col.g, accent_col.b, 0.78f), false, MAX(1.5f, 2.0f * ui_scale));
			} else {
				draw_circle(center + Vector2(0, 3.0f * ui_scale), hub_r + 3.0f * ui_scale, Color(0, 0, 0, 0.45f));
				draw_circle(center, hub_r, Color(0.09f, 0.08f, 0.07f, 0.92f));
				draw_arc(center, hub_r, 0.0f, float(Math::TAU), 48, Color(accent_col.r, accent_col.g, accent_col.b, 0.78f), MAX(1.5f, 2.0f * ui_scale), true);
			}
			if (cached_center_tex.is_valid()) {
				Rect2 ctr_r(center - Vector2(hub_r * 0.88f, hub_r * 0.88f), Vector2(hub_r * 1.76f, hub_r * 1.76f));
				draw_texture_rect(cached_center_tex, ctr_r, false, Color(1, 1, 1, 0.92f));
			}

			bool show_icon = layout_cfg.has("radial_center_show_icon") ? bool(layout_cfg["radial_center_show_icon"]) : true;
			bool show_title = layout_cfg.has("radial_center_show_title") ? bool(layout_cfg["radial_center_show_title"]) : true;
			bool show_desc = layout_cfg.has("radial_center_show_desc") ? bool(layout_cfg["radial_center_show_desc"]) : true;

			float y_off = -hub_r * 0.45f;
			if (show_icon && active.cached_icon.is_valid()) {
				float c_ic = MIN(hub_r * 0.68f, 44.0f * ui_scale);
				Rect2 ic_r(center + Vector2(-c_ic * 0.5f, -hub_r * 0.68f), Vector2(c_ic, c_ic));
				draw_texture_rect(active.cached_icon, ic_r, false, active.disabled ? Color(1, 1, 1, 0.4f) : Color(1, 1, 1, 1));
				y_off = 4.0f * ui_scale;
			}
			if (font.is_valid()) {
				float max_tw = hub_r * 1.72f;
				if (show_title) {
					String title_txt = ClockGearsDataManager::truncate_with_ellipsis(active.title, 20);
					draw_string(font, center + Vector2(-max_tw * 0.5f, y_off + float(title_fsz) * 0.35f),
							title_txt, HORIZONTAL_ALIGNMENT_CENTER, max_tw, title_fsz, accent_col);
				}
				if (show_desc && !active.description.is_empty()) {
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
	set_visible(enabled || container_focus_enabled);
	set_process((enabled && anim != "none") || container_focus_enabled);
	queue_redraw();
}

void ClockGearsMenuCursorOverlay::set_target_rect(const Rect2 &p_rect, bool p_visible) {
	target_rect = p_rect;
	has_target = p_visible;
	queue_redraw();
}

void ClockGearsMenuCursorOverlay::set_container_focus(const Rect2 &p_rect, bool p_enabled, const String &p_mode, const Color &p_col, float p_width) {
	container_focus_rect = p_rect;
	container_focus_enabled = p_enabled && (p_mode != "none");
	container_focus_mode = p_mode;
	container_focus_color = p_col;
	container_focus_width = MAX(1.0f, p_width);
	bool btn_en = bool(cursor_cfg.get("enabled", false));
	set_visible(btn_en || container_focus_enabled);
	String anim = String(cursor_cfg.get("animation", "pulse")).strip_edges().to_lower();
	set_process((btn_en && anim != "none") || container_focus_enabled);
	queue_redraw();
}

Dictionary ClockGearsMenuCursorOverlay::get_cursor_state() const {
	Dictionary d;
	d["enabled"] = bool(cursor_cfg.get("enabled", false));
	d["has_target"] = has_target;
	d["target_rect"] = target_rect;
	d["shape"] = String(cursor_cfg.get("shape", "ring"));
	d["animation"] = String(cursor_cfg.get("animation", "pulse"));
	d["container_focus_enabled"] = container_focus_enabled;
	d["container_focus_rect"] = container_focus_rect;
	d["container_focus_mode"] = container_focus_mode;
	d["container_focus_color"] = "#" + container_focus_color.to_html(true);
	return d;
}

void ClockGearsMenuCursorOverlay::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		if (!is_visible_in_tree() || (!has_target && !container_focus_enabled)) {
			return;
		}
		anim_phase += get_process_delta_time() * 4.5;
		queue_redraw();
	} else if (p_what == NOTIFICATION_DRAW) {
		if (container_focus_enabled && container_focus_rect.has_area() && container_focus_mode != "none") {
			float pulse = (container_focus_mode == "ring") ? (1.5f * float(Math::sin(anim_phase)) * ui_scale) : 0.0f;
			Rect2 cf = container_focus_rect.grow(MAX(1.0f, 2.0f * ui_scale) + pulse);
			float bw = MAX(1.5f, container_focus_width * ui_scale);
			Color glow_col(container_focus_color.r, container_focus_color.g, container_focus_color.b, 0.28f);
			Vector<Vector2> glow_pts = ClockGearsWindowStyleBox::build_shape_polygon(cf.grow(bw * 0.9f), "rounded", 8.0f * ui_scale, 16);
			if (glow_pts.size() >= 3) {
				glow_pts.push_back(glow_pts[0]);
				draw_polyline(glow_pts, glow_col, bw * 1.8f, true);
			}
			Vector<Vector2> cf_pts = ClockGearsWindowStyleBox::build_shape_polygon(cf, "rounded", 8.0f * ui_scale, 16);
			if (cf_pts.size() >= 3) {
				cf_pts.push_back(cf_pts[0]);
				draw_polyline(cf_pts, container_focus_color, bw, true);
			}
		}
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

Rect2 ClockGearsUiDesignLayer::compute_manual_image_rect(const Rect2 &p_frame, const Vector2 &p_tex_size, const Dictionary &p_cfg) {
	if (p_tex_size.x <= 0.0f || p_tex_size.y <= 0.0f || p_frame.size.x <= 0.0f || p_frame.size.y <= 0.0f) {
		return p_frame;
	}
	float base_s = MIN(p_frame.size.x / p_tex_size.x, p_frame.size.y / p_tex_size.y);
	bool keep_asp = p_cfg.has("manual_keep_aspect") ? bool(p_cfg["manual_keep_aspect"]) : true;
	float sx_pct = CLAMP(float(p_cfg.get("manual_scale_x_pct", 100.0)), 5.0f, 1000.0f);
	float sy_pct = keep_asp ? sx_pct : CLAMP(float(p_cfg.get("manual_scale_y_pct", sx_pct)), 5.0f, 1000.0f);
	Vector2 dsz(p_tex_size.x * base_s * (sx_pct / 100.0f), p_tex_size.y * base_s * (sy_pct / 100.0f));
	float ox_pct = CLAMP(float(p_cfg.get("manual_offset_x_pct", 0.0)), -400.0f, 400.0f);
	float oy_pct = CLAMP(float(p_cfg.get("manual_offset_y_pct", 0.0)), -400.0f, 400.0f);
	Vector2 center = p_frame.position + p_frame.size * 0.5f + Vector2(p_frame.size.x * (ox_pct / 100.0f), p_frame.size.y * (oy_pct / 100.0f));
	return Rect2(center - dsz * 0.5f, dsz);
}

static float _cg_layer_elem_self_scale(const Dictionary &p_e) {
	float pct = CLAMP(float(p_e.get("scale_percent", 100.0)), 25.0f, 400.0f);
	float f = pct / 100.0f;
	if (bool(p_e.get("integer_scale", false))) {
		if (f >= 0.75f) f = float(MAX(1, int(Math::round(f))));
		else if (f >= 0.375f) f = 0.5f;
		else f = 0.25f;
	}
	return f;
}

HashMap<String, float> ClockGearsUiDesignLayer::compute_all_element_scales(const Array &p_elems) {
	HashMap<String, Dictionary> by_id;
	HashMap<String, String> canon;
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = p_elems[i];
		String id = String(e.get("id", ""));
		if (id.is_empty()) continue;
		by_id[id] = e;
		canon[id.to_lower()] = id;
	}
	HashMap<String, float> out;
	for (const KeyValue<String, Dictionary> &kv : by_id) {
		float acc = 1.0f;
		String cur = kv.key;
		HashSet<String> vis;
		for (int hop = 0; hop < 32 && !cur.is_empty() && by_id.has(cur) && !vis.has(cur); hop++) {
			vis.insert(cur);
			const Dictionary &e = by_id[cur];
			acc *= _cg_layer_elem_self_scale(e);
			String raw_pid = String(e.get("parent_id", "")).strip_edges().to_lower();
			cur = (!raw_pid.is_empty() && canon.has(raw_pid)) ? canon[raw_pid] : String();
		}
		out[kv.key] = acc;
	}
	return out;
}

Rect2 ClockGearsUiDesignLayer::compute_element_rect(const Dictionary &p_elem, const Rect2 &p_stage_rect, float p_scale) {
	String anchor = String(p_elem.get("anchor", "top_left")).strip_edges().to_lower();
	String etype = String(p_elem.get("type", "text")).strip_edges().to_lower();
	float es = _cg_layer_elem_self_scale(p_elem);
	float ew = (etype == "folder") ? (float(CLAMP(int(p_elem.get("width", 0)), 0, 960)) * es) : (float(CLAMP(int(p_elem.get("width", 120)), 16, 960)) * es);
	float eh = (etype == "folder") ? (float(CLAMP(int(p_elem.get("height", 0)), 0, 540)) * es) : (float(CLAMP(int(p_elem.get("height", 36)), 16, 540)) * es);
	float ox = float(int(p_elem.get("offset_x", 0)));
	float oy = float(int(p_elem.get("offset_y", 0)));

	Vector2 af = compute_anchor_factors(anchor);
	float base_x = af.x * (LOGICAL_WIDTH - ew) + ox;
	float base_y = af.y * (LOGICAL_HEIGHT - eh) + oy;
	Vector2 pos(Math::round(p_stage_rect.position.x + base_x * p_scale), Math::round(p_stage_rect.position.y + base_y * p_scale));
	Vector2 sz(etype == "folder" ? Math::round(ew * p_scale) : MAX(4.0f, Math::round(ew * p_scale)), etype == "folder" ? Math::round(eh * p_scale) : MAX(4.0f, Math::round(eh * p_scale)));
	return Rect2(pos, sz);
}

HashMap<String, Rect2> ClockGearsUiDesignLayer::compute_all_element_rects(const Array &p_elems, const Rect2 &p_stage_rect, float p_scale) {
	HashMap<String, Dictionary> by_id;
	HashMap<String, String> canon;
	HashMap<String, Vector<String>> ordered_children;
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = p_elems[i];
		String id = String(e.get("id", ""));
		if (id.is_empty()) continue;
		by_id[id] = e;
		canon[id.to_lower()] = id;
	}
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = p_elems[i];
		String id = String(e.get("id", ""));
		if (id.is_empty() || String(e.get("type", "")).to_lower() == "folder" || !bool(e.get("visible", true))) continue;
		String raw_pid = String(e.get("parent_id", "")).strip_edges().to_lower();
		if (!raw_pid.is_empty() && canon.has(raw_pid)) {
			ordered_children[canon[raw_pid]].push_back(id);
		}
	}
	HashMap<String, float> scales = compute_all_element_scales(p_elems);
	HashMap<String, Rect2> log_cache;
	struct LogResolver {
		static Rect2 run(const String &p_id, const HashMap<String, Dictionary> &p_by_id, const HashMap<String, String> &p_canon, const HashMap<String, Vector<String>> &p_children, const HashMap<String, float> &p_scales, HashMap<String, Rect2> &r_cache, int depth) {
			if (p_id.is_empty() || !p_by_id.has(p_id) || depth > 32) {
				return Rect2(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT);
			}
			if (r_cache.has(p_id)) {
				return r_cache[p_id];
			}
			const Dictionary &e = p_by_id[p_id];
			String raw_pid = String(e.get("parent_id", "")).strip_edges().to_lower();
			Rect2 pr(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT);
			String cpid;
			float p_acc = 1.0f;
			if (!raw_pid.is_empty() && p_canon.has(raw_pid)) {
				cpid = p_canon[raw_pid];
				if (cpid != p_id) {
					pr = run(cpid, p_by_id, p_canon, p_children, p_scales, r_cache, depth + 1);
					p_acc = p_scales.has(cpid) ? p_scales[cpid] : 1.0f;
				} else {
					cpid = "";
				}
			}
			String etype = String(e.get("type", "text")).strip_edges().to_lower();
			if (!cpid.is_empty() && p_by_id.has(cpid) && etype != "folder") {
				const Dictionary &pe = p_by_id[cpid];
				String plm = String(pe.get("layout_mode", "free")).strip_edges().to_lower();
				if (plm == "vertical_list" || plm == "horizontal_list" || plm == "grid") {
					const Vector<String> &sibs = p_children.has(cpid) ? p_children[cpid] : Vector<String>();
					int sidx = sibs.find(p_id);
					int cnt = MAX(1, sibs.size());
					if (sidx >= 0 && pr.size.x > 0.5f && pr.size.y > 0.5f) {
						float sp = float(CLAMP(int(pe.get("layout_spacing", 8)), 0, 64)) * p_acc;
						float pad = 8.0f * p_acc;
						float avail_w = MAX(16.0f, pr.size.x - pad * 2.0f);
						float avail_h = MAX(16.0f, pr.size.y - pad * 2.0f);
						int cols = (plm == "vertical_list") ? 1 : ((plm == "horizontal_list") ? cnt : MAX(1, CLAMP(int(pe.get("layout_columns", 2)), 1, 12)));
						int rows = MAX(1, int(Math::ceil(float(cnt) / float(cols))));
						float cw = MAX(16.0f, (avail_w - sp * float(cols - 1)) / float(cols));
						float ch = MAX(16.0f, (avail_h - sp * float(rows - 1)) / float(rows));
						int c = sidx % cols;
						int r = sidx / cols;
						Rect2 out(pr.position.x + pad + float(c) * (cw + sp), pr.position.y + pad + float(r) * (ch + sp), cw, ch);
						r_cache[p_id] = out;
						return out;
					}
				}
			}
			float my_acc = p_scales.has(p_id) ? p_scales[p_id] : _cg_layer_elem_self_scale(e);
			if (etype == "folder" && String(e.get("role", "")) == "pager_page" && !cpid.is_empty() && p_by_id.has(cpid) && String(p_by_id[cpid].get("type", "")).to_lower() == "pager") {
				Dictionary pg_cfg = p_by_id[cpid].has("pager") && p_by_id[cpid]["pager"].get_type() == Variant::DICTIONARY
						? Dictionary(p_by_id[cpid]["pager"])
						: ClockGearsDataManager::make_default_pager_config();
				Rect2 out = ClockGearsDataManager::compute_pager_body_rect(pr, pg_cfg, p_acc);
				r_cache[p_id] = out;
				return out;
			}
			float ref_w = (pr.size.x > 0.5f) ? pr.size.x : LOGICAL_WIDTH;
			float ref_h = (pr.size.y > 0.5f) ? pr.size.y : LOGICAL_HEIGHT;
			float ew = (etype == "folder") ? (float(CLAMP(int(e.get("width", 0)), 0, 960)) * my_acc) : (float(CLAMP(int(e.get("width", 120)), 16, 960)) * my_acc);
			float eh = (etype == "folder") ? (float(CLAMP(int(e.get("height", 0)), 0, 540)) * my_acc) : (float(CLAMP(int(e.get("height", 36)), 16, 540)) * my_acc);
			Vector2 af = compute_anchor_factors(String(e.get("anchor", "top_left")));
			float ox = float(int(e.get("offset_x", 0))) * p_acc;
			float oy = float(int(e.get("offset_y", 0))) * p_acc;
			Rect2 out(pr.position.x + af.x * (ref_w - ew) + ox, pr.position.y + af.y * (ref_h - eh) + oy, ew, eh);
			r_cache[p_id] = out;
			return out;
		}
	};
	HashMap<String, Rect2> scaled_out;
	for (const KeyValue<String, Dictionary> &kv : by_id) {
		Rect2 lr = LogResolver::run(kv.key, by_id, canon, ordered_children, scales, log_cache, 0);
		String etype = String(kv.value.get("type", "text")).strip_edges().to_lower();
		Vector2 pos(Math::round(p_stage_rect.position.x + lr.position.x * p_scale), Math::round(p_stage_rect.position.y + lr.position.y * p_scale));
		Vector2 sz(etype == "folder" ? Math::round(lr.size.x * p_scale) : MAX(4.0f, Math::round(lr.size.x * p_scale)), etype == "folder" ? Math::round(lr.size.y * p_scale) : MAX(4.0f, Math::round(lr.size.y * p_scale)));
		scaled_out[kv.key] = Rect2(pos, sz);
	}
	return scaled_out;
}

Rect2 ClockGearsUiDesignLayer::compute_element_global_rect(const String &p_elem_id, const Array &p_elems, const Rect2 &p_stage_rect, float p_scale) {
	HashMap<String, Rect2> all = compute_all_element_rects(p_elems, p_stage_rect, p_scale);
	if (all.has(p_elem_id)) {
		return all[p_elem_id];
	}
	String low = p_elem_id.strip_edges().to_lower();
	for (const KeyValue<String, Rect2> &kv : all) {
		if (kv.key.to_lower() == low) {
			return kv.value;
		}
	}
	return Rect2();
}

bool ClockGearsUiDesignLayer::is_element_effectively_visible(const String &p_elem_id, const Array &p_elems) {
	HashMap<String, Dictionary> by_id;
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = p_elems[i];
		String id = String(e.get("id", "")).to_lower();
		if (!id.is_empty()) by_id[id] = e;
	}
	String cur = p_elem_id.strip_edges().to_lower();
	HashSet<String> vis;
	while (!cur.is_empty() && by_id.has(cur) && !vis.has(cur)) {
		if (!bool(by_id[cur].get("visible", true))) {
			return false;
		}
		vis.insert(cur);
		cur = String(by_id[cur].get("parent_id", "")).strip_edges().to_lower();
	}
	return true;
}

bool ClockGearsUiDesignLayer::is_element_effectively_locked(const String &p_elem_id, const Array &p_elems) {
	HashMap<String, Dictionary> by_id;
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = p_elems[i];
		String id = String(e.get("id", "")).to_lower();
		if (!id.is_empty()) by_id[id] = e;
	}
	String cur = p_elem_id.strip_edges().to_lower();
	HashSet<String> vis;
	while (!cur.is_empty() && by_id.has(cur) && !vis.has(cur)) {
		if (bool(by_id[cur].get("locked", false))) {
			return true;
		}
		vis.insert(cur);
		cur = String(by_id[cur].get("parent_id", "")).strip_edges().to_lower();
	}
	return false;
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
		for (int c = 0; c < elements[i].data_cell_buttons.size(); c++) {
			Button *cb = elements[i].data_cell_buttons[c];
			if (cb) {
				bool is_empty = bool(cb->get_meta("entry_empty", false));
				bool is_dis = bool(cb->get_meta("entry_disabled", false));
				cb->set_mouse_filter(p_preview ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
				cb->set_focus_mode((p_preview || is_empty || is_dis) ? FOCUS_NONE : FOCUS_ALL);
			}
		}
	}
}

void ClockGearsUiDesignLayer::set_runtime_actor_context(int p_actor_id, int p_equip_slot_idx) {
	if (runtime_actor_id == p_actor_id && runtime_equip_slot_idx == p_equip_slot_idx) {
		return;
	}
	runtime_actor_id = p_actor_id;
	runtime_equip_slot_idx = MAX(0, p_equip_slot_idx);
	refresh_live_values();
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
			memdelete(radial_canvas);
		}
		radial_canvas = nullptr;
	}
}

void ClockGearsUiDesignLayer::clear_elements() {
	_clear_aux_layout_controls();
	// Desacoplar primero para evitar double-free cuando clip_children emparenta hijos bajo otro ElementEntry.
	for (int i = 0; i < elements.size(); i++) {
		Control *r = elements[i].root;
		if (r) {
			if (Node *rp = r->get_parent()) {
				rp->remove_child(r);
			}
		}
	}
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].root) {
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
	if (radial_canvas && !preview_mode) {
		radial_canvas->play_open_anim();
	}
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
	active_focus_container_id = String();
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
	// Aplanar primero cualquier jerarquía visual previa ("clip_children") antes de reutilizar o borrar nodos.
	Vector<ElementEntry> reusable = elements;
	elements.clear();
	for (int r = 0; r < reusable.size(); r++) {
		Control *rr = reusable[r].root;
		if (rr) {
			rr->set_name(vformat("_cg_old_%d", r));
			if (rr->get_parent() && rr->get_parent() != this) {
				rr->get_parent()->remove_child(rr);
				add_child(rr);
			}
		}
	}
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
		radial_canvas->set_data_context(runtime_actor_id, runtime_equip_slot_idx, use_editor_test_data);
		radial_canvas->configure(lay, raw, MAX(0.25f, current_ui_scale), !preview_mode);
	}

	// Identificar carpetas "Plantilla del elemento" dentro de contenedores con fuente de datos
	HashMap<String, Vector<Dictionary>> container_tpl_children;
	HashMap<String, Size2> container_tpl_sizes;
	HashSet<String> tpl_skip_ids;
	{
		HashMap<String, Dictionary> raw_map;
		for (int i = 0; i < raw.size(); i++) {
			if (raw[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = raw[i];
				String did = String(d.get("id", ""));
				if (!did.is_empty()) raw_map[did] = d;
			}
		}
		for (int i = 0; i < raw.size(); i++) {
			if (raw[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary c = raw[i];
			String ctype = String(c.get("type", "")).strip_edges().to_lower();
			String clm = String(c.get("layout_mode", "free")).strip_edges().to_lower();
			String cds = ClockGearsDataManager::normalize_ui_data_source(c.get("data_source", "manual"));
			if (cds == "manual" || (ctype != "container" && !(ctype == "frame" && clm != "free"))) {
				continue;
			}
			String cid = String(c.get("id", ""));
			String tpl_fid = String(c.get("item_template_folder_id", "")).strip_edges();
			if (tpl_fid.is_empty()) {
				for (int j = 0; j < raw.size(); j++) {
					if (raw[j].get_type() != Variant::DICTIONARY) continue;
					Dictionary fd = raw[j];
					if (String(fd.get("type", "")).to_lower() == "folder" && String(fd.get("parent_id", "")) == cid) {
						tpl_fid = String(fd.get("id", ""));
						break;
					}
				}
			}
			if (!tpl_fid.is_empty()) {
				tpl_skip_ids.insert(tpl_fid);
				HashSet<String> folder_tree;
				folder_tree.insert(tpl_fid);
				bool added = true;
				while (added) {
					added = false;
					for (int j = 0; j < raw.size(); j++) {
						if (raw[j].get_type() != Variant::DICTIONARY) continue;
						Dictionary cd = raw[j];
						String ch_id = String(cd.get("id", ""));
						String pid = String(cd.get("parent_id", ""));
						if (!ch_id.is_empty() && !folder_tree.has(ch_id) && folder_tree.has(pid)) {
							folder_tree.insert(ch_id);
							tpl_skip_ids.insert(ch_id);
							added = true;
						}
					}
				}
				Vector<Dictionary> tpl_kids;
				float max_w = 120.0f;
				float max_h = 80.0f;
				for (int j = 0; j < raw.size(); j++) {
					if (raw[j].get_type() != Variant::DICTIONARY) continue;
					Dictionary cd = raw[j];
					String ch_id = String(cd.get("id", ""));
					if (ch_id == tpl_fid || !folder_tree.has(ch_id)) continue;
					if (String(cd.get("type", "")).to_lower() == "folder") continue;
					tpl_kids.push_back(cd.duplicate(true));
					max_w = MAX(max_w, float(int(cd.get("offset_x", 0)) + int(cd.get("width", 64))));
					max_h = MAX(max_h, float(int(cd.get("offset_y", 0)) + int(cd.get("height", 32))));
				}
				container_tpl_children[cid] = tpl_kids;
				container_tpl_sizes[cid] = Size2(max_w, max_h);
			}
		}
	}

	HashSet<String> radial_parent_ids_low;
	for (int i = 0; i < raw.size(); i++) {
		if (raw[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary rd = raw[i];
		if (String(rd.get("type", "")).strip_edges().to_lower() == "radial") {
			String rid = String(rd.get("id", "")).strip_edges().to_lower();
			if (!rid.is_empty()) radial_parent_ids_low.insert(rid);
		}
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
		String pid_low = String(cfg.get("parent_id", "")).strip_edges().to_lower();
		if (etype == "button" && !pid_low.is_empty() && radial_parent_ids_low.has(pid_low)) {
			continue;
		}
		if (etype == "folder" && String(cfg.get("role", "")) != "pager_page") {
			continue;
		}
		if (tpl_skip_ids.has(String(cfg.get("id", "")))) {
			continue;
		}
		ElementEntry entry;
		entry.id = String(cfg.get("id", vformat("elem_%d", i + 1)));
		entry.name = String(cfg.get("name", entry.id));
		entry.type = etype;
		entry.anchor = String(cfg.get("anchor", "top_left"));
		entry.offset_x = int(cfg.get("offset_x", 0));
		entry.offset_y = int(cfg.get("offset_y", 0));
		entry.width = (etype == "folder") ? CLAMP(int(cfg.get("width", 0)), 0, 960) : CLAMP(int(cfg.get("width", 120)), 16, 960);
		entry.height = (etype == "folder") ? CLAMP(int(cfg.get("height", 0)), 0, 540) : CLAMP(int(cfg.get("height", 36)), 16, 540);
		entry.layer_order = int(cfg.get("layer_order", 0));
		entry.visible = is_element_effectively_visible(entry.id, raw);
		entry.parent_id = String(cfg.get("parent_id", "")).strip_edges();
		entry.clip_children = bool(cfg.get("clip_children", false));
		entry.window_style_id = String(cfg.get("window_style_id", "")).strip_edges().to_lower();
		entry.data_binding = String(cfg.get("data_binding", "none")).strip_edges().to_lower();
		entry.widget_kind = String(cfg.get("widget_kind", "")).strip_edges().to_lower();
		entry.action = String(cfg.get("action", "")).strip_edges().to_lower();
		entry.actions = cfg.has("actions") && cfg["actions"].get_type() == Variant::ARRAY ? Array(cfg["actions"]).duplicate(true) : Array();
		entry.focus_order = int(cfg.get("focus_order", i));
		entry.array_order = i;
		entry.tab_index = MAX(0, int(cfg.get("tab_index", 0)));
		entry.config = cfg;
		if (container_tpl_children.has(entry.id)) {
			entry.data_tpl_children = container_tpl_children[entry.id];
			entry.data_tpl_cell_base_size = container_tpl_sizes[entry.id];
		}

		bool has_shape = cfg.has("shape") && cfg["shape"].get_type() == Variant::DICTIONARY;
		bool can_reuse_frame = (etype == "frame" && !entry.clip_children && String(cfg.get("image_path", "")).strip_edges().is_empty());
		bool can_reuse_txt = (etype == "text" && !entry.clip_children && !bool(cfg.get("show_background", false)) && String(cfg.get("window_style_id", "")).strip_edges().is_empty());
		bool reused_ok = false;
		if (!has_shape && (can_reuse_txt || can_reuse_frame)) {
			for (int r = 0; r < reusable.size(); r++) {
				if (!used[r] && reusable[r].root && reusable[r].type == etype && !reusable[r].clip_host && !reusable[r].clip_children && (etype != "frame" || String(reusable[r].config.get("image_path", "")).strip_edges().is_empty())) {
					used.write[r] = true;
					entry.root = reusable[r].root;
					entry.bg_panel = reusable[r].bg_panel;
					entry.content_root = reusable[r].content_root;
					entry.label = reusable[r].label;
					if (entry.root->get_parent() != this) {
						if (Node *old_p = entry.root->get_parent()) {
							old_p->remove_child(entry.root);
						}
						add_child(entry.root);
					}
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
		if (!reusable[r].root) continue;
		if (!preview_mode) {
			for (int k = 0; k < elements.size(); k++) {
				if (elements[k].id == reusable[r].id && reusable[r].runtime_vis_override_set) {
					elements.write[k].runtime_vis_override_set = true;
					elements.write[k].runtime_vis_override = reusable[r].runtime_vis_override;
					break;
				}
			}
		}
		if (!used[r]) {
			if (Node *rp = reusable[r].root->get_parent()) {
				rp->remove_child(reusable[r].root);
			}
			memdelete(reusable[r].root);
		}
	}

	// Resolver recorte opcional al padre ("clip_children") y páginas de paginador ("pager_page"):
	// Cada pager_page se emparenta dentro de pager_pages_clip; sus hijos dentro de la página.
	HashMap<String, Dictionary> raw_by_id;
	HashMap<String, Control *> entry_root_by_id;
	HashMap<String, Control *> pager_clip_by_id;
	for (int i = 0; i < raw.size(); i++) {
		if (raw[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = raw[i];
			raw_by_id[String(d.get("id", ""))] = d;
		}
	}
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (e.root) {
			e.root->set_clip_contents(e.clip_children);
			entry_root_by_id[e.id] = e.root;
		}
		if (e.type == "pager" && e.pager_pages_clip) {
			pager_clip_by_id[e.id] = e.pager_pages_clip;
			for (int r = 0; r < reusable.size(); r++) {
				if (reusable[r].type == "pager" && reusable[r].id == e.id) {
					if (!preview_mode) {
						e.pager_active_idx = CLAMP(reusable[r].pager_active_idx, 0, MAX(0, e.pager_page_ids.size() - 1));
					}
					e.pager_page_focus_elem = reusable[r].pager_page_focus_elem;
					e.pager_page_scroll = reusable[r].pager_page_scroll;
					break;
				}
			}
		}
	}
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) continue;
		if (e.type == "folder" && String(e.config.get("role", "")) == "pager_page" && !e.parent_id.is_empty() && pager_clip_by_id.has(e.parent_id)) {
			e.clip_ancestor_id = e.parent_id;
			Control *want_parent = pager_clip_by_id[e.parent_id];
			if (e.root->get_parent() != want_parent) {
				if (Node *op = e.root->get_parent()) {
					op->remove_child(e.root);
				}
				want_parent->add_child(e.root);
			}
			e.root->set_z_as_relative(true);
			e.root->set_z_index(0);
			continue;
		}
		String clip_anc;
		String cur = e.parent_id;
		HashSet<String> vis;
		while (!cur.is_empty() && raw_by_id.has(cur) && !vis.has(cur)) {
			vis.insert(cur);
			const Dictionary &ad = raw_by_id[cur];
			String at = String(ad.get("type", ""));
			bool is_pg_page = (at == "folder" && String(ad.get("role", "")) == "pager_page");
			if ((is_pg_page || (bool(ad.get("clip_children", false)) && at != "folder")) && entry_root_by_id.has(cur)) {
				clip_anc = cur;
				break;
			}
			cur = String(ad.get("parent_id", "")).strip_edges();
		}
		e.clip_ancestor_id = clip_anc;
		Control *want_parent = (!clip_anc.is_empty() && entry_root_by_id.has(clip_anc)) ? entry_root_by_id[clip_anc] : this;
		if (e.root->get_parent() != want_parent) {
			if (Node *op = e.root->get_parent()) {
				op->remove_child(e.root);
			}
			want_parent->add_child(e.root);
		}
		e.root->set_z_as_relative(want_parent == this);
		e.root->set_z_index(e.layer_order);
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
	root->set_meta("ui_elem_id", r_entry.id);
	const bool allow_pass = (r_entry.type == "button") || (r_entry.type == "widget" && !preview_mode);
	root->set_mouse_filter(allow_pass ? MOUSE_FILTER_PASS : MOUSE_FILTER_IGNORE);
	root->set_z_index(r_entry.layer_order);
	root->set_visible(r_entry.visible);
	add_child(root);
	r_entry.root = root;

	auto resolve_inherited_int_scale = [&]() -> bool {
		if (bool(r_entry.config.get("manual_integer_scale", false)) || bool(r_entry.config.get("integer_scale", false))) {
			return true;
		}
		Array all_els = (active_design.has("elements") && active_design["elements"].get_type() == Variant::ARRAY)
				? Array(active_design["elements"])
				: Array();
		String cur_pid = r_entry.parent_id;
		int hops = 0;
		while (!cur_pid.is_empty() && hops++ < 16) {
			bool found = false;
			for (int ei = 0; ei < all_els.size(); ei++) {
				if (all_els[ei].get_type() != Variant::DICTIONARY) continue;
				Dictionary ed = all_els[ei];
				if (String(ed.get("id", "")) == cur_pid) {
					if (bool(ed.get("integer_scale", false)) || bool(ed.get("manual_integer_scale", false))) {
						return true;
					}
					cur_pid = String(ed.get("parent_id", "")).strip_edges();
					found = true;
					break;
				}
			}
			if (!found) break;
		}
		return false;
	};

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

	bool pa = bool(r_entry.config.get("pixel_art", true));
	bool int_sc = resolve_inherited_int_scale();
	String elem_sm = String(r_entry.config.get("scale_mode", "stretch")).strip_edges().to_lower();
	TextureFilter tf = (pa || elem_sm == "integer" || int_sc) ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR;
	if (int_sc) {
		root->set_texture_filter(TEXTURE_FILTER_NEAREST);
	}

	if (r_entry.type == "frame") {
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
					content->add_child(np);
				} else {
					content->set_clip_contents(true);
					TextureRect *fimg = memnew(TextureRect);
					fimg->set_name("FrameFillImage");
					fimg->set_mouse_filter(MOUSE_FILTER_IGNORE);
					fimg->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
					fimg->set_texture_filter(tf);
					fimg->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
					fimg->set_stretch_mode(sm == "fit" ? TextureRect::STRETCH_KEEP_ASPECT_CENTERED : (sm == "fill" ? TextureRect::STRETCH_KEEP_ASPECT_COVERED : (sm == "tile" ? TextureRect::STRETCH_TILE : TextureRect::STRETCH_SCALE)));
					fimg->set_texture(ftex);
					content->add_child(fimg);
					r_entry.image_rect = fimg;
				}
			}
		}
	} else if (r_entry.type == "button") {
		String bsm = String(r_entry.config.get("scale_mode", "stretch")).strip_edges().to_lower();
		String img_norm = String(r_entry.config.get("image_normal", "")).strip_edges();
		String img_hov = String(r_entry.config.get("image_hover", "")).strip_edges();
		String img_prs = String(r_entry.config.get("image_pressed", "")).strip_edges();
		String img_dis = String(r_entry.config.get("image_disabled", "")).strip_edges();
		String fb_img = String(r_entry.config.get("image_path", "")).strip_edges();
		bool use_manual_img = (bsm == "manual") && (!img_norm.is_empty() || !img_hov.is_empty() || !img_prs.is_empty() || !img_dis.is_empty() || !fb_img.is_empty());

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

		if (use_manual_img) {
			content->set_clip_contents(true);
			UiButtonManualImageHost *bimg = memnew(UiButtonManualImageHost);
			bimg->set_name("ManualButtonImg");
			bimg->set_mouse_filter(MOUSE_FILTER_IGNORE);
			bimg->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			bimg->configure(
					btn,
					r_entry.config,
					img_norm.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(img_norm),
					img_hov.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(img_hov),
					img_prs.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(img_prs),
					img_dis.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(img_dis),
					fb_img.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(fb_img));
			content->add_child(bimg);
			r_entry.manual_btn_img_host = bimg;
			btn->connect("mouse_entered", callable_mp((CanvasItem *)bimg, &CanvasItem::queue_redraw));
			btn->connect("mouse_exited", callable_mp((CanvasItem *)bimg, &CanvasItem::queue_redraw));
			btn->connect("button_down", callable_mp((CanvasItem *)bimg, &CanvasItem::queue_redraw));
			btn->connect("button_up", callable_mp((CanvasItem *)bimg, &CanvasItem::queue_redraw));
			btn->connect("focus_entered", callable_mp((CanvasItem *)bimg, &CanvasItem::queue_redraw));
			btn->connect("focus_exited", callable_mp((CanvasItem *)bimg, &CanvasItem::queue_redraw));
		}

		String ip = String(r_entry.config.get("icon_path", use_manual_img ? String() : String(r_entry.config.get("image_path", "")))).strip_edges();
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
			if (tex.is_valid()) np->set_texture(tex);
			content->add_child(np);
		} else {
			content->set_clip_contents(true);
			TextureRect *img = memnew(TextureRect);
			img->set_name("Image");
			img->set_mouse_filter(MOUSE_FILTER_IGNORE);
			img->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			img->set_texture_filter(tf);
			if (sm == "stretch" || sm == "manual") {
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
			if (tex.is_valid()) {
				img->set_texture(tex);
			}
			if (bool(r_entry.config.get("rotatable", false)) && !preview_mode) {
				img->set_mouse_filter(MOUSE_FILTER_STOP);
				img->connect("gui_input", callable_mp(this, &ClockGearsUiDesignLayer::_on_rotatable_preview_gui_input).bind(r_entry.id));
			}
			content->add_child(img);
			r_entry.image_rect = img;
		}
	} else if (r_entry.type == "container") {
		ScrollContainer *dsc = memnew(ScrollContainer);
		dsc->set_name("DataContainerScroll");
		dsc->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		dsc->set_clip_contents(true);
		dsc->set_mouse_filter(MOUSE_FILTER_PASS);
		content->add_child(dsc);
		r_entry.data_scroll = dsc;

		Control *dcan = memnew(Control);
		dcan->set_name("DataItemsCanvas");
		dcan->set_mouse_filter(MOUSE_FILTER_PASS);
		dcan->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dsc->add_child(dcan);
		r_entry.data_items_canvas = dcan;
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
			Ref<StyleBoxEmpty> transp_sb;
			transp_sb.instantiate();
			transp_sb->set_content_margin_all(4.0f);
			card->add_theme_style_override("panel", transp_sb);
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
		ClockGearsRadialCanvas *rc = memnew(ClockGearsRadialCanvas);
		rc->set_name("ElemRadialCanvas");
		rc->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		rc->connect("slot_activated", callable_mp(this, &ClockGearsUiDesignLayer::_on_radial_slot_activated));
		rc->connect("selection_changed", callable_mp(this, &ClockGearsUiDesignLayer::_on_radial_elem_selection_changed).bind(r_entry.id));
		Dictionary rlay = (r_entry.config.has("layout") && r_entry.config["layout"].get_type() == Variant::DICTIONARY)
				? Dictionary(r_entry.config["layout"]).duplicate(true)
				: Dictionary();
		rlay["mode"] = "radial";
		rlay["_is_element"] = true;
		String ds = ClockGearsDataManager::normalize_ui_data_source(r_entry.config.get("data_source", rlay.get("data_source", "manual")));
		rlay["data_source"] = ds;
		static const char *k_copy_keys[] = {
			"data_filter_category", "data_only_usable", "data_sort_by", "data_equip_slot",
			"data_show_empty", "data_empty_count", "slot_action", "slot_actions", "pixel_art"
		};
		for (int ck = 0; ck < 9; ck++) {
			if (r_entry.config.has(k_copy_keys[ck]) && !rlay.has(k_copy_keys[ck])) {
				rlay[k_copy_keys[ck]] = r_entry.config[k_copy_keys[ck]];
			}
		}
		Array child_btns;
		Array all_raw = active_design.get("elements", Array());
		String rid_low = r_entry.id.to_lower();
		for (int bi = 0; bi < all_raw.size(); bi++) {
			if (all_raw[bi].get_type() != Variant::DICTIONARY) continue;
			Dictionary bd = all_raw[bi];
			if (String(bd.get("type", "")).strip_edges().to_lower() == "button" &&
					String(bd.get("parent_id", "")).strip_edges().to_lower() == rid_low) {
				child_btns.push_back(bd);
			}
		}
		if (ds == "manual" && child_btns.is_empty() && (!rlay.has("radial_slots") || Array(rlay["radial_slots"]).is_empty())) {
			rlay["radial_slots"] = ClockGearsDataManager::make_default_radial_slots(6, screen_id);
		} else if (ds != "manual") {
			rlay["radial_slots"] = Array();
		}
		rlay["radial_center_x"] = 480;
		rlay["radial_center_y"] = 270;
		content->add_child(rc);
		rc->set_data_context(runtime_actor_id, runtime_equip_slot_idx, use_editor_test_data);
		rc->configure(rlay, child_btns, MAX(0.25f, current_ui_scale), !preview_mode);
		if (!radial_canvas) {
			radial_canvas = rc;
		}
	} else if (r_entry.type == "folder") {
		if (r_entry.bg_panel) {
			r_entry.bg_panel->set_visible(false);
		}
	} else if (r_entry.type == "pager") {
		content->set_mouse_filter(MOUSE_FILTER_PASS);
		content->connect("gui_input", callable_mp(this, &ClockGearsUiDesignLayer::_on_pager_root_gui_input).bind(r_entry.id));

		Control *ind_host = memnew(Control);
		ind_host->set_name("PagerIndicatorHost");
		ind_host->set_mouse_filter(MOUSE_FILTER_PASS);
		ind_host->set_clip_contents(true);
		content->add_child(ind_host);
		r_entry.pager_indicator_host = ind_host;

		Control *clip_pages = memnew(Control);
		clip_pages->set_name("PagerPagesClip");
		clip_pages->set_mouse_filter(MOUSE_FILTER_IGNORE);
		clip_pages->set_clip_contents(true);
		content->add_child(clip_pages);
		r_entry.pager_pages_clip = clip_pages;

		r_entry.pager_page_ids.clear();
		Array all_raw = active_design.get("elements", Array());
		String pid_low = r_entry.id.to_lower();
		for (int pi = 0; pi < all_raw.size(); pi++) {
			if (all_raw[pi].get_type() != Variant::DICTIONARY) continue;
			Dictionary pd = all_raw[pi];
			if (String(pd.get("parent_id", "")).strip_edges().to_lower() == pid_low &&
					String(pd.get("role", "")) == "pager_page") {
				r_entry.pager_page_ids.push_back(String(pd.get("id", "")));
			}
		}
		Dictionary pg_cfg = r_entry.config.has("pager") && r_entry.config["pager"].get_type() == Variant::DICTIONARY
				? Dictionary(r_entry.config["pager"])
				: ClockGearsDataManager::make_default_pager_config();
		int init_idx = preview_mode ? int(pg_cfg.get("active_editor_page", 0)) : int(pg_cfg.get("default_page", 0));
		r_entry.pager_active_idx = r_entry.pager_page_ids.is_empty() ? 0 : CLAMP(init_idx, 0, r_entry.pager_page_ids.size() - 1);
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

void ClockGearsUiDesignLayer::_update_element_image_rect(ElementEntry &r_entry, const Size2 &p_target_sz) {
	if (!r_entry.image_rect) {
		return;
	}
	Ref<Texture2D> it = r_entry.image_rect->get_texture();
	if (!it.is_valid()) {
		return;
	}
	Vector2 ts = it->get_size();
	if (ts.x <= 0.0f || ts.y <= 0.0f) {
		return;
	}
	String sm = String(r_entry.config.get("scale_mode", "")).strip_edges().to_lower();
	if (sm == "integer") {
		int mult = MAX(1, int(Math::floor(MIN(p_target_sz.x / ts.x, p_target_sz.y / ts.y))));
		Vector2 dsz = ts * float(mult);
		r_entry.image_rect->set_anchors_preset(PRESET_TOP_LEFT);
		r_entry.image_rect->set_position(((p_target_sz - dsz) * 0.5f).floor());
		r_entry.image_rect->set_size(dsz);
	} else if (sm == "manual") {
		Rect2 dr = compute_manual_image_rect(Rect2(Vector2(), p_target_sz), ts, r_entry.config);
		r_entry.image_rect->set_anchors_preset(PRESET_TOP_LEFT);
		r_entry.image_rect->set_position(dr.position);
		r_entry.image_rect->set_size(dr.size);
	}
}

void ClockGearsUiDesignLayer::_apply_element_style(ElementEntry &r_entry, float p_scale) {
	if (!r_entry.bg_panel || !r_entry.content_root || !r_entry.root) {
		return;
	}
	float elem_op = CLAMP(float(r_entry.config.get("opacity", 1.0)), 0.0f, 1.0f);
	r_entry.root->set_modulate(Color(1.0f, 1.0f, 1.0f, elem_op));
	const bool has_shape = r_entry.config.has("shape") && r_entry.config["shape"].get_type() == Variant::DICTIONARY;
	Dictionary shape_cfg = has_shape ? Dictionary(r_entry.config["shape"]) : Dictionary();
	auto merge_shape_into = [&](Dictionary &r_target) {
		Array keys = shape_cfg.keys();
		for (int k = 0; k < keys.size(); k++) {
			r_target[keys[k]] = shape_cfg[keys[k]];
		}
		String st_str = String(shape_cfg.get("shape_type", "")).strip_edges().to_lower();
		if (st_str == "rect") {
			r_target["border_type"] = "rect";
		} else if (st_str == "rounded" || st_str == "pill" || st_str == "circle" || st_str == "ellipse") {
			r_target["border_type"] = "rounded";
		}
	};

	if (r_entry.clip_host && has_shape) {
		r_entry.bg_panel->set_visible(false);
		r_entry.clip_host->set_position(Vector2());
		r_entry.clip_host->set_size(r_entry.root->get_size());
		bool draw_fill = (r_entry.type != "image") || !r_entry.window_style_id.is_empty() || bool(r_entry.config.get("show_background", false));
		r_entry.clip_host->configure(shape_cfg, p_scale, draw_fill);
		if (r_entry.border_overlay) {
			r_entry.border_overlay->set_position(Vector2());
			r_entry.border_overlay->set_size(r_entry.root->get_size());
			r_entry.border_overlay->configure(shape_cfg, p_scale);
		}
		r_entry.content_root->set_position(Vector2());
		r_entry.content_root->set_size(r_entry.root->get_size());
		_update_element_image_rect(r_entry, r_entry.root->get_size());
		if (r_entry.manual_btn_img_host) {
			r_entry.manual_btn_img_host->queue_redraw();
		}
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
		if (!r_entry.window_style_id.is_empty() && r_entry.window_style_id != "default") {
			if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
				b_over = dm->get_window_style(r_entry.window_style_id);
			}
		}
		if (has_shape) {
			merge_shape_into(b_over);
		}
		ClockGearsWindowStyleManager::apply_button_style(r_entry.button, window_type_fallback, b_over);
		String img_norm = String(r_entry.config.get("image_normal", "")).strip_edges();
		String img_hov = String(r_entry.config.get("image_hover", "")).strip_edges();
		String img_prs = String(r_entry.config.get("image_pressed", "")).strip_edges();
		String img_dis = String(r_entry.config.get("image_disabled", "")).strip_edges();
		if (r_entry.manual_btn_img_host) {
			Ref<StyleBoxEmpty> empty_sb;
			empty_sb.instantiate();
			r_entry.button->add_theme_style_override("normal", empty_sb);
			r_entry.button->add_theme_style_override("hover", empty_sb);
			r_entry.button->add_theme_style_override("pressed", empty_sb);
			r_entry.button->add_theme_style_override("hover_pressed", empty_sb);
			r_entry.button->add_theme_style_override("focus", empty_sb);
			r_entry.button->add_theme_style_override("disabled", empty_sb);
			r_entry.manual_btn_img_host->queue_redraw();
		} else if (!img_norm.is_empty() || !img_hov.is_empty() || !img_prs.is_empty() || !img_dis.is_empty()) {
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
	bool force_panel = (r_entry.type == "frame" || r_entry.type == "widget" || r_entry.type == "container") || has_shape || bool(r_entry.config.get("show_background", false));
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
		_update_element_image_rect(r_entry, rsz);
		if (r_entry.type == "container") {
			_build_or_refresh_data_container(r_entry, p_scale, true);
		}
		return;
	}

	r_entry.bg_panel->set_visible(true);
	Dictionary surface_overrides;
	if (!sid.is_empty() && sid != "default") {
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Dictionary specific = dm->get_window_style(sid);
			if (!specific.is_empty()) {
				surface_overrides = specific;
			}
		}
	}
	if (has_shape) {
		merge_shape_into(surface_overrides);
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
	_update_element_image_rect(r_entry, csz);
	if (r_entry.type == "container") {
		_build_or_refresh_data_container(r_entry, p_scale, true);
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
			radial_canvas->set_data_context(runtime_actor_id, runtime_equip_slot_idx, use_editor_test_data);
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

	Array raw = active_design.get("elements", Array());
	HashMap<String, float> acc_scales = compute_all_element_scales(raw);
	HashMap<String, Rect2> all_rects = compute_all_element_rects(raw, current_stage_rect, current_ui_scale);
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) {
			continue;
		}
		float e_acc = acc_scales.has(e.id) ? acc_scales[e.id] : _cg_layer_elem_self_scale(e.config);
		float eff_scale = current_ui_scale * e_acc;
		Rect2 er = all_rects.has(e.id) ? all_rects[e.id] : compute_element_rect(e.config, current_stage_rect, current_ui_scale);
		bool is_pager_page = (e.type == "folder" && String(e.config.get("role", "")) == "pager_page");
		Vector2 local_pos = er.position;
		if (is_pager_page) {
			local_pos = Vector2();
		} else if (!e.clip_ancestor_id.is_empty() && all_rects.has(e.clip_ancestor_id)) {
			local_pos = er.position - all_rects[e.clip_ancestor_id].position;
		}
		bool hold_page_pose = e.container_trans_active;
		if (is_pager_page) {
			for (int pi = 0; pi < elements.size(); pi++) {
				if (elements[pi].type == "pager" && elements[pi].id == e.parent_id && elements[pi].pager_trans_active) {
					hold_page_pose = true;
					break;
				}
			}
		}
		e.container_base_pos = local_pos;
		if (!hold_page_pose && e.root->get_position() != local_pos) {
			e.root->set_position(local_pos);
		}
		if (e.root->get_size() != er.size) {
			e.root->set_size(er.size);
		}
		bool eff_vis = is_element_effectively_visible(e.id, raw);
		if (!preview_mode && e.runtime_vis_override_set) {
			eff_vis = e.runtime_vis_override;
		}
		e.visible = eff_vis;
		if (!is_pager_page && !e.container_trans_active && e.root->is_visible() != eff_vis) {
			e.root->set_visible(eff_vis);
		}
		if (e.root->get_z_index() != e.layer_order) {
			e.root->set_z_index(e.layer_order);
		}
		_apply_element_style(e, eff_scale);

		String fp = String(e.config.get("font_path", "")).strip_edges();
		Ref<Font> cfont = !fp.is_empty() ? _load_cached_font(fp) : Ref<Font>();

		if (e.label) {
			int fsz = MAX(8, int(Math::round(float(e.config.get("font_size", 16)) * eff_scale)));
			if (!e.label->has_theme_font_size_override("font_size") || e.label->get_theme_font_size("font_size") != fsz) {
				e.label->add_theme_font_size_override("font_size", fsz);
			}
			if (cfont.is_valid()) {
				e.label->add_theme_font_override("font", cfont);
			} else if (e.label->has_theme_font_override("font")) {
				e.label->remove_theme_font_override("font");
			}
			if (e.config.has("font_color")) {
				e.label->add_theme_color_override("font_color", _cg_ui_parse_color(e.config["font_color"], Color(1, 1, 1, 1)));
			}
		}
		if (e.button) {
			if (cfont.is_valid()) {
				e.button->add_theme_font_override("font", cfont);
			} else if (e.button->has_theme_font_override("font")) {
				e.button->remove_theme_font_override("font");
			}
			if (e.config.has("font_color")) {
				Color fc = _cg_ui_parse_color(e.config["font_color"], Color(1, 1, 1, 1));
				e.button->add_theme_color_override("font_color", fc);
				e.button->add_theme_color_override("font_hover_color", fc);
				e.button->add_theme_color_override("font_focus_color", fc);
			}
		}
		if (e.bar_bg_rect) {
			e.bar_bg_rect->set_color(_cg_ui_parse_color(e.config.get("bg_color", "#1A1E2ECC"), Color(0.1f, 0.12f, 0.18f, 0.8f)));
		}
		if (e.bar_fill_rect) {
			e.bar_fill_rect->set_color(_cg_ui_parse_color(e.config.get("fill_color", "#D93B3BFF"), Color(0.85f, 0.23f, 0.23f, 1.0f)));
		}
		if (e.bar_label) {
			int fsz = MAX(8, int(Math::round(float(e.config.get("font_size", 13)) * eff_scale)));
			e.bar_label->add_theme_font_size_override("font_size", fsz);
			if (cfont.is_valid()) {
				e.bar_label->add_theme_font_override("font", cfont);
			} else if (e.bar_label->has_theme_font_override("font")) {
				e.bar_label->remove_theme_font_override("font");
			}
			if (e.config.has("font_color")) {
				e.bar_label->add_theme_color_override("font_color", _cg_ui_parse_color(e.config["font_color"], Color(1, 1, 1, 1)));
			}
		}
		if (e.icon_rect) {
			Size2 csz = e.content_root ? e.content_root->get_size() : er.size;
			float icon_sz = CLAMP(csz.y - 4.0f, 8.0f, 160.0f);
			e.icon_rect->set_custom_minimum_size(Size2(icon_sz, icon_sz));
		}
		if (e.type == "radial" && e.content_root) {
			if (ClockGearsRadialCanvas *rc = Object::cast_to<ClockGearsRadialCanvas>(e.content_root->find_child("ElemRadialCanvas", false, false))) {
				Dictionary rlay = (e.config.has("layout") && e.config["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(e.config["layout"]).duplicate(true) : Dictionary();
				rlay["mode"] = "radial";
				rlay["_is_element"] = true;
				rlay["radial_center_x"] = 480;
				rlay["radial_center_y"] = 270;
				rlay["data_source"] = ClockGearsDataManager::normalize_ui_data_source(e.config.get("data_source", rlay.get("data_source", "manual")));
				static const char *k_copy_keys_r[] = { "data_filter_category", "data_only_usable", "data_sort_by", "data_equip_slot", "data_show_empty", "data_empty_count", "slot_action", "slot_actions", "pixel_art" };
				for (int ck = 0; ck < 9; ck++) {
					if (e.config.has(k_copy_keys_r[ck]) && !rlay.has(k_copy_keys_r[ck])) rlay[k_copy_keys_r[ck]] = e.config[k_copy_keys_r[ck]];
				}
				Array cbtns;
				String rid_l = e.id.to_lower();
				for (int bi = 0; bi < raw.size(); bi++) {
					if (raw[bi].get_type() != Variant::DICTIONARY) continue;
					Dictionary bd = raw[bi];
					if (String(bd.get("type", "")).to_lower() == "button" && String(bd.get("parent_id", "")).to_lower() == rid_l) {
						cbtns.push_back(bd);
					}
				}
				if (String(rlay["data_source"]) == "manual" && cbtns.is_empty() && (!rlay.has("radial_slots") || Array(rlay["radial_slots"]).is_empty())) {
					rlay["radial_slots"] = ClockGearsDataManager::make_default_radial_slots(6, screen_id);
				} else if (String(rlay["data_source"]) != "manual") {
					rlay["radial_slots"] = Array();
				}
				rc->set_data_context(runtime_actor_id, runtime_equip_slot_idx, use_editor_test_data);
				rc->configure(rlay, cbtns, MAX(0.20f, eff_scale), !preview_mode);
			}
		} else if (e.type == "pager" && e.content_root) {
			Dictionary pg_cfg = e.config.has("pager") && e.config["pager"].get_type() == Variant::DICTIONARY
					? Dictionary(e.config["pager"])
					: ClockGearsDataManager::make_default_pager_config();
			Rect2 local_full(Vector2(), e.content_root->get_size());
			Rect2 body_r = ClockGearsDataManager::compute_pager_body_rect(local_full, pg_cfg, eff_scale);
			if (e.pager_pages_clip) {
				e.pager_pages_clip->set_position(body_r.position);
				e.pager_pages_clip->set_size(body_r.size);
			}
			String im = String(pg_cfg.get("indicator_mode", "tabs")).strip_edges().to_lower();
			String ip = String(pg_cfg.get("indicator_pos", "top")).strip_edges().to_lower();
			if (e.pager_indicator_host) {
				if (im == "none") {
					e.pager_indicator_host->set_visible(false);
				} else {
					e.pager_indicator_host->set_visible(true);
					Rect2 ir;
					if (ip == "top") {
						ir = Rect2(0, 0, local_full.size.x, MAX(0.0f, body_r.position.y));
					} else if (ip == "bottom") {
						float by = body_r.position.y + body_r.size.y;
						ir = Rect2(0, by, local_full.size.x, MAX(0.0f, local_full.size.y - by));
					} else if (ip == "left") {
						ir = Rect2(0, 0, MAX(0.0f, body_r.position.x), local_full.size.y);
					} else {
						float bx = body_r.position.x + body_r.size.x;
						ir = Rect2(bx, 0, MAX(0.0f, local_full.size.x - bx), local_full.size.y);
					}
					e.pager_indicator_host->set_position(ir.position);
					e.pager_indicator_host->set_size(ir.size);
					_rebuild_pager_indicator(e, eff_scale);
				}
			}
			_sync_pager_pages_layout_and_vis(e);
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
	Array raw = active_design.has("elements") ? Array(active_design["elements"]) : Array();
	bool found_raw = false;
	for (int i = 0; i < raw.size(); i++) {
		if (raw[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary re = raw[i];
		if (String(re.get("id", "")).to_lower() == target) {
			if (bool(re.get("clip_children", false)) != bool(p_elem.get("clip_children", false)) ||
					int(re.get("scale_percent", 100)) != int(p_elem.get("scale_percent", 100)) ||
					bool(re.get("integer_scale", false)) != bool(p_elem.get("integer_scale", false))) {
				return false;
			}
			raw[i] = p_elem.duplicate(true);
			found_raw = true;
			break;
		}
	}
	if (!found_raw) {
		return false;
	}
	active_design["elements"] = raw;
	HashMap<String, Rect2> all_rects = compute_all_element_rects(raw, current_stage_rect, current_ui_scale);
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		bool is_target = (e.id.to_lower() == target);
		if (is_target) {
			if ((e.config.has("shape") && e.config["shape"].get_type() == Variant::DICTIONARY) != (p_elem.has("shape") && p_elem["shape"].get_type() == Variant::DICTIONARY)) {
				return false;
			}
			bool need_style = (e.config.get("shape", Variant()) != p_elem.get("shape", Variant()) ||
					String(e.config.get("window_style_id", "")) != String(p_elem.get("window_style_id", "")) ||
					String(e.config.get("fill_color", "")) != String(p_elem.get("fill_color", "")) ||
					float(e.config.get("opacity", 1.0)) != float(p_elem.get("opacity", 1.0)) ||
					bool(e.config.get("show_background", false)) != bool(p_elem.get("show_background", false)));
			e.config = p_elem.duplicate(true);
			e.window_style_id = String(p_elem.get("window_style_id", "")).strip_edges().to_lower();
			e.anchor = String(p_elem.get("anchor", e.anchor));
			e.offset_x = int(p_elem.get("offset_x", e.offset_x));
			e.offset_y = int(p_elem.get("offset_y", e.offset_y));
			e.width = CLAMP(int(p_elem.get("width", e.width)), 16, 960);
			e.height = CLAMP(int(p_elem.get("height", e.height)), 16, 540);
			e.layer_order = int(p_elem.get("layer_order", e.layer_order));
			e.clip_children = bool(p_elem.get("clip_children", false));
			if (e.root && need_style) {
				_apply_element_style(e, current_ui_scale);
			}
		}
		if (e.root && all_rects.has(e.id)) {
			bool is_pager_page = (e.type == "folder" && String(e.config.get("role", "")) == "pager_page");
			Rect2 er = all_rects[e.id];
			Vector2 lpos = er.position;
			if (is_pager_page) {
				lpos = Vector2();
			} else if (!e.clip_ancestor_id.is_empty() && all_rects.has(e.clip_ancestor_id)) {
				lpos = er.position - all_rects[e.clip_ancestor_id].position;
			}
			if (!is_pager_page) {
				e.root->set_position(lpos);
			}
			e.root->set_size(er.size);
			e.root->set_z_index(e.layer_order);
			bool eff_vis = is_element_effectively_visible(e.id, raw);
			e.visible = eff_vis;
			if (!is_pager_page) {
				e.root->set_visible(eff_vis);
			}
			if (is_target && e.type == "radial" && e.content_root) {
				if (ClockGearsRadialCanvas *rc = Object::cast_to<ClockGearsRadialCanvas>(e.content_root->find_child("ElemRadialCanvas", false, false))) {
					Dictionary rlay = (e.config.has("layout") && e.config["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(e.config["layout"]).duplicate(true) : Dictionary();
					rlay["mode"] = "radial";
					rlay["_is_element"] = true;
					rlay["radial_center_x"] = 480;
					rlay["radial_center_y"] = 270;
					rlay["data_source"] = ClockGearsDataManager::normalize_ui_data_source(e.config.get("data_source", rlay.get("data_source", "manual")));
					static const char *k_copy_keys_f[] = { "data_filter_category", "data_only_usable", "data_sort_by", "data_equip_slot", "data_show_empty", "data_empty_count", "slot_action", "slot_actions", "pixel_art" };
					for (int ck = 0; ck < 9; ck++) {
						if (e.config.has(k_copy_keys_f[ck]) && !rlay.has(k_copy_keys_f[ck])) rlay[k_copy_keys_f[ck]] = e.config[k_copy_keys_f[ck]];
					}
					Array cbtns;
					String rid_l = e.id.to_lower();
					for (int bi = 0; bi < raw.size(); bi++) {
						if (raw[bi].get_type() != Variant::DICTIONARY) continue;
						Dictionary bd = raw[bi];
						if (String(bd.get("type", "")).to_lower() == "button" && String(bd.get("parent_id", "")).to_lower() == rid_l) {
							cbtns.push_back(bd);
						}
					}
					rc->set_data_context(runtime_actor_id, runtime_equip_slot_idx, use_editor_test_data);
					rc->configure(rlay, cbtns, current_ui_scale, !preview_mode);
				}
			}
		}
	}
	return true;
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
				String b = e.data_binding;
				String nt;
				if (dm && (b == "item_name" || b == "item_desc" || b == "item_price" || b == "item_type" || b == "item_params" || (b == "item_qty" && String(e.config.get("item_key", "")).is_empty()))) {
					nt = dm->resolve_ui_entry_field_binding(b, last_focused_data_entry, e.config, (preview_mode || use_editor_test_data));
				} else {
					nt = resolve_element_text(e.config, dm);
				}
				if (e.label->get_text() != nt) {
					e.label->set_text(nt);
				}
			}
		} else if (e.type == "image" && e.image_rect && e.data_binding == "item_icon") {
			Ref<Texture2D> ic_tex;
			if (dm && !last_focused_data_entry.is_empty() && !bool(last_focused_data_entry.get("is_empty", false))) {
				ic_tex = dm->get_entry_icon_texture(last_focused_data_entry, MAX(32, int(e.width * current_ui_scale)));
			}
			e.image_rect->set_texture(ic_tex);
			if (bool(e.config.get("rotatable", false))) {
				e.image_rect->set_pivot_offset(e.image_rect->get_size() * 0.5f);
				e.image_rect->set_rotation_degrees(e.rotatable_angle_deg);
			}
		} else if (e.type == "container") {
			String cds = ClockGearsDataManager::normalize_ui_data_source(e.config.get("data_source", "manual"));
			if (cds != "manual") {
				_build_or_refresh_data_container(e, current_ui_scale, false);
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

		bool is_pager_page = (e.type == "folder" && String(e.config.get("role", "")) == "pager_page");
		if (!is_pager_page && !e.container_trans_active) {
			if (e.runtime_vis_override_set && !preview_mode) {
				vis = e.runtime_vis_override;
			}
			e.root->set_visible(vis);
		}
	}
	_update_container_focus_visuals();
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
	active_focus_container_id = p_elem_id;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_elem_id && !elements[i].parent_id.is_empty()) {
			String pid = elements[i].parent_id;
			for (int j = 0; j < elements.size(); j++) {
				if (elements[j].id == pid && (elements[j].type == "container" || elements[j].type == "radial")) {
					active_focus_container_id = pid;
					break;
				}
			}
			break;
		}
	}
	_on_button_hover(p_elem_id);
	_update_container_focus_visuals();
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
	if (!radial_canvas) {
		return;
	}
	Dictionary info = radial_canvas->get_selected_slot_info();
	if (!info.is_empty()) {
		Dictionary entry;
		entry["key"] = info.get("id", "");
		entry["name"] = info.get("title", "");
		entry["description"] = info.get("description", "");
		entry["icon_path"] = info.get("icon_path", "");
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm) {
			Array inv = dm->get_inventory_entries();
			for (int i = 0; i < inv.size(); i++) {
				if (String(Dictionary(inv[i]).get("key", "")) == String(info.get("id", ""))) {
					entry = Dictionary(inv[i]).duplicate(true);
					break;
				}
			}
		}
		last_focused_container_id = "radial";
		last_focused_data_entry = entry;
		_refresh_focused_entry_elements();
		emit_signal("data_entry_focused", String("radial"), entry);
	}
}

void ClockGearsUiDesignLayer::_on_radial_elem_selection_changed(int p_slot_idx, const String &p_elem_id) {
	if (!preview_mode && !p_elem_id.is_empty()) {
		active_focus_container_id = p_elem_id;
		for (int i = 0; i < elements.size(); i++) {
			if (elements[i].id == p_elem_id && elements[i].content_root) {
				if (ClockGearsRadialCanvas *rc = Object::cast_to<ClockGearsRadialCanvas>(elements[i].content_root->find_child("ElemRadialCanvas", false, false))) {
					radial_canvas = rc;
				}
				break;
			}
		}
		_update_container_focus_visuals();
	}
	_on_radial_selection_changed(p_slot_idx);
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

void ClockGearsUiDesignLayer::_apply_data_cell_state_styles(ElementEntry &r_entry, float p_scale) {
	Dictionary sstyles = (r_entry.config.has("slot_styles") && r_entry.config["slot_styles"].get_type() == Variant::DICTIONARY)
			? Dictionary(r_entry.config["slot_styles"])
			: Dictionary();
	auto make_cell_sb = [&](const String &p_state, const Color &p_def_bg, const Color &p_def_border, int p_def_bw) -> Ref<StyleBoxFlat> {
		Dictionary cfg = sstyles.has(p_state) && sstyles[p_state].get_type() == Variant::DICTIONARY ? Dictionary(sstyles[p_state]) : Dictionary();
		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_bg_color(_cg_ui_parse_color(cfg.get("fill_color", p_def_bg.to_html(true)), p_def_bg));
		sb->set_border_color(_cg_ui_parse_color(cfg.get("border_color", p_def_border.to_html(true)), p_def_border));
		int bw = cfg.has("border_width") ? CLAMP(int(cfg["border_width"]), 0, 8) : p_def_bw;
		int brad = cfg.has("corner_radius") ? CLAMP(int(cfg["corner_radius"]), 0, 32) : 6;
		sb->set_border_width_all(MAX(0, int(Math::round(float(bw) * p_scale))));
		sb->set_corner_radius_all(MAX(0, int(Math::round(float(brad) * p_scale))));
		int pad = MAX(2, int(Math::round(4.0f * p_scale)));
		sb->set_content_margin_all(pad);
		return sb;
	};
	Ref<StyleBoxFlat> sb_norm = make_cell_sb("normal", Color(0.11f, 0.13f, 0.18f, 0.90f), Color(0.55f, 0.45f, 0.30f, 0.65f), 1);
	Ref<StyleBoxFlat> sb_sel = make_cell_sb("selected", Color(0.22f, 0.18f, 0.12f, 0.98f), Color(0.96f, 0.74f, 0.33f, 1.0f), 2);
	Ref<StyleBoxFlat> sb_dis = make_cell_sb("disabled", Color(0.08f, 0.09f, 0.12f, 0.55f), Color(0.32f, 0.34f, 0.38f, 0.40f), 1);
	Ref<StyleBoxFlat> sb_emp = make_cell_sb("empty", Color(0.06f, 0.07f, 0.10f, 0.40f), Color(0.35f, 0.36f, 0.42f, 0.30f), 1);

	for (int i = 0; i < r_entry.data_cell_buttons.size(); i++) {
		Button *cb = r_entry.data_cell_buttons[i];
		if (!cb) continue;
		bool is_emp = bool(cb->get_meta("entry_empty", false));
		bool is_dis = bool(cb->get_meta("entry_disabled", false));
		bool is_sel = (i == r_entry.focused_data_slot_idx) && !is_emp;
		Ref<StyleBoxFlat> active_sb = is_emp ? sb_emp : (is_sel ? sb_sel : (is_dis ? sb_dis : sb_norm));
		cb->add_theme_style_override("normal", active_sb);
		cb->add_theme_style_override("hover", is_emp ? sb_emp : sb_sel);
		cb->add_theme_style_override("pressed", is_emp ? sb_emp : sb_sel);
		cb->add_theme_style_override("focus", is_emp ? sb_emp : sb_sel);
		cb->add_theme_style_override("disabled", is_emp ? sb_emp : sb_dis);
	}
}

void ClockGearsUiDesignLayer::_refresh_focused_entry_elements() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) continue;
		String b = e.data_binding;
		if ((e.type == "text" || e.type == "variable") && e.label) {
			if (b == "item_name" || b == "item_desc" || b == "item_price" || b == "item_type" || b == "item_params" || (b == "item_qty" && String(e.config.get("item_key", "")).is_empty())) {
				String nt = dm ? dm->resolve_ui_entry_field_binding(b, last_focused_data_entry, e.config, (preview_mode || use_editor_test_data)) : String();
				e.label->set_text(nt);
			}
		} else if (e.type == "image" && e.image_rect && b == "item_icon") {
			Ref<Texture2D> ic_tex;
			if (dm && !last_focused_data_entry.is_empty() && !bool(last_focused_data_entry.get("is_empty", false))) {
				ic_tex = dm->get_entry_icon_texture(last_focused_data_entry, MAX(32, int(e.width * current_ui_scale)));
			}
			e.image_rect->set_texture(ic_tex);
			if (bool(e.config.get("rotatable", false))) {
				e.image_rect->set_pivot_offset(e.image_rect->get_size() * 0.5f);
				e.image_rect->set_rotation_degrees(e.rotatable_angle_deg);
			}
		}
	}
}

void ClockGearsUiDesignLayer::_build_or_refresh_data_container(ElementEntry &r_entry, float p_scale, bool p_rebuild_nodes) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !r_entry.data_scroll || !r_entry.data_items_canvas || !r_entry.content_root) {
		return;
	}
	String ds = ClockGearsDataManager::normalize_ui_data_source(r_entry.config.get("data_source", "manual"));
	if (ds == "manual") {
		r_entry.data_scroll->set_visible(false);
		return;
	}
	r_entry.data_scroll->set_visible(true);
	Size2 avail = r_entry.content_root->get_size();
	if (avail.x < 16.0f || avail.y < 16.0f) {
		return;
	}

	Array entries = dm->resolve_ui_container_entries(r_entry.config, runtime_actor_id, runtime_equip_slot_idx, (preview_mode || use_editor_test_data));
	float spx = float(CLAMP(int(r_entry.config.get("layout_spacing", 8)), 0, 64)) * p_scale;
	float spy = float(CLAMP(int(r_entry.config.get("layout_spacing_y", r_entry.config.get("layout_spacing", 8))), 0, 64)) * p_scale;
	float cell_pad = float(CLAMP(int(r_entry.config.get("cell_padding", 6)), 0, 32)) * p_scale;
	String sig = ds + "|" + r_entry.config.get("layout_mode", "grid").operator String() + "|" + itos(entries.size()) + "|" +
			itos(int(Math::round(avail.x))) + "x" + itos(int(Math::round(avail.y))) + "|" + itos(r_entry.data_carousel_offset) + "|" +
			itos(int(Math::round(spx))) + "x" + itos(int(Math::round(spy))) + "p" + itos(int(Math::round(cell_pad))) + "|" +
			itos(int(r_entry.config.get("layout_columns", 4))) + "r" + itos(int(r_entry.config.get("layout_rows", 2))) + "|" +
			itos(int(Math::round(p_scale * 1000.0f))) + "|" +
			itos(int(preview_mode)) + "|" + itos(int(use_editor_test_data));
	for (int i = 0; i < entries.size() && i < 64; i++) {
		Dictionary ed = entries[i];
		sig += ";" + String(ed.get("key", "")) + ":" + itos(int(ed.get("qty", 0))) + ":" + String(ed.get("name", "")) + ":" + itos(int(bool(ed.get("disabled", false))));
	}
	String prev_sig = r_entry.data_items_canvas->has_meta("_cg_sig") ? String(r_entry.data_items_canvas->get_meta("_cg_sig")) : String();
	if (!p_rebuild_nodes && prev_sig == sig) {
		return;
	}
	r_entry.data_items_canvas->set_meta("_cg_sig", sig);

	for (int c = r_entry.data_items_canvas->get_child_count() - 1; c >= 0; c--) {
		Node *ch = r_entry.data_items_canvas->get_child(c);
		r_entry.data_items_canvas->remove_child(ch);
		ch->queue_free();
	}
	r_entry.data_cell_buttons.clear();
	r_entry.data_resolved_entries = entries;

	String lmode = String(r_entry.config.get("layout_mode", "grid")).strip_edges().to_lower();
	bool scroll_on = r_entry.config.has("layout_scroll") ? bool(r_entry.config["layout_scroll"]) : true;
	int cols = (lmode == "vertical_list") ? 1 : MAX(1, CLAMP(int(r_entry.config.get("layout_columns", 4)), 1, 20));
	int rows_cfg = CLAMP(int(r_entry.config.get("layout_rows", 2)), 0, 20);
	bool pa = bool(r_entry.config.get("pixel_art", true));

	int total = entries.size();
	if (total <= 0) {
		r_entry.data_items_canvas->set_custom_minimum_size(avail);
		return;
	}

	if (lmode == "horizontal_list") {
		r_entry.data_scroll->set_horizontal_scroll_mode(scroll_on ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
		r_entry.data_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	} else {
		r_entry.data_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		r_entry.data_scroll->set_vertical_scroll_mode((scroll_on && lmode != "carousel") ? ScrollContainer::SCROLL_MODE_AUTO : ScrollContainer::SCROLL_MODE_DISABLED);
	}

	float cw = avail.x;
	float ch = MAX(42.0f * p_scale, r_entry.data_tpl_cell_base_size.y * p_scale);
	if (lmode == "grid") {
		cw = MAX(36.0f * p_scale, (avail.x - spx * float(cols - 1)) / float(cols));
		if (rows_cfg > 0) {
			ch = MAX(36.0f * p_scale, (avail.y - spy * float(rows_cfg - 1)) / float(rows_cfg));
		} else {
			ch = MAX(48.0f * p_scale, r_entry.data_tpl_cell_base_size.y * p_scale);
		}
	} else if (lmode == "vertical_list") {
		cols = 1;
		cw = MAX(64.0f * p_scale, avail.x - (scroll_on ? 10.0f * p_scale : 0.0f));
		ch = MAX(42.0f * p_scale, MIN(84.0f * p_scale, r_entry.data_tpl_cell_base_size.y * p_scale));
	} else if (lmode == "horizontal_list") {
		cols = total;
		cw = MAX(96.0f * p_scale, r_entry.data_tpl_cell_base_size.x * p_scale);
		ch = MAX(42.0f * p_scale, avail.y - (scroll_on ? 10.0f * p_scale : 0.0f));
	} else if (lmode == "carousel") {
		cols = MIN(3, total);
		cw = avail.x * 0.42f;
		ch = avail.y * 0.86f;
	}

	int total_rows = (lmode == "horizontal_list") ? 1 : MAX(1, int(Math::ceil(float(total) / float(cols))));
	float canvas_w = (lmode == "horizontal_list") ? (float(total) * cw + float(MAX(0, total - 1)) * spx) : avail.x;
	float canvas_h = (lmode == "horizontal_list" || lmode == "carousel") ? avail.y : (float(total_rows) * ch + float(MAX(0, total_rows - 1)) * spy);
	r_entry.data_items_canvas->set_custom_minimum_size(Size2(canvas_w, MAX(avail.y, canvas_h)));
	r_entry.data_items_canvas->set_size(Size2(canvas_w, MAX(avail.y, canvas_h)));

	Dictionary ov_map = (r_entry.config.has("element_overrides") && r_entry.config["element_overrides"].get_type() == Variant::DICTIONARY)
			? Dictionary(r_entry.config["element_overrides"])
			: Dictionary();
	Ref<Font> cfont = _load_cached_font(String(r_entry.config.get("font_path", "")));

	for (int idx = 0; idx < total; idx++) {
		Dictionary ed = entries[idx];
		Dictionary ov = ov_map.has(itos(idx)) && ov_map[itos(idx)].get_type() == Variant::DICTIONARY ? Dictionary(ov_map[itos(idx)]) : Dictionary();
		bool is_empty = bool(ed.get("is_empty", false));
		bool ov_vis = ov.has("visible") ? bool(ov["visible"]) : true;
		String ov_vmode = String(ov.get("visibility_mode", "always")).strip_edges().to_lower();
		if (ov_vmode == "hidden" || ov_vmode == "never") {
			ov_vis = false;
		} else if (ov_vis && ov_vmode == "switch") {
			String vsw = String(ov.get("visibility_switch", "")).strip_edges();
			ov_vis = (!vsw.is_empty() && dm) ? dm->get_switch(vsw) : false;
		}
		bool ov_dis = false;
		String ov_emode = String(ov.get("enabled_mode", "always")).strip_edges().to_lower();
		if (ov_emode == "disabled") {
			ov_dis = true;
		} else if (ov_emode == "switch") {
			String esw = String(ov.get("enabled_switch", "")).strip_edges();
			ov_dis = (!esw.is_empty() && dm) ? !dm->get_switch(esw) : true;
		}
		bool is_dis = bool(ed.get("disabled", false)) || is_empty || ov_dis;

		Rect2 cell_r;
		if (lmode == "carousel") {
			int cur = CLAMP(r_entry.data_carousel_offset, 0, total - 1);
			int rel = idx - cur;
			if (Math::abs(rel) > 1) {
				Button *hidden_b = memnew(Button);
				hidden_b->set_visible(false);
				r_entry.data_items_canvas->add_child(hidden_b);
				r_entry.data_cell_buttons.push_back(hidden_b);
				continue;
			}
			float csc = (rel == 0) ? 1.0f : 0.78f;
			Size2 csz(cw * csc, ch * csc);
			Vector2 mid = avail * 0.5f;
			cell_r = Rect2(Vector2(mid.x + float(rel) * cw * 0.85f - csz.x * 0.5f, mid.y - csz.y * 0.5f), csz);
		} else {
			int col = (lmode == "horizontal_list") ? idx : (idx % cols);
			int row = (lmode == "horizontal_list") ? 0 : (idx / cols);
			cell_r = Rect2(Vector2(float(col) * (cw + spx), float(row) * (ch + spy)), Size2(cw, ch));
		}

		Button *cb = memnew(Button);
		cb->set_name(vformat("DataCell_%d", idx));
		cb->set_position(cell_r.position);
		cb->set_size(cell_r.size);
		cb->set_visible(ov_vis);
		cb->set_clip_contents(true);
		cb->set_mouse_filter(preview_mode ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
		cb->set_focus_mode((preview_mode || is_empty || is_dis || !ov_vis) ? FOCUS_NONE : FOCUS_ALL);
		cb->set_disabled(is_dis && !is_empty);
		cb->set_meta("entry_empty", is_empty);
		cb->set_meta("entry_disabled", is_dis);
		cb->set_meta("container_id", r_entry.id);
		cb->set_meta("slot_idx", idx);
		String o_img_n = String(ov.get("image_normal", ov.get("image_path", ""))).strip_edges();
		String o_img_h = String(ov.get("image_hover", "")).strip_edges();
		String o_img_p = String(ov.get("image_pressed", "")).strip_edges();
		String o_img_d = String(ov.get("image_disabled", "")).strip_edges();
		if (!o_img_n.is_empty() || !o_img_h.is_empty() || !o_img_p.is_empty() || !o_img_d.is_empty()) {
			auto apply_ov_tex = [&](const StringName &p_st, const String &p_path) {
				String use_p = p_path.is_empty() ? o_img_n : p_path;
				if (use_p.is_empty()) return;
				Ref<Texture2D> t = ClockGearsWindowStyleManager::load_texture_safe(use_p);
				if (t.is_valid()) {
					Ref<StyleBoxTexture> sbt;
					sbt.instantiate();
					sbt->set_texture(t);
					cb->add_theme_style_override(p_st, sbt);
				}
			};
			apply_ov_tex("normal", o_img_n);
			apply_ov_tex("hover", o_img_h);
			apply_ov_tex("pressed", o_img_p);
			apply_ov_tex("focus", o_img_h);
			apply_ov_tex("disabled", o_img_d);
		} else {
			Dictionary sstyles = (r_entry.config.has("slot_styles") && r_entry.config["slot_styles"].get_type() == Variant::DICTIONARY)
					? Dictionary(r_entry.config["slot_styles"])
					: Dictionary();
			Dictionary st_n = ov.has("fill_color") || ov.has("border_color")
					? ov
					: (is_empty && sstyles.has("empty") && sstyles["empty"].get_type() == Variant::DICTIONARY
							? Dictionary(sstyles["empty"])
							: (sstyles.has("normal") && sstyles["normal"].get_type() == Variant::DICTIONARY ? Dictionary(sstyles["normal"]) : Dictionary()));
			Dictionary st_s = (sstyles.has("selected") && sstyles["selected"].get_type() == Variant::DICTIONARY)
					? Dictionary(sstyles["selected"])
					: Dictionary();
			Color def_bg = is_empty ? Color(0.08f, 0.10f, 0.14f, 0.78f) : Color(0.11f, 0.13f, 0.18f, 0.94f);
			Color def_bd = is_empty ? Color(0.45f, 0.50f, 0.60f, 0.48f) : Color(0.83f, 0.61f, 0.26f, 0.85f);
			int bw_n = MAX(1, int(st_n.get("border_width", 1)));
			int rad_px = CLAMP(int(6.0f * p_scale), 2, 12);

			Ref<StyleBoxFlat> osb;
			osb.instantiate();
			osb->set_bg_color(_cg_ui_parse_color(st_n.get("fill_color", ""), def_bg));
			osb->set_border_color(_cg_ui_parse_color(st_n.get("border_color", ""), def_bd));
			osb->set_border_width_all(bw_n);
			osb->set_corner_radius_all(rad_px);

			Ref<StyleBoxFlat> hsb;
			hsb.instantiate();
			hsb->set_bg_color(_cg_ui_parse_color(st_s.get("fill_color", ""), Color(0.17f, 0.20f, 0.28f, 0.98f)));
			hsb->set_border_color(_cg_ui_parse_color(st_s.get("border_color", ""), Color(0.96f, 0.74f, 0.33f, 1.0f)));
			hsb->set_border_width_all(MAX(2, int(st_s.get("border_width", 2))));
			hsb->set_corner_radius_all(rad_px);

			cb->add_theme_style_override("normal", osb);
			cb->add_theme_style_override("disabled", osb);
			cb->add_theme_style_override("hover", is_empty ? osb : hsb);
			cb->add_theme_style_override("focus", is_empty ? osb : hsb);
			cb->add_theme_style_override("pressed", hsb);
		}
		if (ov.has("font_color")) {
			cb->add_theme_color_override("font_color", _cg_ui_parse_color(ov["font_color"], Color(1, 1, 1, 1)));
		}
		if (!preview_mode) {
			cb->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_data_cell_pressed).bind(r_entry.id, idx), CONNECT_DEFERRED);
			cb->connect("mouse_entered", callable_mp(this, &ClockGearsUiDesignLayer::_on_data_cell_focused).bind(r_entry.id, idx));
			cb->connect("focus_entered", callable_mp(this, &ClockGearsUiDesignLayer::_on_data_cell_focused).bind(r_entry.id, idx));
		}
		r_entry.data_items_canvas->add_child(cb);
		r_entry.data_cell_buttons.push_back(cb);

		if (!r_entry.data_tpl_children.is_empty()) {
			Vector2 base_sz = Vector2(MAX(40.0f, r_entry.data_tpl_cell_base_size.x), MAX(30.0f, r_entry.data_tpl_cell_base_size.y));
			float inner_w = MAX(16.0f, cell_r.size.x - cell_pad * 2.0f);
			float inner_h = MAX(16.0f, cell_r.size.y - cell_pad * 2.0f);
			float sx = inner_w / base_sz.x;
			float sy = inner_h / base_sz.y;
			for (int k = 0; k < r_entry.data_tpl_children.size(); k++) {
				const Dictionary &tc = r_entry.data_tpl_children[k];
				String ttype = String(tc.get("type", "text")).strip_edges().to_lower();
				String tbind = String(tc.get("data_binding", "none")).strip_edges().to_lower();
				Vector2 child_pos(cell_pad + float(int(tc.get("offset_x", 0))) * sx, cell_pad + float(int(tc.get("offset_y", 0))) * sy);
				Vector2 child_sz(MAX(8.0f, float(int(tc.get("width", 48))) * sx), MAX(8.0f, float(int(tc.get("height", 20))) * sy));
				if (ttype == "image" || tbind == "item_icon") {
					TextureRect *tr = memnew(TextureRect);
					tr->set_mouse_filter(MOUSE_FILTER_IGNORE);
					tr->set_position(child_pos);
					tr->set_size(child_sz);
					tr->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
					tr->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
					tr->set_texture_filter(pa ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
					if (!is_empty) {
						tr->set_texture(dm->get_entry_icon_texture(ed, MAX(16, int(MIN(child_sz.x, child_sz.y)))));
					}
					cb->add_child(tr);
				} else {
					Label *lb = memnew(Label);
					lb->set_mouse_filter(MOUSE_FILTER_IGNORE);
					lb->set_position(child_pos);
					lb->set_size(child_sz);
					lb->set_clip_text(true);
					lb->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
					String al = String(tc.get("align", "left"));
					lb->set_horizontal_alignment((al == "center") ? HORIZONTAL_ALIGNMENT_CENTER : ((al == "right") ? HORIZONTAL_ALIGNMENT_RIGHT : HORIZONTAL_ALIGNMENT_LEFT));
					lb->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
					int fsz = MAX(8, int(Math::round(float(tc.get("font_size", 13)) * MIN(sx, sy))));
					lb->add_theme_font_size_override("font_size", fsz);
					if (cfont.is_valid()) lb->add_theme_font_override("font", cfont);
					if (tc.has("font_color")) {
						Color fcol = _cg_ui_parse_color(tc["font_color"], Color(1, 1, 1, 1));
						if (is_empty || is_dis) fcol.a *= 0.55f;
						lb->add_theme_color_override("font_color", fcol);
					}
					String txt = (!tbind.is_empty() && tbind != "none")
							? dm->resolve_ui_entry_field_binding(tbind, ed, tc, (preview_mode || use_editor_test_data))
							: String(ov.get("text", tc.get("text", "")));
					lb->set_text(txt);
					cb->add_child(lb);
				}
			}
		} else {
			// Plantilla por defecto integrada con relleno interior, icono, nombre y cantidad (o Vacía N)
			MarginContainer *cmc = memnew(MarginContainer);
			cmc->set_mouse_filter(MOUSE_FILTER_IGNORE);
			cmc->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
			int pad_i = MAX(2, int(Math::round(cell_pad)));
			cmc->add_theme_constant_override("margin_left", pad_i);
			cmc->add_theme_constant_override("margin_right", pad_i);
			cmc->add_theme_constant_override("margin_top", pad_i);
			cmc->add_theme_constant_override("margin_bottom", pad_i);
			cb->add_child(cmc);

			String disp_name = ov.has("text") ? String(ov["text"]) : dm->resolve_ui_entry_field_binding("item_name", ed, Dictionary(), (preview_mode || use_editor_test_data));
			String qty_str = dm->resolve_ui_entry_field_binding("item_qty", ed, Dictionary(), (preview_mode || use_editor_test_data));
			int fsz = MAX(9, int(Math::round(13.0f * p_scale)));
			const bool vertical_card = (lmode == "grid" || lmode == "carousel") && cell_r.size.y >= 84.0f * p_scale && cell_r.size.x < 240.0f * p_scale;
			if (vertical_card && !is_empty) {
				VBoxContainer *vb = memnew(VBoxContainer);
				vb->set_mouse_filter(MOUSE_FILTER_IGNORE);
				vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
				vb->add_theme_constant_override("separation", MAX(2, int(4.0f * p_scale)));
				cmc->add_child(vb);

				float ic_px = CLAMP(MIN(cell_r.size.y * 0.42f, 44.0f * p_scale), 18.0f, 48.0f);
				TextureRect *ic = memnew(TextureRect);
				ic->set_mouse_filter(MOUSE_FILTER_IGNORE);
				ic->set_custom_minimum_size(Size2(ic_px, ic_px));
				ic->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
				ic->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
				ic->set_texture_filter(pa ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
				ic->set_texture(dm->get_entry_icon_texture(ed, MAX(16, int(ic_px))));
				vb->add_child(ic);

				Label *nlbl = memnew(Label);
				nlbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
				nlbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
				nlbl->set_clip_text(true);
				nlbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				nlbl->add_theme_font_size_override("font_size", fsz);
				if (cfont.is_valid()) nlbl->add_theme_font_override("font", cfont);
				nlbl->set_text(disp_name);
				vb->add_child(nlbl);

				if (!qty_str.is_empty()) {
					Label *qlbl = memnew(Label);
					qlbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
					qlbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
					qlbl->add_theme_font_size_override("font_size", MAX(8, fsz - 1));
					if (cfont.is_valid()) qlbl->add_theme_font_override("font", cfont);
					qlbl->add_theme_color_override("font_color", Color(0.95f, 0.78f, 0.42f, 1.0f));
					qlbl->set_text(qty_str);
					vb->add_child(qlbl);
				}
			} else {
				HBoxContainer *hb = memnew(HBoxContainer);
				hb->set_mouse_filter(MOUSE_FILTER_IGNORE);
				hb->add_theme_constant_override("separation", MAX(4, int(6.0f * p_scale)));
				cmc->add_child(hb);

				if (!is_empty) {
					float ic_px = CLAMP(MIN(cell_r.size.y - float(pad_i) * 2.0f, 36.0f * p_scale), 16.0f, 44.0f);
					TextureRect *ic = memnew(TextureRect);
					ic->set_mouse_filter(MOUSE_FILTER_IGNORE);
					ic->set_custom_minimum_size(Size2(ic_px, ic_px));
					ic->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
					ic->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
					ic->set_texture_filter(pa ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);
					ic->set_texture(dm->get_entry_icon_texture(ed, MAX(16, int(ic_px))));
					hb->add_child(ic);
				}

				Label *nlbl = memnew(Label);
				nlbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
				nlbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				nlbl->set_horizontal_alignment(is_empty ? HORIZONTAL_ALIGNMENT_CENTER : HORIZONTAL_ALIGNMENT_LEFT);
				nlbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
				nlbl->set_clip_text(true);
				nlbl->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				nlbl->add_theme_font_size_override("font_size", fsz);
				if (cfont.is_valid()) nlbl->add_theme_font_override("font", cfont);
				if (is_empty) nlbl->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.80f, 0.65f));
				nlbl->set_text(disp_name);
				hb->add_child(nlbl);

				if (!qty_str.is_empty()) {
					Label *qlbl = memnew(Label);
					qlbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
					qlbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
					qlbl->add_theme_font_size_override("font_size", MAX(8, fsz - 1));
					if (cfont.is_valid()) qlbl->add_theme_font_override("font", cfont);
					qlbl->set_text(qty_str);
					hb->add_child(qlbl);
				}
			}
		}
	}

	if (lmode == "carousel" && total > 1) {
		float bw = 28.0f * p_scale;
		float bh = 28.0f * p_scale;
		Button *prev_b = memnew(Button);
		prev_b->set_text("<");
		prev_b->set_position(Vector2(2.0f, (avail.y - bh) * 0.5f));
		prev_b->set_size(Size2(bw, bh));
		prev_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_data_container_carousel_delta).bind(r_entry.id, -1));
		ClockGearsWindowStyleManager::apply_button_style(prev_b, window_type_fallback);
		r_entry.content_root->add_child(prev_b);
		layout_aux_buttons.push_back(prev_b);

		Button *next_b = memnew(Button);
		next_b->set_text(">");
		next_b->set_position(Vector2(avail.x - bw - 2.0f, (avail.y - bh) * 0.5f));
		next_b->set_size(Size2(bw, bh));
		next_b->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_data_container_carousel_delta).bind(r_entry.id, 1));
		ClockGearsWindowStyleManager::apply_button_style(next_b, window_type_fallback);
		r_entry.content_root->add_child(next_b);
		layout_aux_buttons.push_back(next_b);
	}

	r_entry.focused_data_slot_idx = CLAMP(r_entry.focused_data_slot_idx, 0, total - 1);
	_apply_data_cell_state_styles(r_entry, p_scale);
	if (last_focused_data_entry.is_empty() || last_focused_container_id == r_entry.id) {
		last_focused_container_id = r_entry.id;
		last_focused_data_entry = Dictionary(entries[r_entry.focused_data_slot_idx]).duplicate(true);
		focused_entry_by_container[r_entry.id] = last_focused_data_entry;
		_refresh_focused_entry_elements();
	}
}

void ClockGearsUiDesignLayer::_on_data_container_carousel_delta(const String &p_container_id, int p_delta) {
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_container_id && !elements[i].data_resolved_entries.is_empty()) {
			int cnt = elements[i].data_resolved_entries.size();
			elements.write[i].data_carousel_offset = ((elements[i].data_carousel_offset + p_delta) % cnt + cnt) % cnt;
			elements.write[i].focused_data_slot_idx = elements[i].data_carousel_offset;
			_build_or_refresh_data_container(elements.write[i], current_ui_scale, true);
			_on_data_cell_focused(p_container_id, elements[i].focused_data_slot_idx);
			break;
		}
	}
}

void ClockGearsUiDesignLayer::_on_data_cell_focused(const String &p_container_id, int p_slot_idx) {
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (e.id != p_container_id) continue;
		if (p_slot_idx < 0 || p_slot_idx >= e.data_resolved_entries.size()) return;
		e.focused_data_slot_idx = p_slot_idx;
		_apply_data_cell_state_styles(e, current_ui_scale);
		if (e.data_scroll && p_slot_idx < e.data_cell_buttons.size() && e.data_cell_buttons[p_slot_idx]) {
			e.data_scroll->ensure_control_visible(e.data_cell_buttons[p_slot_idx]);
		}
		Dictionary entry = Dictionary(e.data_resolved_entries[p_slot_idx]).duplicate(true);
		last_focused_container_id = p_container_id;
		active_focus_container_id = p_container_id;
		last_focused_data_entry = entry;
		focused_entry_by_container[p_container_id] = entry;
		_refresh_focused_entry_elements();
		_update_container_focus_visuals();
		emit_signal("data_entry_focused", p_container_id, entry);
		return;
	}
}

void ClockGearsUiDesignLayer::_on_data_cell_pressed(const String &p_container_id, int p_slot_idx) {
	_on_data_cell_focused(p_container_id, p_slot_idx);
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (e.id != p_container_id) continue;
		if (p_slot_idx < 0 || p_slot_idx >= e.data_resolved_entries.size()) return;
		Dictionary entry = e.data_resolved_entries[p_slot_idx];
		if (bool(entry.get("is_empty", false))) return;

		String slot_elem_id = "__cell_" + p_container_id + "_" + itos(p_slot_idx);
		Dictionary ov_map = (e.config.has("element_overrides") && e.config["element_overrides"].get_type() == Variant::DICTIONARY) ? Dictionary(e.config["element_overrides"]) : Dictionary();
		Dictionary ov = ov_map.has(itos(p_slot_idx)) && ov_map[itos(p_slot_idx)].get_type() == Variant::DICTIONARY ? Dictionary(ov_map[itos(p_slot_idx)]) : Dictionary();

		Array steps;
		if (ov.has("actions") && ov["actions"].get_type() == Variant::ARRAY && !Array(ov["actions"]).is_empty()) {
			steps = Array(ov["actions"]).duplicate(true);
		} else if (e.config.has("slot_actions") && e.config["slot_actions"].get_type() == Variant::ARRAY && !Array(e.config["slot_actions"]).is_empty()) {
			steps = Array(e.config["slot_actions"]).duplicate(true);
		} else {
			String sact = String(ov.get("action", e.config.get("slot_action", entry.get("slot_action", "")))).strip_edges().to_lower();
			if (sact.is_empty()) {
				sact = String(entry.get("slot_action", "show_detail")).strip_edges().to_lower();
			}
			if (!sact.is_empty() && sact != "none") {
				Dictionary st;
				st["type"] = sact;
				st["item_key"] = entry.get("key", "");
				st["slot"] = entry.get("slot", 0);
				steps.push_back(st);
			}
		}

		// Ejecutar acciones directas de inventario / equipo cuando no hay manejador externo
		String primary_act;
		bool handled_locally = false;
		if (!steps.is_empty() && steps[0].get_type() == Variant::DICTIONARY) {
			primary_act = String(Dictionary(steps[0]).get("type", "")).strip_edges().to_lower();
		}
		if (dm && !preview_mode && !get_meta("_cg_external_item_handler", false)) {
			String ik = String(entry.get("key", ""));
			int target_actor = runtime_actor_id > 0 ? runtime_actor_id : (!dm->get_party_actor_ids().is_empty() ? int(dm->get_party_actor_ids()[0]) : 1);
			if ((primary_act == "item_use" || primary_act == "use") && !ik.is_empty()) {
				handled_locally = dm->use_item_on_actor(ik, target_actor);
			} else if ((primary_act == "item_discard" || primary_act == "discard") && !ik.is_empty()) {
				handled_locally = dm->discard_item(ik, 1);
			} else if ((primary_act == "equip_apply" || primary_act == "equip") && !ik.is_empty()) {
				handled_locally = dm->equip_actor_piece(target_actor, runtime_equip_slot_idx, ik);
			}
		}
		dynamic_slot_actions_by_id[slot_elem_id.to_lower()] = steps;
		emit_signal("data_entry_activated", p_container_id, p_slot_idx, entry);
		emit_signal("action_triggered", primary_act, slot_elem_id);
		if (handled_locally) {
			refresh_live_values();
		}
		return;
	}
}

void ClockGearsUiDesignLayer::_on_rotatable_preview_gui_input(const Ref<InputEvent> &p_event, const String &p_elem_id) {
	if (preview_mode || p_event.is_null()) return;
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (e.id != p_elem_id || !e.image_rect) continue;
		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
			e.rotatable_dragging = mb->is_pressed();
			return;
		}
		Ref<InputEventMouseMotion> mm = p_event;
		if (mm.is_valid() && e.rotatable_dragging) {
			e.rotatable_angle_deg += mm->get_relative().x * 1.2f;
			e.image_rect->set_pivot_offset(e.image_rect->get_size() * 0.5f);
			e.image_rect->set_rotation_degrees(e.rotatable_angle_deg);
			return;
		}
	}
}

bool ClockGearsUiDesignLayer::focus_data_slot(const String &p_container_id, int p_slot_idx) {
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_container_id && p_slot_idx >= 0 && p_slot_idx < elements[i].data_resolved_entries.size()) {
			_on_data_cell_focused(p_container_id, p_slot_idx);
			if (p_slot_idx < elements[i].data_cell_buttons.size() && elements[i].data_cell_buttons[p_slot_idx]) {
				Button *cb = elements[i].data_cell_buttons[p_slot_idx];
				if (cb->get_focus_mode() != FOCUS_NONE && cb->is_inside_tree()) {
					cb->grab_focus();
				}
			}
			return true;
		}
	}
	return false;
}

bool ClockGearsUiDesignLayer::activate_data_slot(const String &p_container_id, int p_slot_idx) {
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_container_id && p_slot_idx >= 0 && p_slot_idx < elements[i].data_resolved_entries.size()) {
			_on_data_cell_pressed(p_container_id, p_slot_idx);
			return true;
		}
	}
	return false;
}

Dictionary ClockGearsUiDesignLayer::get_data_container_info(const String &p_container_id) const {
	Dictionary info;
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.id == p_container_id || (p_container_id.is_empty() && e.type == "container")) {
			info["id"] = e.id;
			info["data_source"] = ClockGearsDataManager::normalize_ui_data_source(e.config.get("data_source", "manual"));
			info["entry_count"] = e.data_resolved_entries.size();
			info["entries"] = e.data_resolved_entries.duplicate(true);
			info["cell_count"] = e.data_cell_buttons.size();
			info["focused_slot"] = e.focused_data_slot_idx;
			info["scroll_vertical"] = e.data_scroll ? int(e.data_scroll->get_v_scroll()) : 0;
			info["scroll_horizontal"] = e.data_scroll ? int(e.data_scroll->get_h_scroll()) : 0;
			info["scroll_max_v"] = e.data_items_canvas ? int(e.data_items_canvas->get_size().y) : 0;
			return info;
		}
	}
	return info;
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
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.type != "container" || !e.root || !e.root->is_visible()) {
			continue;
		}
		for (int c = 0; c < e.data_cell_buttons.size(); c++) {
			Button *cb = e.data_cell_buttons[c];
			if (cb && cb->is_visible() && !cb->is_disabled() && !bool(cb->get_meta("entry_empty", false))) {
				out.push_back(cb);
			}
		}
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
		if (String(b->get_name()).begins_with("DataCell_")) {
			return b->get_global_position() + b->get_size() * 0.5f;
		}
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
	if (dynamic_slot_actions_by_id.has(target)) {
		return Array(dynamic_slot_actions_by_id[target]).duplicate(true);
	}
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
		Array r_acts = radial_canvas->get_slot_actions_by_id(p_elem_id);
		if (!r_acts.is_empty()) {
			return r_acts;
		}
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
		for (int i = 0; i < elements.size(); i++) {
			ElementEntry &pe = elements.write[i];
			if (pe.type != "pager" || !pe.pager_trans_active) continue;
			pe.pager_trans_elapsed += dt;
			float pt = (pe.pager_trans_duration > 0.0001) ? float(pe.pager_trans_elapsed / pe.pager_trans_duration) : 1.0f;
			if (pt >= 1.0f) {
				pt = 1.0f;
				pe.pager_trans_active = false;
			}
			_apply_pager_transition_pose(pe, pt);
			if (!pe.pager_trans_active) {
				pe.pager_trans_from_idx = -1;
				_sync_pager_pages_layout_and_vis(pe);
			}
		}
		for (int i = 0; i < elements.size(); i++) {
			ElementEntry &ce = elements.write[i];
			if (!ce.container_trans_active) continue;
			ce.container_trans_elapsed += dt;
			float ct = (ce.container_trans_duration > 0.0001) ? float(ce.container_trans_elapsed / ce.container_trans_duration) : 1.0f;
			if (ct >= 1.0f) {
				ct = 1.0f;
				ce.container_trans_active = false;
			}
			_apply_container_transition_pose(ce, ct);
			if (!ce.container_trans_active && ce.root) {
				ce.root->set_position(ce.container_base_pos);
				ce.root->set_scale(Vector2(1.0f, 1.0f));
				ce.root->set_modulate(Color(1, 1, 1, 1));
				bool show_fin = ce.runtime_vis_override_set ? ce.runtime_vis_override : ce.visible;
				ce.root->set_visible(show_fin);
				_update_container_focus_visuals();
			}
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
	ClassDB::bind_static_method("ClockGearsUiDesignLayer", D_METHOD("compute_element_global_rect", "elem_id", "elems", "stage_rect", "scale"), &ClockGearsUiDesignLayer::compute_element_global_rect);
	ClassDB::bind_static_method("ClockGearsUiDesignLayer", D_METHOD("is_element_effectively_visible", "elem_id", "elems"), &ClockGearsUiDesignLayer::is_element_effectively_visible);
	ClassDB::bind_static_method("ClockGearsUiDesignLayer", D_METHOD("is_element_effectively_locked", "elem_id", "elems"), &ClockGearsUiDesignLayer::is_element_effectively_locked);
	ClassDB::bind_method(D_METHOD("set_screen_id", "screen_id"), &ClockGearsUiDesignLayer::set_screen_id);
	ClassDB::bind_method(D_METHOD("get_screen_id"), &ClockGearsUiDesignLayer::get_screen_id);
	ClassDB::bind_method(D_METHOD("set_window_type_fallback", "win_type"), &ClockGearsUiDesignLayer::set_window_type_fallback);
	ClassDB::bind_method(D_METHOD("get_window_type_fallback"), &ClockGearsUiDesignLayer::get_window_type_fallback);
	ClassDB::bind_method(D_METHOD("set_preview_mode", "preview"), &ClockGearsUiDesignLayer::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsUiDesignLayer::is_preview_mode);
	ClassDB::bind_method(D_METHOD("set_use_editor_test_data", "use_test"), &ClockGearsUiDesignLayer::set_use_editor_test_data);
	ClassDB::bind_method(D_METHOD("is_using_editor_test_data"), &ClockGearsUiDesignLayer::is_using_editor_test_data);
	ClassDB::bind_method(D_METHOD("set_runtime_actor_context", "actor_id", "equip_slot_idx"), &ClockGearsUiDesignLayer::set_runtime_actor_context, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_focused_data_entry"), &ClockGearsUiDesignLayer::get_focused_data_entry);
	ClassDB::bind_method(D_METHOD("focus_data_slot", "container_id", "slot_idx"), &ClockGearsUiDesignLayer::focus_data_slot);
	ClassDB::bind_method(D_METHOD("activate_data_slot", "container_id", "slot_idx"), &ClockGearsUiDesignLayer::activate_data_slot);
	ClassDB::bind_method(D_METHOD("get_data_container_info", "container_id"), &ClockGearsUiDesignLayer::get_data_container_info, DEFVAL(""));
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
	ClassDB::bind_method(D_METHOD("handle_runtime_input_event", "event"), &ClockGearsUiDesignLayer::handle_runtime_input_event);
	ClassDB::bind_method(D_METHOD("set_pager_active_page", "pager_id", "page_idx", "animate", "play_sound"), &ClockGearsUiDesignLayer::set_pager_active_page, DEFVAL(true), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("step_pager_page", "pager_id", "delta", "animate"), &ClockGearsUiDesignLayer::step_pager_page, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_pager_active_page", "pager_id"), &ClockGearsUiDesignLayer::get_pager_active_page);
	ClassDB::bind_method(D_METHOD("get_pager_info", "pager_id"), &ClockGearsUiDesignLayer::get_pager_info);
	ClassDB::bind_method(D_METHOD("set_container_open", "elem_id", "open", "trans", "dur"), &ClockGearsUiDesignLayer::set_container_open, DEFVAL("scale"), DEFVAL(0.16));
	ClassDB::bind_method(D_METHOD("toggle_container_open", "elem_id", "trans", "dur"), &ClockGearsUiDesignLayer::toggle_container_open, DEFVAL("scale"), DEFVAL(0.16));
	ClassDB::bind_method(D_METHOD("is_container_open", "elem_id"), &ClockGearsUiDesignLayer::is_container_open);
	ClassDB::bind_method(D_METHOD("focus_container", "elem_id"), &ClockGearsUiDesignLayer::focus_container);
	ClassDB::bind_method(D_METHOD("navigate_focus_direction", "dx", "dy"), &ClockGearsUiDesignLayer::navigate_focus_direction);
	ClassDB::bind_method(D_METHOD("get_container_focus_overlay_state"), &ClockGearsUiDesignLayer::get_container_focus_overlay_state);
	ClassDB::bind_method(D_METHOD("get_active_focus_container_id"), &ClockGearsUiDesignLayer::get_active_focus_container_id);
	ClassDB::bind_method(D_METHOD("set_container_filter", "elem_id", "filter_category"), &ClockGearsUiDesignLayer::set_container_filter);
	ClassDB::bind_method(D_METHOD("set_container_data_source", "elem_id", "data_source"), &ClockGearsUiDesignLayer::set_container_data_source);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsUiDesignLayer::get_layout_bounds_info, DEFVAL(Vector2()));

	ADD_SIGNAL(MethodInfo("action_triggered", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::STRING, "element_id")));
	ADD_SIGNAL(MethodInfo("radial_selection_changed", PropertyInfo(Variant::INT, "slot_index")));
	ADD_SIGNAL(MethodInfo("data_entry_focused", PropertyInfo(Variant::STRING, "container_id"), PropertyInfo(Variant::DICTIONARY, "entry")));
	ADD_SIGNAL(MethodInfo("data_entry_activated", PropertyInfo(Variant::STRING, "container_id"), PropertyInfo(Variant::INT, "slot_index"), PropertyInfo(Variant::DICTIONARY, "entry")));
	ADD_SIGNAL(MethodInfo("pager_page_changed", PropertyInfo(Variant::STRING, "pager_id"), PropertyInfo(Variant::INT, "page_index"), PropertyInfo(Variant::STRING, "page_id")));
}

ClockGearsUiDesignLayer::ClockGearsUiDesignLayer() {
	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_process_input(true);
	set_process_unhandled_input(true);
}

void ClockGearsUiDesignLayer::input(const Ref<InputEvent> &p_event) {
	if (preview_mode || !is_visible_in_tree() || p_event.is_null()) {
		return;
	}
	if (Viewport *vp = get_viewport()) {
		Control *fo = vp->gui_get_focus_owner();
		if (fo && (fo->is_class("LineEdit") || fo->is_class("TextEdit"))) {
			return;
		}
	}

	// Comprobar si hay un contenedor/botón no-radial enfocado o navegación entre contenedores (Punto 4)
	bool focused_is_radial = false;
	ClockGearsRadialCanvas *focused_rc = nullptr;
	ClockGearsRadialCanvas *fallback_rc = nullptr;
	int best_layer = -999999;
	bool has_navigable_container = false;
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (!e.root || !e.root->is_visible_in_tree()) continue;
		if (e.type == "container" || e.type == "radial") {
			has_navigable_container = true;
		} else if (!String(e.config.get("focus_next_left", "")).is_empty() ||
				!String(e.config.get("focus_next_right", "")).is_empty() ||
				!String(e.config.get("focus_next_up", "")).is_empty() ||
				!String(e.config.get("focus_next_down", "")).is_empty()) {
			has_navigable_container = true;
		}
		if (e.type == "radial" && e.content_root) {
			ClockGearsRadialCanvas *rc = Object::cast_to<ClockGearsRadialCanvas>(e.content_root->find_child("ElemRadialCanvas", false, false));
			if (!rc || !rc->is_visible_in_tree()) continue;
			if (e.id == active_focus_container_id) {
				focused_is_radial = true;
				focused_rc = rc;
			}
			if (e.layer_order >= best_layer) {
				best_layer = e.layer_order;
				fallback_rc = rc;
			}
		}
	}
	if (active_focus_container_id.is_empty() && fallback_rc) {
		focused_is_radial = true;
		focused_rc = fallback_rc;
	}
	if (!focused_rc && radial_canvas && radial_canvas->is_visible_in_tree() && active_focus_container_id.is_empty()) {
		focused_is_radial = true;
		focused_rc = radial_canvas;
	}

	if (focused_is_radial && focused_rc) {
		Ref<InputEventJoypadMotion> jm = p_event;
		if (jm.is_valid()) {
			int dev = jm->get_device();
			float ax = Input::get_singleton() ? Input::get_singleton()->get_joy_axis(dev, JoyAxis::LEFT_X) : 0.0f;
			float ay = Input::get_singleton() ? Input::get_singleton()->get_joy_axis(dev, JoyAxis::LEFT_Y) : 0.0f;
			if (jm->get_axis() == JoyAxis::LEFT_X) ax = jm->get_axis_value();
			if (jm->get_axis() == JoyAxis::LEFT_Y) ay = jm->get_axis_value();
			Vector2 stick(ax, ay);
			if (stick.length() >= 0.55f) {
				focused_rc->select_by_angle_vector(stick);
				if (get_viewport()) get_viewport()->set_input_as_handled();
				return;
			}
		}
		if (p_event->is_pressed() && !p_event->is_echo()) {
			Ref<InputEventKey> rk = p_event;
			Ref<InputEventJoypadButton> rj = p_event;
			Key kc = rk.is_valid() ? rk->get_keycode() : Key::NONE;
			JoyButton jb = rj.is_valid() ? rj->get_button_index() : JoyButton::INVALID;
			if (kc == Key::RIGHT || kc == Key::D || jb == JoyButton::DPAD_RIGHT) {
				focused_rc->step_selection(1);
				if (get_viewport()) get_viewport()->set_input_as_handled();
				return;
			}
			if (kc == Key::LEFT || kc == Key::A || jb == JoyButton::DPAD_LEFT) {
				focused_rc->step_selection(-1);
				if (get_viewport()) get_viewport()->set_input_as_handled();
				return;
			}
			if (kc == Key::UP || kc == Key::W || jb == JoyButton::DPAD_UP) {
				focused_rc->select_by_angle_vector(Vector2(0, -1));
				if (get_viewport()) get_viewport()->set_input_as_handled();
				return;
			}
			if (kc == Key::DOWN || kc == Key::S || jb == JoyButton::DPAD_DOWN) {
				focused_rc->select_by_angle_vector(Vector2(0, 1));
				if (get_viewport()) get_viewport()->set_input_as_handled();
				return;
			}
			if (kc == Key::ENTER || kc == Key::KP_ENTER || kc == Key::SPACE || jb == JoyButton::A) {
				if (Viewport *vp = get_viewport()) {
					Button *focused_btn = Object::cast_to<Button>(vp->gui_get_focus_owner());
					if (focused_btn && owns_control(focused_btn) && !focused_btn->is_disabled() && focused_btn->is_visible_in_tree()) {
						focused_btn->emit_signal("pressed");
						vp->set_input_as_handled();
						return;
					}
				}
				if (focused_rc->activate_selected_slot()) {
					if (get_viewport()) get_viewport()->set_input_as_handled();
					return;
				}
			}
		}
		return;
	}

	if (has_navigable_container && p_event->is_pressed()) {
		Ref<InputEventKey> rk = p_event;
		Ref<InputEventJoypadButton> rj = p_event;
		Key kc = rk.is_valid() ? rk->get_keycode() : Key::NONE;
		JoyButton jb = rj.is_valid() ? rj->get_button_index() : JoyButton::INVALID;
		int dx = 0;
		int dy = 0;
		if (kc == Key::RIGHT || jb == JoyButton::DPAD_RIGHT) dx = 1;
		else if (kc == Key::LEFT || jb == JoyButton::DPAD_LEFT) dx = -1;
		else if (kc == Key::DOWN || jb == JoyButton::DPAD_DOWN) dy = 1;
		else if (kc == Key::UP || jb == JoyButton::DPAD_UP) dy = -1;
		if (dx != 0 || dy != 0) {
			if (navigate_focus_direction(dx, dy)) {
				if (get_viewport()) get_viewport()->set_input_as_handled();
				return;
			}
		}
	}
}

void ClockGearsUiDesignLayer::unhandled_input(const Ref<InputEvent> &p_event) {
	if (preview_mode || !is_visible_in_tree() || p_event.is_null() || !p_event->is_pressed() || p_event->is_echo()) {
		return;
	}
	if (handle_runtime_input_event(p_event)) {
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
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

bool ClockGearsUiDesignLayer::owns_control(const Control *p_ctrl) const {
	if (!p_ctrl) {
		return false;
	}
	const Node *cur = p_ctrl;
	while (cur) {
		if (cur == this) {
			return true;
		}
		cur = cur->get_parent();
	}
	return false;
}

bool ClockGearsUiDesignLayer::execute_common_action_step(const Dictionary &p_step, ClockGearsUiDesignLayer *p_layer) {
	String stype = String(p_step.get("type", p_step.get("action", ""))).strip_edges().to_lower();
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (stype == "set_switch") {
		String sw = String(p_step.get("switch_name", p_step.get("switch", p_step.get("name", "")))).strip_edges();
		Variant raw_v = p_step.get("value", true);
		if (dm && !sw.is_empty()) {
			bool val = (raw_v.get_type() == Variant::STRING && String(raw_v).strip_edges().to_lower() == "toggle")
					? !dm->get_switch(sw)
					: bool(raw_v);
			dm->set_switch(sw, val);
			if (p_layer && p_layer->is_visible()) {
				p_layer->refresh_live_values();
			}
			return true;
		}
		return false;
	}
	if (stype == "set_variable") {
		String vn = String(p_step.get("variable_name", "")).strip_edges();
		if (vn.is_empty()) {
			vn = String(p_step.get("var_name", p_step.get("variable", p_step.get("name", "")))).strip_edges();
		}
		String op = String(p_step.get("op", "set")).strip_edges().to_lower();
		int val = int(p_step.get("value", 0));
		if (dm && !vn.is_empty()) {
			int cur = dm->get_variable(vn);
			if (op == "+" || op == "add") cur += val;
			else if (op == "-" || op == "sub") cur -= val;
			else if (op == "*" || op == "mul") cur *= val;
			else if ((op == "/" || op == "div") && val != 0) cur /= val;
			else if ((op == "%" || op == "mod") && val != 0) cur %= val;
			else cur = val;
			dm->set_variable(vn, cur);
			if (p_layer && p_layer->is_visible()) {
				p_layer->refresh_live_values();
			}
			return true;
		}
		return false;
	}
	if (stype == "play_sound") {
		String spath = String(p_step.get("sound_path", p_step.get("sound", p_step.get("path", "")))).strip_edges();
		float vol = p_step.has("volume") ? float(p_step["volume"]) : Math::db_to_linear(float(p_step.get("volume_db", 0.0f)));
		float pitch = float(p_step.get("pitch", 1.0f));
		if (!spath.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_se(spath, vol, pitch);
			}
			return true;
		}
		return false;
	}
	if (stype == "play_music") {
		String spath = String(p_step.get("sound_path", p_step.get("music_path", p_step.get("path", "")))).strip_edges();
		float vol = p_step.has("volume") ? float(p_step["volume"]) : Math::db_to_linear(float(p_step.get("volume_db", 0.0f)));
		float pitch = float(p_step.get("pitch", 1.0f));
		if (!spath.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_bgm(spath, vol, pitch);
			}
			return true;
		}
		return false;
	}
	if (stype == "hud_visibility") {
		String scope = String(p_step.get("scope", "all")).strip_edges().to_lower();
		String widget = String(p_step.get("widget_id", p_step.get("widget", p_step.get("widget_name", "")))).strip_edges();
		Variant raw_vis = p_step.get("visible", true);
		if (dm) {
			bool vis = true;
			if (raw_vis.get_type() == Variant::STRING && String(raw_vis).strip_edges().to_lower() == "toggle") {
				vis = widget.is_empty() ? !dm->is_hud_runtime_visible() : !dm->is_hud_widget_runtime_visible(widget);
			} else {
				vis = bool(raw_vis);
			}
			if (!widget.is_empty() && scope != "all") {
				dm->set_hud_widget_runtime_visible(widget, vis);
			} else {
				dm->set_hud_runtime_visible(vis);
			}
			return true;
		}
		return false;
	}
	if (p_layer) {
		String tgt = String(p_step.get("target_elem", p_step.get("target", ""))).strip_edges();
		if (stype == "next_page") {
			return p_layer->step_pager_page(tgt, 1, true);
		}
		if (stype == "prev_page") {
			return p_layer->step_pager_page(tgt, -1, true);
		}
		if (stype == "go_to_page") {
			String pid = String(p_step.get("page_id", "")).strip_edges();
			int idx = int(p_step.get("page_index", 0));
			if (!pid.is_empty()) {
				Dictionary pinfo = p_layer->get_pager_info(tgt);
				Array pids = pinfo.get("page_ids", Array());
				for (int pi = 0; pi < pids.size(); pi++) {
					if (String(pids[pi]).to_lower() == pid.to_lower()) {
						idx = pi;
						break;
					}
				}
			}
			return p_layer->set_pager_active_page(tgt, idx, true, true);
		}
		if (stype == "open_container" || stype == "close_container" || stype == "toggle_container") {
			String tt = String(p_step.get("transition_type", "scale")).strip_edges().to_lower();
			double dur = CLAMP(double(p_step.get("transition_duration", 0.16)), 0.05, 1.5);
			if (stype == "toggle_container") return p_layer->toggle_container_open(tgt, tt, dur);
			return p_layer->set_container_open(tgt, stype == "open_container", tt, dur);
		}
		if (stype == "focus_container") {
			return p_layer->focus_container(tgt);
		}
		if (stype == "set_container_filter") {
			return p_layer->set_container_filter(tgt, String(p_step.get("filter_category", "all")));
		}
		if (stype == "set_container_data_source") {
			return p_layer->set_container_data_source(tgt, String(p_step.get("data_source", "inventory")));
		}
	}
	return false;
}

bool ClockGearsUiDesignLayer::does_input_match_trigger(const Ref<InputEvent> &p_event, const Dictionary &p_trig) {
	if (p_event.is_null() || !p_event->is_pressed() || p_event->is_echo() || p_trig.is_empty()) {
		return false;
	}
	String kind = String(p_trig.get("kind", "key")).strip_edges().to_lower();
	if (kind == "key") {
		Ref<InputEventKey> ek = p_event;
		if (ek.is_null()) return false;
		int64_t kc = int64_t(p_trig.get("keycode", 0));
		if (kc > 0 && ((int64_t)ek->get_keycode() == kc || (int64_t)ek->get_physical_keycode() == kc)) {
			return true;
		}
		String kn = String(p_trig.get("key_name", p_trig.get("key", ""))).strip_edges();
		if (!kn.is_empty()) {
			Key pk = find_keycode(kn);
			if (pk != Key::NONE && (ek->get_keycode() == pk || ek->get_physical_keycode() == pk)) {
				return true;
			}
		}
		return false;
	}
	if (kind == "joy_button") {
		Ref<InputEventJoypadButton> ej = p_event;
		if (ej.is_null()) return false;
		int jb = int(p_trig.get("joy_button", -1));
		return jb >= 0 && ej->get_button_index() == (JoyButton)jb;
	}
	if (kind == "mouse_button") {
		Ref<InputEventMouseButton> em = p_event;
		if (em.is_null() || em->is_double_click()) return false;
		int mb = int(p_trig.get("mouse_button", 0));
		return mb > 0 && (int)em->get_button_index() == mb;
	}
	if (kind == "action") {
		String an = String(p_trig.get("action_name", "")).strip_edges();
		return !an.is_empty() && p_event->is_action_pressed(an, false);
	}
	return false;
}

bool ClockGearsUiDesignLayer::does_input_match_any_trigger(const Ref<InputEvent> &p_event, const Array &p_trigs) {
	for (int i = 0; i < p_trigs.size(); i++) {
		if (p_trigs[i].get_type() == Variant::DICTIONARY && does_input_match_trigger(p_event, Dictionary(p_trigs[i]))) {
			return true;
		}
	}
	return false;
}

void ClockGearsUiDesignLayer::_execute_action_list(const Array &p_actions, const String &p_src_id) {
	for (int i = 0; i < p_actions.size(); i++) {
		if (p_actions[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary st = p_actions[i];
		if (!execute_common_action_step(st, this)) {
			String stype = String(st.get("type", st.get("action", ""))).strip_edges().to_lower();
			if (!stype.is_empty()) {
				Array single;
				single.push_back(st);
				dynamic_slot_actions_by_id["__input_step"] = single;
				emit_signal("action_triggered", stype, "__input_step");
			}
		}
	}
}

void ClockGearsUiDesignLayer::_save_pager_page_state(ElementEntry &r_entry, const String &p_page_id) {
	if (p_page_id.is_empty()) return;
	Dictionary pg_cfg = r_entry.config.has("pager") && r_entry.config["pager"].get_type() == Variant::DICTIONARY
			? Dictionary(r_entry.config["pager"])
			: Dictionary();
	bool rem_scroll = pg_cfg.has("remember_scroll") ? bool(pg_cfg["remember_scroll"]) : true;
	bool rem_focus = pg_cfg.has("remember_focus") ? bool(pg_cfg["remember_focus"]) : true;
	Viewport *vp = get_viewport();
	Control *focus_owner = vp ? vp->gui_get_focus_owner() : nullptr;

	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &ce = elements[i];
		if (ce.clip_ancestor_id != p_page_id && ce.parent_id != p_page_id) continue;
		if (rem_scroll && ce.data_scroll) {
			r_entry.pager_page_scroll[p_page_id] = Vector2(float(ce.data_scroll->get_h_scroll()), float(ce.data_scroll->get_v_scroll()));
		}
		if (rem_focus && focus_owner) {
			if (ce.button == focus_owner) {
				r_entry.pager_page_focus_elem[p_page_id] = ce.id;
			} else if (ce.data_items_canvas) {
				for (int bi = 0; bi < ce.data_cell_buttons.size(); bi++) {
					if (ce.data_cell_buttons[bi] == focus_owner) {
						r_entry.pager_page_focus_elem[p_page_id] = ce.id + "#" + itos(bi);
						break;
					}
				}
			}
		}
	}
}

void ClockGearsUiDesignLayer::_restore_pager_page_state(ElementEntry &r_entry, const String &p_page_id) {
	if (p_page_id.is_empty()) return;
	Dictionary pg_cfg = r_entry.config.has("pager") && r_entry.config["pager"].get_type() == Variant::DICTIONARY
			? Dictionary(r_entry.config["pager"])
			: Dictionary();
	bool rem_scroll = pg_cfg.has("remember_scroll") ? bool(pg_cfg["remember_scroll"]) : true;
	bool rem_focus = pg_cfg.has("remember_focus") ? bool(pg_cfg["remember_focus"]) : true;
	if (rem_scroll && r_entry.pager_page_scroll.has(p_page_id)) {
		Vector2 sv = r_entry.pager_page_scroll[p_page_id];
		for (int i = 0; i < elements.size(); i++) {
			ElementEntry &ce = elements.write[i];
			if ((ce.clip_ancestor_id == p_page_id || ce.parent_id == p_page_id) && ce.data_scroll) {
				ce.data_scroll->set_h_scroll(int(Math::round(sv.x)));
				ce.data_scroll->set_v_scroll(int(Math::round(sv.y)));
				break;
			}
		}
	}
	if (rem_focus && r_entry.pager_page_focus_elem.has(p_page_id)) {
		String tok = r_entry.pager_page_focus_elem[p_page_id];
		int hash_pos = tok.find("#");
		String eid = (hash_pos >= 0) ? tok.substr(0, hash_pos) : tok;
		int slot_i = (hash_pos >= 0) ? tok.substr(hash_pos + 1).to_int() : -1;
		for (int i = 0; i < elements.size(); i++) {
			const ElementEntry &ce = elements[i];
			if (ce.id != eid) continue;
			if (slot_i >= 0 && slot_i < ce.data_cell_buttons.size() && ce.data_cell_buttons[slot_i] && ce.data_cell_buttons[slot_i]->is_visible_in_tree()) {
				ce.data_cell_buttons[slot_i]->grab_focus();
				return;
			}
			if (ce.button && ce.button->is_visible_in_tree()) {
				ce.button->grab_focus();
				return;
			}
		}
	}
}

void ClockGearsUiDesignLayer::_sync_pager_pages_layout_and_vis(ElementEntry &r_entry) {
	if (r_entry.type != "pager") return;
	int cnt = r_entry.pager_page_ids.size();
	if (cnt == 0) return;
	r_entry.pager_active_idx = CLAMP(r_entry.pager_active_idx, 0, cnt - 1);
	String active_pid = r_entry.pager_page_ids[r_entry.pager_active_idx];
	String trans_pid = (r_entry.pager_trans_active && r_entry.pager_trans_from_idx >= 0 && r_entry.pager_trans_from_idx < cnt)
			? r_entry.pager_page_ids[r_entry.pager_trans_from_idx]
			: String();
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &pe = elements.write[i];
		if (pe.type != "folder" || String(pe.config.get("role", "")) != "pager_page" || pe.parent_id != r_entry.id || !pe.root) {
			continue;
		}
		bool show_page = (pe.id == active_pid) || (!trans_pid.is_empty() && pe.id == trans_pid);
		pe.root->set_visible(show_page);
		if (!r_entry.pager_trans_active) {
			pe.root->set_position(Vector2());
			pe.root->set_scale(Vector2(1.0f, 1.0f));
			pe.root->set_modulate(Color(1, 1, 1, 1));
		}
	}
}

void ClockGearsUiDesignLayer::_apply_pager_transition_pose(ElementEntry &r_entry, float p_t) {
	if (r_entry.type != "pager" || !r_entry.pager_pages_clip) return;
	int cnt = r_entry.pager_page_ids.size();
	if (r_entry.pager_active_idx < 0 || r_entry.pager_active_idx >= cnt) return;
	String to_id = r_entry.pager_page_ids[r_entry.pager_active_idx];
	String from_id = (r_entry.pager_trans_from_idx >= 0 && r_entry.pager_trans_from_idx < cnt)
			? r_entry.pager_page_ids[r_entry.pager_trans_from_idx]
			: String();

	Dictionary pg_cfg = r_entry.config.has("pager") && r_entry.config["pager"].get_type() == Variant::DICTIONARY
			? Dictionary(r_entry.config["pager"])
			: ClockGearsDataManager::make_default_pager_config();
	String tt = String(pg_cfg.get("transition_type", "slide")).strip_edges().to_lower();
	String tc = String(pg_cfg.get("transition_curve", "ease_out")).strip_edges().to_lower();
	float t = CLAMP(p_t, 0.0f, 1.0f);
	float e = t;
	if (tc == "ease_out") {
		float inv = 1.0f - t;
		e = 1.0f - inv * inv * inv;
	} else if (tc == "ease_in_out") {
		e = (t < 0.5f) ? (4.0f * t * t * t) : (1.0f - Math::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f);
	} else if (tc == "back") {
		float c1 = 1.70158f;
		float c3 = c1 + 1.0f;
		float tm1 = t - 1.0f;
		e = 1.0f + c3 * tm1 * tm1 * tm1 + c1 * tm1 * tm1;
	}

	Control *from_root = nullptr;
	Control *to_root = nullptr;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == from_id) from_root = elements[i].root;
		if (elements[i].id == to_id) to_root = elements[i].root;
	}
	float w = MAX(32.0f, r_entry.pager_pages_clip->get_size().x);
	float dir = float(r_entry.pager_trans_dir >= 0 ? 1 : -1);
	if (tt == "slide") {
		if (from_root) {
			from_root->set_visible(t < 1.0f);
			from_root->set_position(Vector2(-dir * w * e, 0.0f));
			from_root->set_modulate(Color(1, 1, 1, 1));
			from_root->set_scale(Vector2(1, 1));
		}
		if (to_root) {
			to_root->set_visible(true);
			to_root->set_position(Vector2(dir * w * (1.0f - e), 0.0f));
			to_root->set_modulate(Color(1, 1, 1, 1));
			to_root->set_scale(Vector2(1, 1));
		}
	} else if (tt == "fade") {
		if (from_root) {
			from_root->set_visible(t < 1.0f);
			from_root->set_position(Vector2());
			from_root->set_modulate(Color(1, 1, 1, CLAMP(1.0f - e, 0.0f, 1.0f)));
		}
		if (to_root) {
			to_root->set_visible(true);
			to_root->set_position(Vector2());
			to_root->set_modulate(Color(1, 1, 1, CLAMP(e, 0.0f, 1.0f)));
		}
	} else if (tt == "scale") {
		if (from_root) {
			from_root->set_visible(t < 1.0f);
			from_root->set_pivot_offset(from_root->get_size() * 0.5f);
			float sc = Math::lerp(1.0f, 0.88f, CLAMP(e, 0.0f, 1.0f));
			from_root->set_scale(Vector2(sc, sc));
			from_root->set_modulate(Color(1, 1, 1, CLAMP(1.0f - e, 0.0f, 1.0f)));
		}
		if (to_root) {
			to_root->set_visible(true);
			to_root->set_pivot_offset(to_root->get_size() * 0.5f);
			float sc = Math::lerp(0.88f, 1.0f, CLAMP(e, 0.0f, 1.0f));
			to_root->set_scale(Vector2(sc, sc));
			to_root->set_modulate(Color(1, 1, 1, CLAMP(e, 0.0f, 1.0f)));
		}
	}
}

void ClockGearsUiDesignLayer::_rebuild_pager_indicator(ElementEntry &r_entry, float p_scale) {
	if (r_entry.type != "pager" || !r_entry.pager_indicator_host) return;
	Control *host = r_entry.pager_indicator_host;
	for (int ci = host->get_child_count() - 1; ci >= 0; ci--) {
		Node *ch = host->get_child(ci);
		ch->set_name(vformat("_cg_old_ind_%d", ci));
		host->remove_child(ch);
		ch->queue_free();
	}
	Dictionary pg_cfg = r_entry.config.has("pager") && r_entry.config["pager"].get_type() == Variant::DICTIONARY
			? Dictionary(r_entry.config["pager"])
			: ClockGearsDataManager::make_default_pager_config();
	String im = String(pg_cfg.get("indicator_mode", "tabs")).strip_edges().to_lower();
	if (im == "none") return;
	String ip = String(pg_cfg.get("indicator_pos", "top")).strip_edges().to_lower();
	bool vertical = (ip == "left" || ip == "right");
	bool clickable = pg_cfg.has("click_indicator_switches") ? bool(pg_cfg["click_indicator_switches"]) : true;

	BoxContainer *box = vertical ? static_cast<BoxContainer *>(memnew(VBoxContainer)) : static_cast<BoxContainer *>(memnew(HBoxContainer));
	box->set_name("PagerIndBox");
	box->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	box->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	box->add_theme_constant_override("separation", MAX(2, int(Math::round(6.0f * p_scale))));
	host->add_child(box);

	Array raw_els = active_design.get("elements", Array());
	HashMap<String, Dictionary> page_by_id;
	for (int i = 0; i < raw_els.size(); i++) {
		if (raw_els[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary ed = raw_els[i];
		page_by_id[String(ed.get("id", ""))] = ed;
	}
	bool int_sc = bool(r_entry.config.get("integer_scale", false));

	auto style_ind_btn = [&](Button *p_b, bool p_active) {
		ClockGearsWindowStyleManager::apply_button_style(p_b, window_type_fallback, Dictionary());
		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_bg_color(p_active ? Color(0.24f, 0.17f, 0.08f, 0.94f) : Color(0.08f, 0.10f, 0.14f, 0.80f));
		sb->set_border_color(p_active ? Color(0.96f, 0.74f, 0.32f, 1.0f) : Color(0.45f, 0.48f, 0.56f, 0.55f));
		sb->set_border_width_all(MAX(1, int(Math::round((p_active ? 2.0f : 1.0f) * p_scale))));
		sb->set_corner_radius_all(MAX(4, int(Math::round(6.0f * p_scale))));
		sb->set_content_margin(SIDE_LEFT, 8.0f * p_scale);
		sb->set_content_margin(SIDE_RIGHT, 8.0f * p_scale);
		sb->set_content_margin(SIDE_TOP, 3.0f * p_scale);
		sb->set_content_margin(SIDE_BOTTOM, 3.0f * p_scale);
		p_b->add_theme_style_override("normal", sb);
		p_b->add_theme_style_override("hover", sb);
		p_b->add_theme_style_override("pressed", sb);
		p_b->add_theme_color_override("font_color", p_active ? Color(1.0f, 0.90f, 0.62f, 1.0f) : Color(0.78f, 0.82f, 0.90f, 0.9f));
		p_b->add_theme_font_size_override("font_size", MAX(10, int(Math::round(13.0f * p_scale))));
	};

	if (im == "arrows") {
		Button *b_prev = memnew(Button("<"));
		b_prev->set_name("PagerPrevBtn");
		b_prev->set_focus_mode(FOCUS_NONE);
		style_ind_btn(b_prev, false);
		if (clickable) b_prev->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_pager_arrow_pressed).bind(r_entry.id, -1));
		box->add_child(b_prev);

		String cur_name = "-";
		String cur_icon;
		if (r_entry.pager_active_idx >= 0 && r_entry.pager_active_idx < r_entry.pager_page_ids.size()) {
			String pid = r_entry.pager_page_ids[r_entry.pager_active_idx];
			if (page_by_id.has(pid)) {
				cur_name = String(page_by_id[pid].get("name", pid));
				cur_icon = String(page_by_id[pid].get("page_icon", ""));
			}
		}
		Button *b_mid = memnew(Button(cur_name));
		b_mid->set_focus_mode(FOCUS_NONE);
		b_mid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		if (!cur_icon.is_empty()) {
			Ref<Texture2D> ic = ClockGearsWindowStyleManager::load_texture_safe(cur_icon);
			if (ic.is_valid()) {
				b_mid->set_button_icon(ic);
				b_mid->set_expand_icon(true);
				if (int_sc) b_mid->set_texture_filter(TEXTURE_FILTER_NEAREST);
			}
		}
		style_ind_btn(b_mid, true);
		box->add_child(b_mid);

		Button *b_next = memnew(Button(">"));
		b_next->set_name("PagerNextBtn");
		b_next->set_focus_mode(FOCUS_NONE);
		style_ind_btn(b_next, false);
		if (clickable) b_next->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_pager_arrow_pressed).bind(r_entry.id, 1));
		box->add_child(b_next);
		return;
	}

	for (int i = 0; i < r_entry.pager_page_ids.size(); i++) {
		String pid = r_entry.pager_page_ids[i];
		Dictionary pd = page_by_id.has(pid) ? page_by_id[pid] : Dictionary();
		String pname = String(pd.get("name", String::utf8("Página ") + itos(i + 1)));
		String picon = String(pd.get("page_icon", "")).strip_edges();
		bool is_act = (i == r_entry.pager_active_idx);
		Button *tb = memnew(Button);
		tb->set_name(vformat("PagerInd_%d", i));
		tb->set_focus_mode(FOCUS_NONE);
		tb->set_meta("pager_id", r_entry.id);
		tb->set_meta("page_idx", i);
		if (im == "dots") {
			tb->set_text(is_act ? String::utf8("●") : String::utf8("○"));
			tb->set_custom_minimum_size(Size2(24 * p_scale, 24 * p_scale));
			tb->set_tooltip_text(pname);
		} else {
			tb->set_text(pname);
			tb->set_clip_text(true);
			tb->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			if (!vertical) tb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			if (!picon.is_empty()) {
				Ref<Texture2D> ic = ClockGearsWindowStyleManager::load_texture_safe(picon);
				if (ic.is_valid()) {
					tb->set_button_icon(ic);
					tb->set_expand_icon(true);
					if (int_sc) tb->set_texture_filter(TEXTURE_FILTER_NEAREST);
				}
			}
		}
		style_ind_btn(tb, is_act);
		if (clickable) {
			tb->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_pager_tab_pressed).bind(r_entry.id, i));
		}
		box->add_child(tb);
	}
}

void ClockGearsUiDesignLayer::_on_pager_tab_pressed(const String &p_pager_id, int p_page_idx) {
	set_pager_active_page(p_pager_id, p_page_idx, !preview_mode, !preview_mode);
}

void ClockGearsUiDesignLayer::_on_pager_arrow_pressed(const String &p_pager_id, int p_delta) {
	step_pager_page(p_pager_id, p_delta, !preview_mode);
}

void ClockGearsUiDesignLayer::_on_pager_root_gui_input(const Ref<InputEvent> &p_event, const String &p_pager_id) {
	if (preview_mode || p_event.is_null() || !p_event->is_pressed()) return;
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_null()) return;
	MouseButton bi = mb->get_button_index();
	if (bi != MouseButton::WHEEL_UP && bi != MouseButton::WHEEL_DOWN) return;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].id == p_pager_id && elements[i].type == "pager") {
			Dictionary pg_cfg = elements[i].config.has("pager") && elements[i].config["pager"].get_type() == Variant::DICTIONARY
					? Dictionary(elements[i].config["pager"])
					: Dictionary();
			if (bool(pg_cfg.get("mouse_wheel_switches", false))) {
				step_pager_page(p_pager_id, (bi == MouseButton::WHEEL_DOWN) ? 1 : -1, true);
				if (get_viewport()) get_viewport()->set_input_as_handled();
			}
			break;
		}
	}
}

bool ClockGearsUiDesignLayer::set_pager_active_page(const String &p_pager_id, int p_page_idx, bool p_animate, bool p_play_sound) {
	String low = p_pager_id.strip_edges().to_lower();
	int pe_idx = -1;
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].type == "pager" && (low.is_empty() || elements[i].id.to_lower() == low)) {
			pe_idx = i;
			break;
		}
	}
	if (pe_idx < 0) return false;
	ElementEntry &pe = elements.write[pe_idx];
	int cnt = pe.pager_page_ids.size();
	if (cnt <= 0) return false;
	Dictionary pg_cfg = pe.config.has("pager") && pe.config["pager"].get_type() == Variant::DICTIONARY
			? Dictionary(pe.config["pager"])
			: ClockGearsDataManager::make_default_pager_config();
	bool wrap = pg_cfg.has("wrap_around") ? bool(pg_cfg["wrap_around"]) : true;
	int target = p_page_idx;
	bool wrapped = false;
	if (target < 0) {
		if (!wrap) return false;
		target = cnt - 1;
		wrapped = true;
	} else if (target >= cnt) {
		if (!wrap) return false;
		target = 0;
		wrapped = true;
	}
	int prev = pe.pager_active_idx;
	if (target == prev && !pe.pager_trans_active) {
		return false;
	}
	String prev_pid = (prev >= 0 && prev < cnt) ? pe.pager_page_ids[prev] : String();
	if (!preview_mode && !prev_pid.is_empty()) {
		_save_pager_page_state(pe, prev_pid);
	}
	int dir = 1;
	if (wrapped) {
		dir = (p_page_idx >= cnt) ? 1 : -1;
	} else {
		dir = (target >= prev) ? 1 : -1;
	}
	pe.pager_active_idx = target;
	String next_pid = pe.pager_page_ids[target];
	if (p_play_sound && !preview_mode) {
		String s_wrap = String(pg_cfg.get("sound_wrap", "")).strip_edges();
		String s_chg = String(pg_cfg.get("sound_change", "")).strip_edges();
		String spath = (wrapped && !s_wrap.is_empty()) ? s_wrap : s_chg;
		if (!spath.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_se(spath, 1.0f, 1.0f);
			}
		}
	}

	String tt = String(pg_cfg.get("transition_type", "slide")).strip_edges().to_lower();
	double dur = CLAMP(double(pg_cfg.get("transition_duration", 0.22)), 0.05, 1.5);
	if (p_animate && !preview_mode && tt != "none" && dur > 0.01 && prev != target) {
		pe.pager_trans_active = true;
		pe.pager_trans_from_idx = prev;
		pe.pager_trans_dir = dir;
		pe.pager_trans_elapsed = 0.0;
		pe.pager_trans_duration = dur;
		_sync_pager_pages_layout_and_vis(pe);
		_apply_pager_transition_pose(pe, 0.0f);
	} else {
		pe.pager_trans_active = false;
		pe.pager_trans_from_idx = -1;
		_sync_pager_pages_layout_and_vis(pe);
	}

	float p_acc = 1.0f;
	HashMap<String, float> scales = compute_all_element_scales(active_design.get("elements", Array()));
	if (scales.has(pe.id)) p_acc = scales[pe.id];
	_rebuild_pager_indicator(pe, current_ui_scale * p_acc);

	if (!preview_mode) {
		Array raw_els = active_design.get("elements", Array());
		for (int i = 0; i < raw_els.size(); i++) {
			if (raw_els[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary pd = raw_els[i];
			if (String(pd.get("id", "")) == next_pid && pd.has("enter_actions") && pd["enter_actions"].get_type() == Variant::ARRAY) {
				_execute_action_list(Array(pd["enter_actions"]), next_pid);
				break;
			}
		}
		_restore_pager_page_state(pe, next_pid);
	}
	emit_signal("pager_page_changed", pe.id, target, next_pid);
	return true;
}

bool ClockGearsUiDesignLayer::step_pager_page(const String &p_pager_id, int p_delta, bool p_animate) {
	if (p_delta == 0) return false;
	String low = p_pager_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].type == "pager" && (low.is_empty() || elements[i].id.to_lower() == low)) {
			return set_pager_active_page(elements[i].id, elements[i].pager_active_idx + p_delta, p_animate, true);
		}
	}
	return false;
}

int ClockGearsUiDesignLayer::get_pager_active_page(const String &p_pager_id) const {
	String low = p_pager_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].type == "pager" && (low.is_empty() || elements[i].id.to_lower() == low)) {
			return elements[i].pager_active_idx;
		}
	}
	return -1;
}

Dictionary ClockGearsUiDesignLayer::get_pager_info(const String &p_pager_id) const {
	Dictionary out;
	String low = p_pager_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].type == "pager" && (low.is_empty() || elements[i].id.to_lower() == low)) {
			const ElementEntry &pe = elements[i];
			out["id"] = pe.id;
			out["active_index"] = pe.pager_active_idx;
			out["page_count"] = pe.pager_page_ids.size();
			out["transition_active"] = pe.pager_trans_active;
			Array pids;
			for (int k = 0; k < pe.pager_page_ids.size(); k++) pids.push_back(pe.pager_page_ids[k]);
			out["page_ids"] = pids;
			out["active_page_id"] = (pe.pager_active_idx >= 0 && pe.pager_active_idx < pe.pager_page_ids.size()) ? pe.pager_page_ids[pe.pager_active_idx] : String();
			return out;
		}
	}
	return out;
}

bool ClockGearsUiDesignLayer::_does_element_have_focus(const ElementEntry &p_entry) const {
	if (!p_entry.root || !p_entry.root->is_visible_in_tree()) return false;
	if (!active_focus_container_id.is_empty() && active_focus_container_id.to_lower() == p_entry.id.to_lower()) {
		return true;
	}
	if (Viewport *vp = get_viewport()) {
		if (Control *fo = vp->gui_get_focus_owner()) {
			if (fo == p_entry.root || p_entry.root->is_ancestor_of(fo)) {
				return true;
			}
		}
	}
	return false;
}

bool ClockGearsUiDesignLayer::_are_ancestors_visible_in_tree(const ElementEntry &p_entry) const {
	if (!p_entry.root) return false;
	Node *p = p_entry.root->get_parent();
	if (Control *pc = Object::cast_to<Control>(p)) {
		return pc->is_visible_in_tree();
	}
	return is_visible_in_tree();
}

void ClockGearsUiDesignLayer::_apply_container_transition_pose(ElementEntry &r_entry, float p_t) {
	if (!r_entry.root) return;
	float t = CLAMP(p_t, 0.0f, 1.0f);
	float prog = r_entry.container_trans_showing ? t : (1.0f - t);
	float inv = 1.0f - prog;
	float e = 1.0f - inv * inv * inv;
	String tt = r_entry.container_trans_type.to_lower();
	r_entry.root->set_visible(true);
	if (tt == "fade") {
		r_entry.root->set_scale(Vector2(1, 1));
		r_entry.root->set_position(r_entry.container_base_pos);
		r_entry.root->set_modulate(Color(1, 1, 1, CLAMP(e, 0.0f, 1.0f)));
	} else if (tt == "scale") {
		r_entry.root->set_pivot_offset(r_entry.root->get_size() * 0.5f);
		float sc = Math::lerp(0.75f, 1.0f, CLAMP(e, 0.0f, 1.0f));
		r_entry.root->set_scale(Vector2(sc, sc));
		r_entry.root->set_position(r_entry.container_base_pos);
		r_entry.root->set_modulate(Color(1, 1, 1, CLAMP(e, 0.0f, 1.0f)));
	} else if (tt == "slide") {
		r_entry.root->set_scale(Vector2(1, 1));
		r_entry.root->set_position(r_entry.container_base_pos + Vector2(0.0f, (1.0f - e) * 28.0f * current_ui_scale));
		r_entry.root->set_modulate(Color(1, 1, 1, CLAMP(e, 0.0f, 1.0f)));
	}
}

bool ClockGearsUiDesignLayer::set_container_open(const String &p_elem_id, bool p_open, const String &p_trans, double p_dur) {
	String low = p_elem_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!low.is_empty() && e.id.to_lower() != low) continue;
		if (low.is_empty() && e.type != "container" && e.type != "radial" && e.type != "pager") continue;
		e.runtime_vis_override_set = true;
		e.runtime_vis_override = p_open;
		e.visible = p_open;
		String tt = p_trans.strip_edges().to_lower();
		double dur = CLAMP(p_dur, 0.05, 1.5);
		if (e.root) {
			if (!preview_mode && !tt.is_empty() && tt != "none" && dur > 0.01) {
				e.container_trans_active = true;
				e.container_trans_showing = p_open;
				e.container_trans_type = tt;
				e.container_trans_elapsed = 0.0;
				e.container_trans_duration = dur;
				_apply_container_transition_pose(e, 0.0f);
			} else {
				e.container_trans_active = false;
				e.root->set_scale(Vector2(1, 1));
				e.root->set_modulate(Color(1, 1, 1, 1));
				e.root->set_visible(p_open);
			}
		}
		if (p_open) {
			focus_container(e.id);
		} else if (active_focus_container_id.to_lower() == e.id.to_lower()) {
			active_focus_container_id = String();
			_update_container_focus_visuals();
		} else {
			_update_container_focus_visuals();
		}
		return true;
	}
	return false;
}

bool ClockGearsUiDesignLayer::toggle_container_open(const String &p_elem_id, const String &p_trans, double p_dur) {
	bool cur = is_container_open(p_elem_id);
	return set_container_open(p_elem_id, !cur, p_trans, p_dur);
}

bool ClockGearsUiDesignLayer::is_container_open(const String &p_elem_id) const {
	String low = p_elem_id.strip_edges().to_lower();
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (!low.is_empty() && e.id.to_lower() != low) continue;
		if (low.is_empty() && e.type != "container" && e.type != "radial" && e.type != "pager") continue;
		if (e.container_trans_active) return e.container_trans_showing;
		if (e.runtime_vis_override_set) return e.runtime_vis_override;
		return e.root ? e.root->is_visible() : e.visible;
	}
	return false;
}

void ClockGearsUiDesignLayer::_update_container_focus_visuals() {
	if (preview_mode) {
		if (cursor_overlay) {
			cursor_overlay->set_container_focus(Rect2(), false);
		}
		return;
	}
	ElementEntry *focused_container = nullptr;
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (e.id.to_lower() == active_focus_container_id.to_lower() && e.root && e.root->is_visible_in_tree()) {
			focused_container = &e;
			break;
		}
	}
	if (!focused_container && get_viewport()) {
		if (Control *fo = get_viewport()->gui_get_focus_owner()) {
			for (int i = 0; i < elements.size(); i++) {
				ElementEntry &e = elements.write[i];
				if ((e.type == "container" || e.type == "radial") && e.root && e.root->is_visible_in_tree() &&
						(fo == e.root || e.root->is_ancestor_of(fo))) {
					focused_container = &e;
					active_focus_container_id = e.id;
					break;
				}
			}
		}
	}

	bool has_focused = (focused_container != nullptr);
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if ((e.type != "container" && e.type != "radial") || !e.root || e.container_trans_active) {
			continue;
		}
		bool is_foc = (focused_container == &e);
		bool dim_en = bool(e.config.get("dim_unfocused", false));
		float target_alpha = 1.0f;
		if (has_focused && !is_foc && dim_en) {
			target_alpha = CLAMP(float(e.config.get("unfocused_opacity", 65)) * 0.01f, 0.15f, 1.0f);
		}
		Color mod(1, 1, 1, target_alpha);
		if (e.root->get_modulate() != mod) {
			e.root->set_modulate(mod);
		}
	}

	if (cursor_overlay) {
		if (focused_container && (focused_container->type == "container" || focused_container->type == "radial") && focused_container->root) {
			String fcm = String(focused_container->config.get("focus_cursor_mode", "theme_border")).strip_edges().to_lower();
			Color fcc = _cg_ui_parse_color(focused_container->config.get("focus_cursor_color", "#f5bc54ff"), Color(0.96f, 0.74f, 0.33f, 1.0f));
			float fcw = float(CLAMP(int(focused_container->config.get("focus_cursor_width", 2)), 1, 8));
			Rect2 gr = focused_container->root->get_global_rect();
			Rect2 local_r(gr.position - cursor_overlay->get_global_position(), gr.size);
			cursor_overlay->set_container_focus(local_r, fcm != "none", fcm, fcc, fcw);
		} else {
			cursor_overlay->set_container_focus(Rect2(), false);
		}
	}
}

bool ClockGearsUiDesignLayer::_is_entry_focus_target(const ElementEntry &p_entry) const {
	if (!p_entry.root || !p_entry.root->is_visible_in_tree()) {
		return false;
	}
	if (p_entry.type == "container") {
		for (int i = 0; i < p_entry.data_cell_buttons.size(); i++) {
			Button *b = p_entry.data_cell_buttons[i];
			if (b && b->get_focus_mode() != FOCUS_NONE && b->is_visible_in_tree() && !b->is_disabled()) {
				return true;
			}
		}
		TypedArray<Node> btns = p_entry.root->find_children("*", "Button", true, false);
		for (int bi = 0; bi < btns.size(); bi++) {
			Button *b = Object::cast_to<Button>(btns[bi]);
			if (b && b->get_focus_mode() != FOCUS_NONE && b->is_visible_in_tree() && !b->is_disabled()) {
				return true;
			}
		}
		return false;
	}
	if (p_entry.type == "radial") {
		return true;
	}
	if (p_entry.type == "button" && p_entry.button) {
		return p_entry.button->get_focus_mode() != FOCUS_NONE && !p_entry.button->is_disabled();
	}
	return false;
}

bool ClockGearsUiDesignLayer::_step_within_data_container(ElementEntry &r_entry, int p_dx, int p_dy) {
	if (r_entry.type != "container" || r_entry.data_cell_buttons.is_empty()) {
		return false;
	}
	int n = r_entry.data_cell_buttons.size();
	int cur = CLAMP(r_entry.focused_data_slot_idx, 0, n - 1);
	String lmode = String(r_entry.config.get("layout_mode", "grid")).strip_edges().to_lower();
	int next = -1;
	if (lmode == "vertical_list") {
		if (p_dx != 0) return false;
		next = cur + p_dy;
	} else if (lmode == "horizontal_list" || lmode == "carousel") {
		if (p_dy != 0) return false;
		next = cur + p_dx;
	} else {
		int cols = MAX(1, int(r_entry.config.get("layout_columns", 4)));
		int r = cur / cols;
		int c = cur % cols;
		int nr = r + p_dy;
		int nc = c + p_dx;
		if (nc < 0 || nc >= cols || nr < 0) {
			return false;
		}
		next = nr * cols + nc;
	}
	if (next < 0 || next >= n) {
		return false;
	}
	Button *nb = r_entry.data_cell_buttons[next];
	if (!nb || nb->get_focus_mode() == FOCUS_NONE || !nb->is_visible_in_tree() || nb->is_disabled()) {
		return false;
	}
	nb->grab_focus();
	_on_data_cell_focused(r_entry.id, next);
	return true;
}

bool ClockGearsUiDesignLayer::navigate_focus_direction(int p_dx, int p_dy) {
	if (preview_mode || !is_visible_in_tree() || (p_dx == 0 && p_dy == 0)) {
		return false;
	}
	int cur_idx = -1;
	if (!active_focus_container_id.is_empty()) {
		for (int i = 0; i < elements.size(); i++) {
			if (elements[i].id.to_lower() == active_focus_container_id.to_lower() && elements[i].root && elements[i].root->is_visible_in_tree()) {
				cur_idx = i;
				break;
			}
		}
	}
	if (cur_idx < 0 && get_viewport()) {
		if (Control *fo = get_viewport()->gui_get_focus_owner()) {
			for (int i = 0; i < elements.size(); i++) {
				if (elements[i].root && elements[i].root->is_visible_in_tree() && (fo == elements[i].root || elements[i].root->is_ancestor_of(fo))) {
					if (elements[i].type == "container" || elements[i].type == "button" || elements[i].type == "radial") {
						cur_idx = i;
						break;
					}
				}
			}
		}
	}
	if (cur_idx < 0) {
		for (int i = 0; i < elements.size(); i++) {
			if ((elements[i].type == "container" || elements[i].type == "radial") && _is_entry_focus_target(elements[i])) {
				return focus_container(elements[i].id);
			}
		}
		return false;
	}

	// 1. Paso interno dentro del contenedor de datos actual si no toca su borde
	if (_step_within_data_container(elements.write[cur_idx], p_dx, p_dy)) {
		return true;
	}

	const ElementEntry &cur_e = elements[cur_idx];
	// 2. Enlace manual explícito focus_next_left/right/up/down
	String link_key = (p_dx < 0) ? "focus_next_left" : ((p_dx > 0) ? "focus_next_right" : ((p_dy < 0) ? "focus_next_up" : "focus_next_down"));
	String manual_tgt = String(cur_e.config.get(link_key, "")).strip_edges();
	if (!manual_tgt.is_empty()) {
		for (int i = 0; i < elements.size(); i++) {
			if (elements[i].id.to_lower() == manual_tgt.to_lower() && _is_entry_focus_target(elements[i])) {
				return focus_container(elements[i].id);
			}
		}
	}

	// 3. Búsqueda espacial al vecino más cercano en la dirección (p_dx, p_dy)
	Rect2 cur_r = cur_e.root ? cur_e.root->get_global_rect() : Rect2();
	Vector2 cur_c = cur_r.get_center();
	Vector2 dir{ float(p_dx), float(p_dy) };
	float best_score = 1e18f;
	String best_id;
	for (int i = 0; i < elements.size(); i++) {
		if (i == cur_idx) continue;
		const ElementEntry &cand = elements[i];
		if (!_is_entry_focus_target(cand)) continue;
		if (!cand.parent_id.is_empty() && cand.parent_id.to_lower() == cur_e.id.to_lower()) continue;
		Rect2 br = cand.root->get_global_rect();
		Vector2 d = br.get_center() - cur_c;
		float primary = d.dot(dir);
		if (primary <= 4.0f) continue;
		float perp = Math::abs(d.x * dir.y - d.y * dir.x);
		float score = primary + perp * 2.2f;
		if (score < best_score) {
			best_score = score;
			best_id = cand.id;
		}
	}
	if (!best_id.is_empty()) {
		return focus_container(best_id);
	}
	return false;
}

Dictionary ClockGearsUiDesignLayer::get_container_focus_overlay_state() const {
	Dictionary out;
	if (cursor_overlay) {
		out = cursor_overlay->get_cursor_state();
	}
	out["active_focus_container_id"] = active_focus_container_id;
	Dictionary alphas;
	for (int i = 0; i < elements.size(); i++) {
		if ((elements[i].type == "container" || elements[i].type == "radial") && elements[i].root) {
			alphas[elements[i].id] = elements[i].root->get_modulate().a;
		}
	}
	out["container_alphas"] = alphas;
	return out;
}

bool ClockGearsUiDesignLayer::focus_container(const String &p_elem_id) {
	String low = p_elem_id.strip_edges().to_lower();
	if (low.is_empty()) return false;
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &e = elements[i];
		if (e.id.to_lower() != low || !e.root || !e.root->is_visible_in_tree()) continue;
		active_focus_container_id = e.id;
		bool focused_ok = false;
		if (e.type == "button" && e.button && e.button->get_focus_mode() != FOCUS_NONE) {
			e.button->grab_focus();
			focused_ok = true;
		} else if (e.type == "container") {
			if (!e.data_cell_buttons.is_empty()) {
				int si = CLAMP(e.focused_data_slot_idx, 0, e.data_cell_buttons.size() - 1);
				if (e.data_cell_buttons[si] && e.data_cell_buttons[si]->get_focus_mode() != FOCUS_NONE && e.data_cell_buttons[si]->is_visible_in_tree()) {
					e.data_cell_buttons[si]->grab_focus();
					focused_ok = true;
				}
				if (!focused_ok) {
					for (int bi = 0; bi < e.data_cell_buttons.size(); bi++) {
						if (e.data_cell_buttons[bi] && e.data_cell_buttons[bi]->get_focus_mode() != FOCUS_NONE && e.data_cell_buttons[bi]->is_visible_in_tree()) {
							e.data_cell_buttons[bi]->grab_focus();
							focused_ok = true;
							break;
						}
					}
				}
			}
			if (!focused_ok) {
				TypedArray<Node> btns = e.root->find_children("*", "Button", true, false);
				for (int bi = 0; bi < btns.size(); bi++) {
					Button *b = Object::cast_to<Button>(btns[bi]);
					if (b && b->get_focus_mode() != FOCUS_NONE && b->is_visible_in_tree() && !b->is_disabled()) {
						b->grab_focus();
						focused_ok = true;
						break;
					}
				}
			}
		} else if (e.type == "radial" && e.content_root) {
			if (ClockGearsRadialCanvas *rc = Object::cast_to<ClockGearsRadialCanvas>(e.content_root->find_child("ElemRadialCanvas", false, false))) {
				radial_canvas = rc;
			}
			if (Viewport *vp = get_viewport()) {
				if (Control *fo = vp->gui_get_focus_owner()) {
					fo->release_focus();
				}
			}
			update_cursor_for_button(nullptr);
		} else {
			TypedArray<Node> btns = e.root->find_children("*", "Button", true, false);
			for (int bi = 0; bi < btns.size(); bi++) {
				Button *b = Object::cast_to<Button>(btns[bi]);
				if (b && b->get_focus_mode() != FOCUS_NONE && b->is_visible_in_tree() && !b->is_disabled()) {
					b->grab_focus();
					break;
				}
			}
		}
		_update_container_focus_visuals();
		return true;
	}
	return false;
}

bool ClockGearsUiDesignLayer::set_container_filter(const String &p_elem_id, const String &p_filter_category) {
	String low = p_elem_id.strip_edges().to_lower();
	String fc = p_filter_category.strip_edges().to_lower();
	if (fc.is_empty()) fc = "all";
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!low.is_empty() && e.id.to_lower() != low) continue;
		if (e.type != "container" && e.type != "radial") continue;
		e.config["data_filter_category"] = fc;
		e.focused_data_slot_idx = 0;
		e.data_carousel_offset = 0;
		if (e.data_scroll) {
			e.data_scroll->set_h_scroll(0);
			e.data_scroll->set_v_scroll(0);
		}
		Array raw = active_design.has("elements") ? Array(active_design["elements"]) : Array();
		for (int ri = 0; ri < raw.size(); ri++) {
			if (raw[ri].get_type() == Variant::DICTIONARY && String(Dictionary(raw[ri]).get("id", "")) == e.id) {
				Dictionary rd = raw[ri];
				rd["data_filter_category"] = fc;
				raw[ri] = rd;
				break;
			}
		}
		if (e.type == "container") {
			_build_or_refresh_data_container(e, MAX(0.25f, current_ui_scale), true);
		} else {
			_relayout_elements(true);
		}
		refresh_live_values();
		return true;
	}
	return false;
}

bool ClockGearsUiDesignLayer::set_container_data_source(const String &p_elem_id, const String &p_data_source) {
	String low = p_elem_id.strip_edges().to_lower();
	String ds = ClockGearsDataManager::normalize_ui_data_source(p_data_source);
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!low.is_empty() && e.id.to_lower() != low) continue;
		if (e.type != "container" && e.type != "radial") continue;
		e.config["data_source"] = ds;
		e.focused_data_slot_idx = 0;
		e.data_carousel_offset = 0;
		if (e.data_scroll) {
			e.data_scroll->set_h_scroll(0);
			e.data_scroll->set_v_scroll(0);
		}
		Array raw = active_design.has("elements") ? Array(active_design["elements"]) : Array();
		for (int ri = 0; ri < raw.size(); ri++) {
			if (raw[ri].get_type() == Variant::DICTIONARY && String(Dictionary(raw[ri]).get("id", "")) == e.id) {
				Dictionary rd = raw[ri];
				rd["data_source"] = ds;
				raw[ri] = rd;
				break;
			}
		}
		if (e.type == "container") {
			_build_or_refresh_data_container(e, MAX(0.25f, current_ui_scale), true);
		} else {
			_relayout_elements(true);
		}
		refresh_live_values();
		return true;
	}
	return false;
}

bool ClockGearsUiDesignLayer::handle_runtime_input_event(const Ref<InputEvent> &p_event) {
	if (preview_mode || !is_visible_in_tree() || p_event.is_null() || !p_event->is_pressed() || p_event->is_echo()) {
		return false;
	}

	auto try_elem_inputs = [&](const ElementEntry &p_entry, bool p_only_focused) -> bool {
		if (!p_entry.config.has("inputs") || p_entry.config["inputs"].get_type() != Variant::ARRAY) return false;
		Array ins = p_entry.config["inputs"];
		bool vis_ok = p_entry.root && p_entry.root->is_visible_in_tree();
		bool anc_ok = _are_ancestors_visible_in_tree(p_entry);
		bool has_focus = _does_element_have_focus(p_entry);
		for (int bi = 0; bi < ins.size(); bi++) {
			if (ins[bi].get_type() != Variant::DICTIONARY) continue;
			Dictionary bd = ins[bi];
			bool req_f = bool(bd.get("require_focus", false));
			if (p_only_focused && (!req_f || !has_focus)) continue;
			if (!p_only_focused && req_f) continue;
			Array trigs = bd.get("triggers", Array());
			if (!does_input_match_any_trigger(p_event, trigs)) continue;
			Array acts = bd.get("actions", Array());
			bool can_self_open = false;
			if (!vis_ok && anc_ok && !req_f) {
				for (int ai = 0; ai < acts.size(); ai++) {
					if (acts[ai].get_type() != Variant::DICTIONARY) continue;
					String at = String(Dictionary(acts[ai]).get("type", "")).to_lower();
					String tgt = String(Dictionary(acts[ai]).get("target_elem", "")).to_lower();
					if ((at == "toggle_container" || at == "open_container") && (tgt.is_empty() || tgt == p_entry.id.to_lower())) {
						can_self_open = true;
						break;
					}
				}
			}
			if (!vis_ok && !can_self_open) continue;
			_execute_action_list(acts, p_entry.id);
			return true;
		}
		return false;
	};

	// 1. Entradas con requisito de foco (prioridad al elemento/contenedor enfocado)
	for (int i = elements.size() - 1; i >= 0; i--) {
		if (try_elem_inputs(elements[i], true)) {
			return true;
		}
	}

	// 2. Paginadores visibles (prev_triggers / next_triggers)
	for (int i = 0; i < elements.size(); i++) {
		const ElementEntry &pe = elements[i];
		if (pe.type != "pager" || !pe.visible || !pe.root || !pe.root->is_visible_in_tree()) {
			continue;
		}
		Dictionary pg_cfg = pe.config.has("pager") && pe.config["pager"].get_type() == Variant::DICTIONARY
				? Dictionary(pe.config["pager"])
				: ClockGearsDataManager::make_default_pager_config();
		Array prev_tr = pg_cfg.has("prev_triggers") && pg_cfg["prev_triggers"].get_type() == Variant::ARRAY ? Array(pg_cfg["prev_triggers"]) : Array();
		Array next_tr = pg_cfg.has("next_triggers") && pg_cfg["next_triggers"].get_type() == Variant::ARRAY ? Array(pg_cfg["next_triggers"]) : Array();
		if (does_input_match_any_trigger(p_event, prev_tr)) {
			if (step_pager_page(pe.id, -1, true)) {
				return true;
			}
		} else if (does_input_match_any_trigger(p_event, next_tr)) {
			if (step_pager_page(pe.id, 1, true)) {
				return true;
			}
		}
	}

	// 3. Entradas de elementos/contenedores sin requisito de foco
	for (int i = elements.size() - 1; i >= 0; i--) {
		if (try_elem_inputs(elements[i], false)) {
			return true;
		}
	}

	// 4. Entradas globales de la pantalla
	if (active_design.has("inputs") && active_design["inputs"].get_type() == Variant::ARRAY) {
		Array sins = active_design["inputs"];
		for (int si = 0; si < sins.size(); si++) {
			if (sins[si].get_type() != Variant::DICTIONARY) continue;
			Dictionary bd = sins[si];
			if (does_input_match_any_trigger(p_event, bd.get("triggers", Array()))) {
				_execute_action_list(bd.get("actions", Array()), "_screen");
				return true;
			}
		}
	}
	return false;
}
