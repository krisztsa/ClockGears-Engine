#include "clockgears_ui_design_layer.h"

#include "../core/clockgears_data_manager.h"
#include "clockgears_title_screen.h"
#include "clockgears_window_style.h"
#include "core/io/image.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "scene/gui/box_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/image_texture.h"

namespace {
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
} // namespace

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

void ClockGearsUiDesignLayer::clear_elements() {
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
}

void ClockGearsUiDesignLayer::apply_screen_design(const String &p_screen_id, const Dictionary &p_design_override) {
	screen_id = p_screen_id.strip_edges().to_lower();
	Dictionary design;
	if (!p_design_override.is_empty()) {
		design = ClockGearsDataManager::normalize_ui_screen_design(screen_id, p_design_override);
	} else if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		design = dm->get_ui_screen_design(screen_id);
	} else {
		design = ClockGearsDataManager::get_default_ui_screen_design(screen_id);
	}
	active_design = design;
	_rebuild_from_design(design);
	last_layout_size = Vector2(-1, -1);
	_relayout_elements(true);
	refresh_live_values();
}

void ClockGearsUiDesignLayer::_rebuild_from_design(const Dictionary &p_design) {
	clear_elements();
	Array raw = p_design.get("elements", Array());
	for (int i = 0; i < raw.size(); i++) {
		if (raw[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary cfg = raw[i];
		ElementEntry entry;
		entry.id = String(cfg.get("id", vformat("elem_%d", i + 1)));
		entry.name = String(cfg.get("name", entry.id));
		entry.type = String(cfg.get("type", "text"));
		entry.anchor = String(cfg.get("anchor", "top_left"));
		entry.offset_x = int(cfg.get("offset_x", 0));
		entry.offset_y = int(cfg.get("offset_y", 0));
		entry.width = CLAMP(int(cfg.get("width", 120)), 16, 960);
		entry.height = CLAMP(int(cfg.get("height", 36)), 16, 540);
		entry.layer_order = int(cfg.get("layer_order", 0));
		entry.visible = bool(cfg.get("visible", true));
		entry.window_style_id = String(cfg.get("window_style_id", "")).strip_edges().to_lower();
		entry.data_binding = String(cfg.get("data_binding", "none")).strip_edges().to_lower();
		entry.action = String(cfg.get("action", "")).strip_edges().to_lower();
		entry.focus_order = int(cfg.get("focus_order", i));
		entry.array_order = i;
		entry.config = cfg.duplicate(true);

		_build_element_entry(entry);
		elements.push_back(entry);
	}
}

void ClockGearsUiDesignLayer::_build_element_entry(ElementEntry &r_entry) {
	Control *root = memnew(Control);
	root->set_name(vformat("UiElem_%s", r_entry.id));
	root->set_mouse_filter(r_entry.type == "button" ? MOUSE_FILTER_PASS : MOUSE_FILTER_IGNORE);
	root->set_z_index(r_entry.layer_order);
	root->set_visible(r_entry.visible);
	add_child(root);
	r_entry.root = root;

	PanelContainer *bg = memnew(PanelContainer);
	bg->set_name("BgPanel");
	bg->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	root->add_child(bg);
	r_entry.bg_panel = bg;

	Control *content = memnew(Control);
	content->set_name("ContentRoot");
	content->set_mouse_filter(r_entry.type == "button" ? MOUSE_FILTER_PASS : MOUSE_FILTER_IGNORE);
	root->add_child(content);
	r_entry.content_root = content;

	TextureFilter tf = bool(r_entry.config.get("pixel_art", true)) ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR;

	if (r_entry.type == "frame") {
		float op = CLAMP(float(r_entry.config.get("opacity", 1.0)), 0.0f, 1.0f);
		bg->set_modulate(Color(1, 1, 1, op));
	} else if (r_entry.type == "button") {
		Button *btn = memnew(Button);
		btn->set_name("ActionButton");
		btn->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		btn->set_text(String(r_entry.config.get("text", r_entry.name)));
		btn->set_clip_text(true);
		btn->set_mouse_filter(preview_mode ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
		btn->set_focus_mode(preview_mode ? FOCUS_NONE : FOCUS_ALL);
		btn->set_meta("ui_elem_id", r_entry.id);
		btn->set_meta("ui_action", r_entry.action);
		btn->set_meta("focus_order", r_entry.focus_order);
		String ip = String(r_entry.config.get("image_path", "")).strip_edges();
		if (!ip.is_empty()) {
			Ref<Texture2D> itex = ClockGearsWindowStyleManager::load_texture_safe(ip);
			if (itex.is_valid()) {
				btn->set_button_icon(itex);
			}
		}
		btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignLayer::_on_button_pressed).bind(r_entry.action, r_entry.id));
		content->add_child(btn);
		r_entry.button = btn;
	} else if (r_entry.type == "image") {
		TextureRect *img = memnew(TextureRect);
		img->set_name("Image");
		img->set_mouse_filter(MOUSE_FILTER_IGNORE);
		img->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		img->set_texture_filter(tf);
		String sm = String(r_entry.config.get("scale_mode", "fit"));
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
		float op = CLAMP(float(r_entry.config.get("opacity", 1.0)), 0.0f, 1.0f);
		img->set_modulate(Color(1, 1, 1, op));
		String ip = String(r_entry.config.get("image_path", "")).strip_edges();
		if (!ip.is_empty()) {
			img->set_texture(ClockGearsWindowStyleManager::load_texture_safe(ip));
		}
		content->add_child(img);
		r_entry.image_rect = img;
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
	} else {
		// text o variable
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
	if (r_entry.type == "button" && r_entry.button) {
		Dictionary b_over;
		if (!r_entry.window_style_id.is_empty() && r_entry.window_style_id != "none") {
			if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
				b_over = dm->get_window_style(r_entry.window_style_id);
			}
		}
		ClockGearsWindowStyleManager::apply_button_style(r_entry.button, window_type_fallback, b_over);
		int fsz = MAX(8, int(Math::round(float(r_entry.config.get("font_size", 16)) * p_scale)));
		r_entry.button->add_theme_font_size_override("font_size", fsz);
		r_entry.bg_panel->set_visible(false);
		r_entry.content_root->set_position(Vector2());
		r_entry.content_root->set_size(r_entry.root->get_size());
		return;
	}

	String sid = r_entry.window_style_id;
	bool force_panel = (r_entry.type == "frame");
	if (!force_panel && (sid.is_empty() || sid == "none")) {
		r_entry.bg_panel->set_visible(false);
		r_entry.content_root->set_position(Vector2());
		r_entry.content_root->set_size(r_entry.root->get_size());
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
	ClockGearsWindowStyleManager::apply_window_style(r_entry.bg_panel, window_type_fallback, surface_overrides);
	Ref<StyleBox> sb = r_entry.bg_panel->get_theme_stylebox("panel");
	float pad_l = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_LEFT)) : (4.0f * p_scale);
	float pad_t = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_TOP)) : (4.0f * p_scale);
	float pad_r = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_RIGHT)) : (4.0f * p_scale);
	float pad_b = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_BOTTOM)) : (4.0f * p_scale);
	Size2 rsz = r_entry.root->get_size();
	r_entry.content_root->set_position(Vector2(pad_l, pad_t));
	r_entry.content_root->set_size(Vector2(MAX(4.0f, rsz.x - pad_l - pad_r), MAX(4.0f, rsz.y - pad_t - pad_b)));
}

