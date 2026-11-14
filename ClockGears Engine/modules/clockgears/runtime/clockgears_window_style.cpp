#include "clockgears_window_style.h"

#include "../core/clockgears_data_manager.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "scene/main/viewport.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/rendering_server.h"

ClockGearsWindowStyleManager *ClockGearsWindowStyleManager::singleton = nullptr;

namespace {
static Color s_parse_html_color(const Variant &p_val, const Color &p_default) {
	if (p_val.get_type() == Variant::COLOR) {
		return Color(p_val);
	}
	String s = String(p_val).strip_edges();
	if (!s.is_empty() && Color::html_is_valid(s)) {
		return Color::html(s);
	}
	return p_default;
}

static void s_merge_surface_overrides(Dictionary &r_surface, const Dictionary &p_overrides) {
	if (p_overrides.is_empty()) {
		return;
	}
	Array keys = p_overrides.keys();
	for (int i = 0; i < keys.size(); i++) {
		Variant k = keys[i];
		r_surface[k] = p_overrides[k];
	}
}

static void s_draw_texture_piece(RID p_canvas_item, const Ref<Texture2D> &p_tex, const Rect2 &p_dst, const Rect2 &p_src, bool p_repeat, float p_scale, const Color &p_modulate) {
	if (!p_tex.is_valid() || p_dst.size.x <= 0.0f || p_dst.size.y <= 0.0f || p_src.size.x <= 0.0f || p_src.size.y <= 0.0f) {
		return;
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) {
		return;
	}
	RID tex_rid = p_tex->get_scaled_rid();
	if (!p_repeat) {
		rs->canvas_item_add_texture_rect_region(p_canvas_item, p_dst, tex_rid, p_src, p_modulate, false, false);
		return;
	}

	float tile_w = MAX(1.0f, p_src.size.x * p_scale);
	float tile_h = MAX(1.0f, p_src.size.y * p_scale);
	for (float y = 0.0f; y < p_dst.size.y - 0.01f; y += tile_h) {
		float draw_h = MIN(tile_h, p_dst.size.y - y);
		float src_h = (draw_h / tile_h) * p_src.size.y;
		for (float x = 0.0f; x < p_dst.size.x - 0.01f; x += tile_w) {
			float draw_w = MIN(tile_w, p_dst.size.x - x);
			float src_w = (draw_w / tile_w) * p_src.size.x;
			Rect2 dst_tile(p_dst.position.x + x, p_dst.position.y + y, draw_w, draw_h);
			Rect2 src_tile(p_src.position.x, p_src.position.y, src_w, src_h);
			rs->canvas_item_add_texture_rect_region(p_canvas_item, dst_tile, tex_rid, src_tile, p_modulate, false, false);
		}
	}
}

static void s_draw_nine_slice(RID p_canvas_item, const Ref<Texture2D> &p_tex, const Rect2 &p_rect, float p_sl, float p_st, float p_sr, float p_sb, float p_scale, bool p_repeat, const Color &p_modulate) {
	if (!p_tex.is_valid() || p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Size2 tex_sz = p_tex->get_size();
	if (tex_sz.x <= 0.0f || tex_sz.y <= 0.0f) {
		return;
	}

	float sl = CLAMP(p_sl, 0.0f, tex_sz.x);
	float sr = CLAMP(p_sr, 0.0f, tex_sz.x - sl);
	float st = CLAMP(p_st, 0.0f, tex_sz.y);
	float sb = CLAMP(p_sb, 0.0f, tex_sz.y - st);

	float smid_w = MAX(0.0f, tex_sz.x - sl - sr);
	float smid_h = MAX(0.0f, tex_sz.y - st - sb);

	float dl = sl * p_scale;
	float dr = sr * p_scale;
	float dt = st * p_scale;
	float db = sb * p_scale;
	if (dl + dr > p_rect.size.x && (dl + dr) > 0.0f) {
		float f = p_rect.size.x / (dl + dr);
		dl *= f;
		dr *= f;
	}
	if (dt + db > p_rect.size.y && (dt + db) > 0.0f) {
		float f = p_rect.size.y / (dt + db);
		dt *= f;
		db *= f;
	}
	float dmid_w = MAX(0.0f, p_rect.size.x - dl - dr);
	float dmid_h = MAX(0.0f, p_rect.size.y - dt - db);

	float sx[3] = { 0.0f, sl, sl + smid_w };
	float sw[3] = { sl, smid_w, sr };
	float sy[3] = { 0.0f, st, st + smid_h };
	float sh[3] = { st, smid_h, sb };

	float dx[3] = { p_rect.position.x, p_rect.position.x + dl, p_rect.position.x + dl + dmid_w };
	float dw[3] = { dl, dmid_w, dr };
	float dy[3] = { p_rect.position.y, p_rect.position.y + dt, p_rect.position.y + dt + dmid_h };
	float dh[3] = { dt, dmid_h, db };

	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			if (dw[col] <= 0.0f || dh[row] <= 0.0f || sw[col] <= 0.0f || sh[row] <= 0.0f) {
				continue;
			}
			bool is_edge_or_center = (row == 1 || col == 1);
			s_draw_texture_piece(p_canvas_item, p_tex, Rect2(dx[col], dy[row], dw[col], dh[row]), Rect2(sx[col], sy[row], sw[col], sh[row]), p_repeat && is_edge_or_center, p_scale, p_modulate);
		}
	}
}
} // namespace

ClockGearsWindowStyleBox::ClockGearsWindowStyleBox() {
	flat_box.instantiate();
	_rebuild_internal();
}

void ClockGearsWindowStyleBox::_rebuild_internal() {
	float s = scale_factor;
	if (pixel_art) {
		s = (float)MAX(1, (int)Math::round(scale_factor));
	} else {
		s = MAX(0.5f, scale_factor);
	}
	scale_factor = s;

	scaled_border_width = (border_type == "none" || base_border_width <= 0) ? 0 : MAX(1, (int)Math::round((float)base_border_width * s));
	scaled_corner_radius = (border_type == "rect" || border_type == "none" || base_corner_radius <= 0) ? 0 : MAX(1, (int)Math::round((float)base_corner_radius * s));

	for (int i = 0; i < 4; i++) {
		float sp = (float)base_padding[i] * s;
		set_content_margin((Side)i, Math::round(sp));
		scaled_nine_slice[i] = (float)base_nine_slice[i];
	}

	if (!flat_box.is_valid()) {
		flat_box.instantiate();
	}
	bool draw_fill_in_flat = (border_type != "image" && fill_type == "color");
	flat_box->set_bg_color(draw_fill_in_flat ? fill_color : Color(0, 0, 0, 0));
	flat_box->set_border_width_all(border_type == "image" ? 0 : scaled_border_width);
	flat_box->set_border_color(border_color);
	flat_box->set_corner_radius_all(scaled_corner_radius);
	flat_box->set_anti_aliased(!pixel_art);
}