void ClockGearsUiDesignLayer::_relayout_elements(bool p_refresh_styles) {
	Size2 vp_sz = _resolve_effective_viewport_size();
	if (!p_refresh_styles && vp_sz == last_layout_size) {
		return;
	}
	last_layout_size = vp_sz;
	current_stage_rect = compute_stage_rect(vp_sz);
	current_ui_scale = MAX(0.25f, MIN(vp_sz.x / LOGICAL_WIDTH, vp_sz.y / LOGICAL_HEIGHT));

	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) {
			continue;
		}
		Rect2 er = compute_element_rect(e.config, current_stage_rect, current_ui_scale);
		e.root->set_position(er.position);
		e.root->set_size(er.size);
		e.root->set_z_index(e.layer_order);
		_apply_element_style(e, current_ui_scale);

		if (e.label) {
			int fsz = MAX(8, int(Math::round(float(e.config.get("font_size", 16)) * current_ui_scale)));
			e.label->add_theme_font_size_override("font_size", fsz);
		}
		if (e.bar_label) {
			int fsz = MAX(8, int(Math::round(float(e.config.get("font_size", 13)) * current_ui_scale)));
			e.bar_label->add_theme_font_size_override("font_size", fsz);
		}
		if (e.icon_rect) {
			Size2 csz = e.content_root ? e.content_root->get_size() : er.size;
			float icon_sz = CLAMP(csz.y - 4.0f, 12.0f, 40.0f);
			e.icon_rect->set_custom_minimum_size(Size2(icon_sz, icon_sz));
		}
	}
}

void ClockGearsUiDesignLayer::refresh_live_values() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	for (int i = 0; i < elements.size(); i++) {
		ElementEntry &e = elements.write[i];
		if (!e.root) {
			continue;
		}
		bool vis = e.visible;
		if (vis && dm && String(e.config.get("visibility_mode", "always")) == "switch") {
			String sw = String(e.config.get("visibility_switch", "")).strip_edges();
			if (!sw.is_empty() && !dm->get_switch(sw)) {
				vis = false;
			}
		}

		if (e.type == "text" || e.type == "variable") {
			if (e.label) {
				e.label->set_text(resolve_element_text(e.config, dm));
			}
		} else if (e.type == "button") {
			if (e.button) {
				String btxt = (e.data_binding != "none" && !e.data_binding.is_empty())
						? resolve_element_text(e.config, dm)
						: String(e.config.get("text", e.name));
				e.button->set_text(btxt);
				bool is_dis = bool(e.config.get("disabled", false)) || (!e.action.is_empty() && bool(disabled_actions.get(e.action, false)));
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
	emit_signal("action_triggered", p_action, p_elem_id);
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
		if (is_visible()) {
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
	ClassDB::bind_method(D_METHOD("clear_elements"), &ClockGearsUiDesignLayer::clear_elements);
	ClassDB::bind_method(D_METHOD("refresh_live_values"), &ClockGearsUiDesignLayer::refresh_live_values);
	ClassDB::bind_method(D_METHOD("get_focus_order_actions"), &ClockGearsUiDesignLayer::get_focus_order_actions);
	ClassDB::bind_method(D_METHOD("get_element_control", "elem_id"), &ClockGearsUiDesignLayer::get_element_control);
	ClassDB::bind_method(D_METHOD("get_element_displayed_text", "elem_id"), &ClockGearsUiDesignLayer::get_element_displayed_text);
	ClassDB::bind_method(D_METHOD("get_element_count"), &ClockGearsUiDesignLayer::get_element_count);
	ClassDB::bind_method(D_METHOD("trigger_action", "action"), &ClockGearsUiDesignLayer::trigger_action);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsUiDesignLayer::get_layout_bounds_info, DEFVAL(Vector2()));

	ADD_SIGNAL(MethodInfo("action_triggered", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::STRING, "element_id")));
}

ClockGearsUiDesignLayer::ClockGearsUiDesignLayer() {
	set_mouse_filter(MOUSE_FILTER_IGNORE);
}