bool ClockGearsWindowStyleBox::configure_from_surface(const Dictionary &p_surface, float p_vp_scale, const String &p_style_id, const String &p_window_type_id) {
	style_id = p_style_id.is_empty() ? String(p_surface.get("id", "rounded")) : p_style_id;
	window_type_id = p_window_type_id;
	border_type = String(p_surface.get("border_type", "rounded")).strip_edges().to_lower();
	fill_type = String(p_surface.get("fill_type", "color")).strip_edges().to_lower();
	center_mode = String(p_surface.get("center_mode", "stretch")).strip_edges().to_lower();
	pixel_art = p_surface.has("pixel_art") ? bool(p_surface["pixel_art"]) : true;
	scale_factor = p_vp_scale > 0.0f ? p_vp_scale : 1.0f;

	base_border_width = MAX(0, int(p_surface.get("border_width", 2)));
	base_corner_radius = MAX(0, int(p_surface.get("corner_radius", 8)));
	border_color = s_parse_html_color(p_surface.get("border_color", "#e0a040ff"), Color(0.88f, 0.63f, 0.25f, 1.0f));
	fill_color = s_parse_html_color(p_surface.get("fill_color", "#14171ff0"), Color(0.08f, 0.09f, 0.12f, 0.94f));
	font_color = s_parse_html_color(p_surface.get("font_color", "#eef2faff"), Color(0.93f, 0.95f, 0.98f, 1.0f));

	Dictionary pad = (p_surface.has("padding") && p_surface["padding"].get_type() == Variant::DICTIONARY) ? Dictionary(p_surface["padding"]) : Dictionary();
	base_padding[SIDE_LEFT] = MAX(0, int(pad.get("left", 12)));
	base_padding[SIDE_TOP] = MAX(0, int(pad.get("top", 10)));
	base_padding[SIDE_RIGHT] = MAX(0, int(pad.get("right", 12)));
	base_padding[SIDE_BOTTOM] = MAX(0, int(pad.get("bottom", 10)));

	Dictionary slice = (p_surface.has("nine_slice") && p_surface["nine_slice"].get_type() == Variant::DICTIONARY) ? Dictionary(p_surface["nine_slice"]) : Dictionary();
	base_nine_slice[SIDE_LEFT] = MAX(0, int(slice.get("left", 8)));
	base_nine_slice[SIDE_TOP] = MAX(0, int(slice.get("top", 8)));
	base_nine_slice[SIDE_RIGHT] = MAX(0, int(slice.get("right", 8)));
	base_nine_slice[SIDE_BOTTOM] = MAX(0, int(slice.get("bottom", 8)));

	image_path = String(p_surface.get("image_path", "")).strip_edges();
	fill_image_path = String(p_surface.get("fill_image_path", "")).strip_edges();
	frame_texture.unref();
	fill_texture.unref();

	if (border_type == "image") {
		frame_texture = ClockGearsWindowStyleManager::load_texture_safe(image_path);
		if (frame_texture.is_null()) {
			return false;
		}
	}
	if (fill_type == "image") {
		fill_texture = ClockGearsWindowStyleManager::load_texture_safe(fill_image_path);
		if (fill_texture.is_null()) {
			return false;
		}
	}

	_rebuild_internal();
	emit_changed();
	return true;
}

float ClockGearsWindowStyleBox::get_style_margin(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0f);
	return (float)base_padding[p_side] * scale_factor;
}

void ClockGearsWindowStyleBox::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) {
		return;
	}
	bool repeat = (center_mode == "repeat");

	if (border_type == "image" && frame_texture.is_valid()) {
		if (fill_type == "image" && fill_texture.is_valid()) {
			s_draw_texture_piece(p_canvas_item, fill_texture, p_rect, Rect2(Vector2(), fill_texture->get_size()), repeat, scale_factor, Color(1, 1, 1, 1));
		} else if (fill_color.a > 0.001f) {
			rs->canvas_item_add_rect(p_canvas_item, p_rect, fill_color);
		}
		s_draw_nine_slice(p_canvas_item, frame_texture, p_rect,
				(float)base_nine_slice[SIDE_LEFT], (float)base_nine_slice[SIDE_TOP],
				(float)base_nine_slice[SIDE_RIGHT], (float)base_nine_slice[SIDE_BOTTOM],
				scale_factor, repeat, Color(1, 1, 1, 1));
		return;
	}

	if (fill_type == "image" && fill_texture.is_valid()) {
		s_draw_texture_piece(p_canvas_item, fill_texture, p_rect, Rect2(Vector2(), fill_texture->get_size()), repeat, scale_factor, Color(1, 1, 1, 1));
	}
	if (flat_box.is_valid()) {
		flat_box->draw(p_canvas_item, p_rect);
	}
}

Dictionary ClockGearsWindowStyleBox::get_scaled_padding() const {
	Dictionary out;
	out["left"] = (int)get_content_margin(SIDE_LEFT);
	out["top"] = (int)get_content_margin(SIDE_TOP);
	out["right"] = (int)get_content_margin(SIDE_RIGHT);
	out["bottom"] = (int)get_content_margin(SIDE_BOTTOM);
	return out;
}

Dictionary ClockGearsWindowStyleBox::get_scaled_nine_slice() const {
	Dictionary out;
	out["left"] = (int)Math::round((float)base_nine_slice[SIDE_LEFT] * scale_factor);
	out["top"] = (int)Math::round((float)base_nine_slice[SIDE_TOP] * scale_factor);
	out["right"] = (int)Math::round((float)base_nine_slice[SIDE_RIGHT] * scale_factor);
	out["bottom"] = (int)Math::round((float)base_nine_slice[SIDE_BOTTOM] * scale_factor);
	return out;
}

void ClockGearsWindowStyleBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_style_id"), &ClockGearsWindowStyleBox::get_style_id);
	ClassDB::bind_method(D_METHOD("get_window_type_id"), &ClockGearsWindowStyleBox::get_window_type_id);
	ClassDB::bind_method(D_METHOD("get_border_type"), &ClockGearsWindowStyleBox::get_border_type);
	ClassDB::bind_method(D_METHOD("get_fill_type"), &ClockGearsWindowStyleBox::get_fill_type);
	ClassDB::bind_method(D_METHOD("get_center_mode"), &ClockGearsWindowStyleBox::get_center_mode);
	ClassDB::bind_method(D_METHOD("is_pixel_art"), &ClockGearsWindowStyleBox::is_pixel_art);
	ClassDB::bind_method(D_METHOD("was_fallback_used"), &ClockGearsWindowStyleBox::was_fallback_used);
	ClassDB::bind_method(D_METHOD("get_scale_factor"), &ClockGearsWindowStyleBox::get_scale_factor);
	ClassDB::bind_method(D_METHOD("get_scaled_border_width"), &ClockGearsWindowStyleBox::get_scaled_border_width);
	ClassDB::bind_method(D_METHOD("get_scaled_corner_radius"), &ClockGearsWindowStyleBox::get_scaled_corner_radius);
	ClassDB::bind_method(D_METHOD("get_border_color"), &ClockGearsWindowStyleBox::get_border_color);
	ClassDB::bind_method(D_METHOD("get_fill_color"), &ClockGearsWindowStyleBox::get_fill_color);
	ClassDB::bind_method(D_METHOD("get_font_color"), &ClockGearsWindowStyleBox::get_font_color);
	ClassDB::bind_method(D_METHOD("get_image_path"), &ClockGearsWindowStyleBox::get_image_path);
	ClassDB::bind_method(D_METHOD("get_fill_image_path"), &ClockGearsWindowStyleBox::get_fill_image_path);
	ClassDB::bind_method(D_METHOD("get_scaled_padding"), &ClockGearsWindowStyleBox::get_scaled_padding);
	ClassDB::bind_method(D_METHOD("get_scaled_nine_slice"), &ClockGearsWindowStyleBox::get_scaled_nine_slice);
}

// --- ClockGearsWindowStyleManager ---

ClockGearsWindowStyleManager::ClockGearsWindowStyleManager() {
	singleton = this;
}

ClockGearsWindowStyleManager::~ClockGearsWindowStyleManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

float ClockGearsWindowStyleManager::compute_viewport_scale(const Control *p_control, bool p_pixel_art) {
	Size2 vp_size(960.0f, 540.0f);
	if (p_control) {
		Viewport *vp = p_control->get_viewport();
		if (vp) {
			Rect2 r = vp->get_visible_rect();
			if (r.size.x > 64.0f && r.size.y > 64.0f) {
				vp_size = r.size;
			}
		} else if (p_control->get_size().x > 64.0f && p_control->get_size().y > 64.0f) {
			vp_size = p_control->get_size();
		}
	}
	float raw = MAX(1.0f, MIN(vp_size.x / 960.0f, vp_size.y / 540.0f));
	if (p_pixel_art) {
		return (float)MAX(1, (int)Math::round(raw));
	}
	return raw;
}

Ref<Texture2D> ClockGearsWindowStyleManager::load_texture_safe(const String &p_path) {
	String path = p_path.strip_edges();
	if (path.is_empty()) {
		return Ref<Texture2D>();
	}
	if (ResourceLoader::exists(path)) {
		Ref<Texture2D> tex = ResourceLoader::load(path);
		if (tex.is_valid()) {
			return tex;
		}
	}
	if (FileAccess::exists(path)) {
		Ref<Image> img = Image::load_from_file(path);
		if (img.is_valid() && !img->is_empty()) {
			return ImageTexture::create_from_image(img);
		}
	}
	return Ref<Texture2D>();
}

static Dictionary s_resolve_style_dict(const String &p_window_type_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		return dm->resolve_window_style(p_window_type_id);
	}
	Dictionary cfg = ClockGearsDataManager::get_default_window_styles_config();
	Array styles = cfg["styles"];
	return styles.is_empty() ? Dictionary() : Dictionary(styles[0]);
}

static Dictionary s_get_rounded_fallback_dict() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Dictionary s = dm->get_window_style("rounded");
		if (!s.is_empty()) {
			return s;
		}
	}
	Dictionary cfg = ClockGearsDataManager::get_default_window_styles_config();
	Array styles = cfg["styles"];
	return styles.is_empty() ? Dictionary() : Dictionary(styles[0]);
}

Ref<ClockGearsWindowStyleBox> ClockGearsWindowStyleManager::build_window_style_box(const String &p_window_type_id, float p_vp_scale, const Dictionary &p_overrides) {
	Dictionary style = s_resolve_style_dict(p_window_type_id);
	Dictionary surface = style.duplicate(true);
	s_merge_surface_overrides(surface, p_overrides);

	Ref<ClockGearsWindowStyleBox> sb;
	sb.instantiate();
	if (!sb->configure_from_surface(surface, p_vp_scale, String(style.get("id", "rounded")), p_window_type_id)) {
		Dictionary fb = s_get_rounded_fallback_dict();
		Dictionary fb_surface = fb.duplicate(true);
		s_merge_surface_overrides(fb_surface, p_overrides);
		sb->configure_from_surface(fb_surface, p_vp_scale, "rounded", p_window_type_id);
		sb->set_fallback_used(true);
	}
	return sb;
}

Ref<ClockGearsWindowStyleBox> ClockGearsWindowStyleManager::build_button_style_box(const String &p_window_type_id, const String &p_state, float p_vp_scale, const Dictionary &p_overrides) {
	Dictionary style = s_resolve_style_dict(p_window_type_id);
	Dictionary btns = (style.has("buttons") && style["buttons"].get_type() == Variant::DICTIONARY) ? Dictionary(style["buttons"]) : Dictionary();
	String state_key = p_state.strip_edges().to_lower();
	if (!btns.has(state_key)) {
		state_key = "normal";
	}
	Dictionary surface = btns.has(state_key) ? Dictionary(btns[state_key]).duplicate(true) : style.duplicate(true);
	if (bool(surface.get("inherit_normal", false)) && btns.has("normal")) {
		surface = Dictionary(btns["normal"]).duplicate(true);
	}
	s_merge_surface_overrides(surface, p_overrides);

	Ref<ClockGearsWindowStyleBox> sb;
	sb.instantiate();
	if (!sb->configure_from_surface(surface, p_vp_scale, String(style.get("id", "rounded")), p_window_type_id)) {
		Dictionary fb = s_get_rounded_fallback_dict();
		Dictionary fb_btns = fb.get("buttons", Dictionary());
		Dictionary fb_surface = fb_btns.has(state_key) ? Dictionary(fb_btns[state_key]).duplicate(true) : fb.duplicate(true);
		s_merge_surface_overrides(fb_surface, p_overrides);
		sb->configure_from_surface(fb_surface, p_vp_scale, "rounded", p_window_type_id);
		sb->set_fallback_used(true);
	}
	return sb;
}

void ClockGearsWindowStyleManager::apply_window_style(Control *p_control, const String &p_window_type_id, const Dictionary &p_overrides) {
	if (!p_control) {
		return;
	}
	Dictionary style = s_resolve_style_dict(p_window_type_id);
	bool pa = style.has("pixel_art") ? bool(style["pixel_art"]) : true;
	float scale = compute_viewport_scale(p_control, pa);
	Ref<ClockGearsWindowStyleBox> sb = build_window_style_box(p_window_type_id, scale, p_overrides);

	p_control->add_theme_style_override("panel", sb);
	p_control->set_texture_filter(sb->is_pixel_art() ? CanvasItem::TEXTURE_FILTER_NEAREST : CanvasItem::TEXTURE_FILTER_LINEAR);
	p_control->set_meta("_cg_window_type_id", p_window_type_id);
	p_control->set_meta("_cg_applied_style_id", sb->get_style_id());
	p_control->set_meta("_cg_style_fallback", sb->was_fallback_used());
}

void ClockGearsWindowStyleManager::apply_button_style(Button *p_button, const String &p_window_type_id, const Dictionary &p_overrides) {
	if (!p_button) {
		return;
	}
	Dictionary style = s_resolve_style_dict(p_window_type_id);
	Dictionary btns = style.get("buttons", Dictionary());
	Dictionary norm = btns.get("normal", style);
	bool pa = norm.has("pixel_art") ? bool(norm["pixel_art"]) : true;
	float scale = compute_viewport_scale(p_button, pa);

	Ref<ClockGearsWindowStyleBox> sb_norm = build_button_style_box(p_window_type_id, "normal", scale, p_overrides);
	Ref<ClockGearsWindowStyleBox> sb_hover = build_button_style_box(p_window_type_id, "hover", scale, p_overrides);
	Ref<ClockGearsWindowStyleBox> sb_pressed = build_button_style_box(p_window_type_id, "pressed", scale, p_overrides);
	Ref<ClockGearsWindowStyleBox> sb_disabled = build_button_style_box(p_window_type_id, "disabled", scale, p_overrides);

	p_button->add_theme_style_override("normal", sb_norm);
	p_button->add_theme_style_override("hover", sb_hover);
	p_button->add_theme_style_override("pressed", sb_pressed);
	p_button->add_theme_style_override("hover_pressed", sb_pressed);
	p_button->add_theme_style_override("disabled", sb_disabled);
	p_button->add_theme_style_override("focus", sb_hover);

	p_button->add_theme_color_override("font_color", sb_norm->get_font_color());
	p_button->add_theme_color_override("font_hover_color", sb_hover->get_font_color());
	p_button->add_theme_color_override("font_pressed_color", sb_pressed->get_font_color());
	p_button->add_theme_color_override("font_hover_pressed_color", sb_pressed->get_font_color());
	p_button->add_theme_color_override("font_disabled_color", sb_disabled->get_font_color());
	p_button->add_theme_color_override("font_focus_color", sb_hover->get_font_color());

	p_button->set_texture_filter(sb_norm->is_pixel_art() ? CanvasItem::TEXTURE_FILTER_NEAREST : CanvasItem::TEXTURE_FILTER_LINEAR);
	p_button->set_meta("_cg_window_type_id", p_window_type_id);
	p_button->set_meta("_cg_applied_style_id", sb_norm->get_style_id());
	p_button->set_meta("_cg_style_fallback", sb_norm->was_fallback_used());
}

void ClockGearsWindowStyleManager::apply_buttons_in_container(Node *p_container, const String &p_window_type_id, const Dictionary &p_overrides) {
	if (!p_container) {
		return;
	}
	for (int i = 0; i < p_container->get_child_count(); i++) {
		Button *b = Object::cast_to<Button>(p_container->get_child(i));
		if (b) {
			apply_button_style(b, p_window_type_id, p_overrides);
		}
	}
}

Dictionary ClockGearsWindowStyleManager::inspect_control_style(const Control *p_control) {
	Dictionary out;
	if (!p_control) {
		return out;
	}
	Ref<StyleBox> sb;
	Button *btn = Object::cast_to<Button>(const_cast<Control *>(p_control));
	if (btn) {
		sb = btn->get_theme_stylebox("normal");
	} else {
		sb = p_control->get_theme_stylebox("panel");
	}
	Ref<ClockGearsWindowStyleBox> cg_sb = sb;
	if (cg_sb.is_valid()) {
		out["unified"] = true;
		out["style_id"] = cg_sb->get_style_id();
		out["window_type_id"] = cg_sb->get_window_type_id();
		out["border_type"] = cg_sb->get_border_type();
		out["fill_type"] = cg_sb->get_fill_type();
		out["center_mode"] = cg_sb->get_center_mode();
		out["pixel_art"] = cg_sb->is_pixel_art();
		out["fallback_used"] = p_control->has_meta("_cg_style_fallback") ? bool(p_control->get_meta("_cg_style_fallback")) : cg_sb->was_fallback_used();
		out["scale_factor"] = cg_sb->get_scale_factor();
		out["border_width"] = cg_sb->get_scaled_border_width();
		out["corner_radius"] = cg_sb->get_scaled_corner_radius();
		out["border_color"] = cg_sb->get_border_color();
		out["fill_color"] = cg_sb->get_fill_color();
		out["padding"] = cg_sb->get_scaled_padding();
		out["nine_slice"] = cg_sb->get_scaled_nine_slice();
		out["image_path"] = cg_sb->get_image_path();
	} else {
		out["unified"] = false;
	}
	return out;
}

void ClockGearsWindowStyleManager::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsWindowStyleManager", D_METHOD("compute_viewport_scale", "control", "pixel_art"), &ClockGearsWindowStyleManager::compute_viewport_scale, DEFVAL(true));
	ClassDB::bind_static_method("ClockGearsWindowStyleManager", D_METHOD("build_window_style_box", "window_type_id", "vp_scale", "overrides"), &ClockGearsWindowStyleManager::build_window_style_box, DEFVAL(1.0f), DEFVAL(Dictionary()));
	ClassDB::bind_static_method("ClockGearsWindowStyleManager", D_METHOD("build_button_style_box", "window_type_id", "state", "vp_scale", "overrides"), &ClockGearsWindowStyleManager::build_button_style_box, DEFVAL("normal"), DEFVAL(1.0f), DEFVAL(Dictionary()));
	ClassDB::bind_static_method("ClockGearsWindowStyleManager", D_METHOD("apply_window_style", "control", "window_type_id", "overrides"), &ClockGearsWindowStyleManager::apply_window_style, DEFVAL(Dictionary()));
	ClassDB::bind_static_method("ClockGearsWindowStyleManager", D_METHOD("apply_button_style", "button", "window_type_id", "overrides"), &ClockGearsWindowStyleManager::apply_button_style, DEFVAL(Dictionary()));
	ClassDB::bind_static_method("ClockGearsWindowStyleManager", D_METHOD("inspect_control_style", "control"), &ClockGearsWindowStyleManager::inspect_control_style);
}
