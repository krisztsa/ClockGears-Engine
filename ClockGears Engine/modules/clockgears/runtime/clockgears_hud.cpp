#include "clockgears_hud.h"

#include "../core/clockgears_data_manager.h"
#include "../core/clockgears_formulas.h"
#include "clockgears_window_style.h"
#include "core/io/image.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "scene/main/viewport.h"

namespace {
static Color s_hud_parse_color(const Variant &p_val, const Color &p_default) {
	if (p_val.get_type() == Variant::COLOR) {
		return Color(p_val);
	}
	String s = String(p_val).strip_edges();
	if (!s.is_empty() && Color::html_is_valid(s)) {
		return Color::html(s);
	}
	return p_default;
}

static Vector2 s_anchor_factors(const String &p_anchor) {
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

static Ref<ImageTexture> s_make_heart_texture(int p_mode) {
	// p_mode: 0 = empty, 1 = half, 2 = full (16x16 pixel art)
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
				if (!filled) {
					img->set_pixel(x, y, empty_c);
				} else {
					img->set_pixel(x, y, (ch == 'H') ? hi_c : full_c);
				}
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

static Ref<ImageTexture> s_make_coin_texture() {
	static const char *k_mask[16] = {
		"................",
		".....OOOOOO.....",
		"...OOHHHHHHOO...",
		"..OHHGGGGGGHHO..",
		".OHGGGGGGGGGGHO.",
		".OGGGGYYYYGGGGO.",
		"OGGGGGYYYYGGGGGO",
		"OGGGGGYYYYGGGGGO",
		"OGGGGGYYYYGGGGGO",
		"OGGGGGYYYYGGGGGO",
		".OGGGGYYYYGGGGO.",
		".OHGGGGGGGGGGHO.",
		"..OHHGGGGGGHHO..",
		"...OOHHHHHHOO...",
		".....OOOOOO.....",
		"................"
	};
	Ref<Image> img = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			char ch = k_mask[y][x];
			if (ch == '.') {
				img->set_pixel(x, y, Color(0, 0, 0, 0));
			} else if (ch == 'O') {
				img->set_pixel(x, y, Color(0.28f, 0.16f, 0.03f, 1.0f));
			} else if (ch == 'H') {
				img->set_pixel(x, y, Color(1.0f, 0.94f, 0.50f, 1.0f));
			} else if (ch == 'Y') {
				img->set_pixel(x, y, Color(0.84f, 0.56f, 0.08f, 1.0f));
			} else {
				img->set_pixel(x, y, Color(0.98f, 0.80f, 0.18f, 1.0f));
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

static Ref<ImageTexture> s_make_item_texture() {
	static const char *k_mask[16] = {
		"................",
		".....OOOOOO.....",
		".....OCCCCC.....",
		"......OSSO......",
		"......OSSO......",
		"....OOBBBBOO....",
		"...OBBLLLLBBO...",
		"..OBLLLLLLLLBO..",
		"..OBLLLHHLLLBO..",
		"..OBLLLLLLLLBO..",
		"..OBLLLLLLLLBO..",
		"...OBLLLLLLBO...",
		"....OBBBBBBO....",
		".....OOOOOO.....",
		"................",
		"................"
	};
	Ref<Image> img = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			char ch = k_mask[y][x];
			if (ch == '.') {
				img->set_pixel(x, y, Color(0, 0, 0, 0));
			} else if (ch == 'O') {
				img->set_pixel(x, y, Color(0.10f, 0.12f, 0.18f, 1.0f));
			} else if (ch == 'C') {
				img->set_pixel(x, y, Color(0.75f, 0.56f, 0.32f, 1.0f));
			} else if (ch == 'S') {
				img->set_pixel(x, y, Color(0.78f, 0.86f, 0.94f, 1.0f));
			} else if (ch == 'H') {
				img->set_pixel(x, y, Color(0.82f, 0.96f, 1.0f, 1.0f));
			} else if (ch == 'B') {
				img->set_pixel(x, y, Color(0.24f, 0.44f, 0.78f, 1.0f));
			} else {
				img->set_pixel(x, y, Color(0.30f, 0.72f, 0.94f, 1.0f));
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

static bool s_item_keys_equivalent(ClockGearsDataManager *p_dm, const String &p_widget_key, const String &p_changed_key) {
	String wk = p_widget_key.strip_edges();
	String ck = p_changed_key.strip_edges();
	if (wk.is_empty() || ck.is_empty()) {
		return false;
	}
	if (wk == ck || wk.to_lower() == ck.to_lower()) {
		return true;
	}
	if (!p_dm) {
		return false;
	}
	Dictionary iw = p_dm->find_inventory_record(wk);
	Dictionary ic = p_dm->find_inventory_record(ck);
	if (bool(iw.get("found", false)) && bool(ic.get("found", false))) {
		return int(iw.get("category", -1)) == int(ic.get("category", -2)) && int(iw.get("id", 0)) == int(ic.get("id", -1));
	}
	return false;
}

static int s_resolve_item_live_qty(ClockGearsDataManager *p_dm, const String &p_item_key, Dictionary &r_info) {
	if (!p_dm) {
		return 0;
	}
	r_info = p_dm->find_inventory_record(p_item_key);
	int qty = p_dm->get_item_qty(p_item_key);
	if (qty > 0 || !bool(r_info.get("found", false))) {
		return qty;
	}
	int cat = int(r_info.get("category", -1));
	int rec_id = int(r_info.get("id", 0));
	String cat_key = String(r_info.get("category_key", ""));
	if (!cat_key.is_empty() && rec_id > 0) {
		int q2 = p_dm->get_item_qty(cat_key + ":" + itos(rec_id));
		if (q2 > 0) {
			return q2;
		}
	}
	Dictionary rec = r_info.get("record", Dictionary());
	String rname = String(rec.get("name", "")).strip_edges();
	if (!rname.is_empty()) {
		int q3 = p_dm->get_item_qty(rname);
		if (q3 > 0) {
			return q3;
		}
	}
	if (cat >= 0 && rec_id > 0) {
		String k4 = p_dm->get_record_inventory_key(cat, rec_id);
		if (!k4.is_empty()) {
			return p_dm->get_item_qty(k4);
		}
	}
	return 0;
}
} // namespace

ClockGearsHudOverlay::ClockGearsHudOverlay() {
	set_name("ClockGearsHudOverlay");
	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	hud_config = ClockGearsDataManager::get_default_hud_config();
}

ClockGearsHudOverlay::~ClockGearsHudOverlay() {
	_disconnect_signals();
}

void ClockGearsHudOverlay::_ensure_fallback_textures() {
	if (fallback_heart_full.is_null()) {
		fallback_heart_empty = s_make_heart_texture(0);
		fallback_heart_half = s_make_heart_texture(1);
		fallback_heart_full = s_make_heart_texture(2);
		fallback_coin_tex = s_make_coin_texture();
		fallback_item_tex = s_make_item_texture();
	}
}

void ClockGearsHudOverlay::_connect_signals() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !dm_signals_connected) {
		Callable cb_rt = callable_mp(this, &ClockGearsHudOverlay::_on_runtime_state_changed);
		if (!dm->is_connected("runtime_state_changed", cb_rt)) {
			dm->connect("runtime_state_changed", cb_rt);
		}
		Callable cb_dc = callable_mp(this, &ClockGearsHudOverlay::_on_data_changed);
		if (!dm->is_connected("data_changed", cb_dc)) {
			dm->connect("data_changed", cb_dc);
		}
		dm_signals_connected = true;
	}
	Viewport *vp = is_inside_tree() ? get_viewport() : nullptr;
	if (vp && connected_viewport_id != vp->get_instance_id()) {
		if (connected_viewport_id.is_valid()) {
			Object *old_obj = ObjectDB::get_instance(connected_viewport_id);
			Viewport *old_vp = Object::cast_to<Viewport>(old_obj);
			if (old_vp) {
				Callable cb_old = callable_mp(this, &ClockGearsHudOverlay::_on_viewport_resized);
				if (old_vp->is_connected("size_changed", cb_old)) {
					old_vp->disconnect("size_changed", cb_old);
				}
			}
		}
		Callable cb_vp = callable_mp(this, &ClockGearsHudOverlay::_on_viewport_resized);
		if (!vp->is_connected("size_changed", cb_vp)) {
			vp->connect("size_changed", cb_vp);
		}
		connected_viewport_id = vp->get_instance_id();
	}
}

void ClockGearsHudOverlay::_disconnect_signals() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && dm_signals_connected) {
		Callable cb_rt = callable_mp(this, &ClockGearsHudOverlay::_on_runtime_state_changed);
		if (dm->is_connected("runtime_state_changed", cb_rt)) {
			dm->disconnect("runtime_state_changed", cb_rt);
		}
		Callable cb_dc = callable_mp(this, &ClockGearsHudOverlay::_on_data_changed);
		if (dm->is_connected("data_changed", cb_dc)) {
			dm->disconnect("data_changed", cb_dc);
		}
	}
	dm_signals_connected = false;
	if (connected_viewport_id.is_valid()) {
		Object *old_obj = ObjectDB::get_instance(connected_viewport_id);
		Viewport *old_vp = Object::cast_to<Viewport>(old_obj);
		if (old_vp) {
			Callable cb_old = callable_mp(this, &ClockGearsHudOverlay::_on_viewport_resized);
			if (old_vp->is_connected("size_changed", cb_old)) {
				old_vp->disconnect("size_changed", cb_old);
			}
		}
		connected_viewport_id = ObjectID();
	}
}

void ClockGearsHudOverlay::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_connect_signals();
			if (!has_explicit_config) {
				reload_from_data_manager();
			} else {
				_relayout_widgets(true);
			}
		} break;
		case NOTIFICATION_READY: {
			_connect_signals();
			_relayout_widgets(true);
		} break;
		case NOTIFICATION_RESIZED: {
			_relayout_widgets(false);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_disconnect_signals();
		} break;
	}
}

void ClockGearsHudOverlay::_clear_widgets() {
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i].root) {
			if (widgets[i].root->get_parent() == this) {
				remove_child(widgets[i].root);
			}
			widgets[i].root->queue_free();
		}
	}
	widgets.clear();
}

void ClockGearsHudOverlay::apply_hud_config(const Dictionary &p_cfg) {
	hud_config = ClockGearsDataManager::normalize_hud_config(p_cfg);
	has_explicit_config = true;
	_rebuild_widgets("config");
}

void ClockGearsHudOverlay::reload_from_data_manager() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		hud_config = ClockGearsDataManager::normalize_hud_config(dm->get_hud_config());
	} else {
		hud_config = ClockGearsDataManager::get_default_hud_config();
	}
	has_explicit_config = false;
	_rebuild_widgets("reload");
}

void ClockGearsHudOverlay::_rebuild_widgets(const String &p_reason) {
	_ensure_fallback_textures();
	_clear_widgets();

	Array raw_widgets = hud_config.get("widgets", Array());
	widgets.resize(raw_widgets.size());
	for (int i = 0; i < raw_widgets.size(); i++) {
		Dictionary w = ClockGearsDataManager::normalize_hud_widget(Dictionary(raw_widgets[i]), i + 1);
		widgets.write[i].config = w;
		widgets.write[i].id = String(w.get("id", ""));
		widgets.write[i].name = String(w.get("name", ""));
		widgets.write[i].type = String(w.get("type", "bar"));
		widgets.write[i].anchor = String(w.get("anchor", "top_left"));
		widgets.write[i].offset_x = int(w.get("offset_x", 0));
		widgets.write[i].offset_y = int(w.get("offset_y", 0));
		widgets.write[i].width = MAX(16, int(w.get("width", 160)));
		widgets.write[i].height = MAX(16, int(w.get("height", 36)));
		widgets.write[i].layer_order = int(w.get("layer_order", 0));
		widgets.write[i].window_style_id = String(w.get("window_style_id", ""));
		widgets.write[i].visibility_mode = String(w.get("visibility_mode", "always"));
		widgets.write[i].visibility_switch = String(w.get("visibility_switch", ""));
		widgets.write[i].hide_during_messages_and_menus = bool(w.get("hide_during_messages_and_menus", true));
		widgets.write[i].pixel_art = bool(w.get("pixel_art", true));

		_build_widget_entry(widgets.write[i], i);
	}

	_relayout_widgets(true);
	for (int i = 0; i < widgets.size(); i++) {
		_update_widget_data(widgets.write[i], p_reason, false);
		_update_widget_visibility(widgets.write[i]);
	}
}

void ClockGearsHudOverlay::_build_widget_entry(WidgetEntry &r_entry, int p_index) {
	Control *root = memnew(Control);
	root->set_name("HudWidget_" + r_entry.id);
	root->set_mouse_filter(MOUSE_FILTER_IGNORE);
	root->set_z_index(r_entry.layer_order);
	root->set_clip_contents(false);
	root->set_meta("_cg_widget_id", r_entry.id);
	root->set_meta("_cg_widget_name", r_entry.name);
	root->set_meta("_cg_widget_index", p_index);
	add_child(root);
	r_entry.root = root;

	Panel *bg_panel = memnew(Panel);
	bg_panel->set_name("BgWindow");
	bg_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg_panel->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	root->add_child(bg_panel);
	r_entry.bg_panel = bg_panel;

	Control *content = memnew(Control);
	content->set_name("ContentRoot");
	content->set_mouse_filter(MOUSE_FILTER_IGNORE);
	root->add_child(content);
	r_entry.content_root = content;

	TextureFilter tf = r_entry.pixel_art ? CanvasItem::TEXTURE_FILTER_NEAREST : CanvasItem::TEXTURE_FILTER_LINEAR;

	if (r_entry.type == "bar") {
		ColorRect *bg_r = memnew(ColorRect);
		bg_r->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bg_r->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		bg_r->set_color(s_hud_parse_color(r_entry.config.get("bg_color", "#1A1E2ECC"), Color(0.1f, 0.12f, 0.18f, 0.8f)));
		content->add_child(bg_r);
		r_entry.bar_bg_rect = bg_r;

		String bg_path = String(r_entry.config.get("bg_image_path", "")).strip_edges();
		TextureRect *bg_tex = memnew(TextureRect);
		bg_tex->set_mouse_filter(MOUSE_FILTER_IGNORE);
		bg_tex->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		bg_tex->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		bg_tex->set_stretch_mode(TextureRect::STRETCH_SCALE);
		bg_tex->set_texture_filter(tf);
		if (!bg_path.is_empty()) {
			Ref<Texture2D> t = ClockGearsWindowStyleManager::load_texture_safe(bg_path);
			if (t.is_valid()) {
				bg_tex->set_texture(t);
				bg_tex->set_visible(true);
				bg_r->set_visible(false);
			} else {
				bg_tex->set_visible(false);
			}
		} else {
			bg_tex->set_visible(false);
		}
		content->add_child(bg_tex);
		r_entry.bar_bg_tex = bg_tex;

		Control *fill_clip = memnew(Control);
		fill_clip->set_mouse_filter(MOUSE_FILTER_IGNORE);
		fill_clip->set_clip_contents(true);
		content->add_child(fill_clip);
		r_entry.bar_fill_clip = fill_clip;

		ColorRect *fill_r = memnew(ColorRect);
		fill_r->set_mouse_filter(MOUSE_FILTER_IGNORE);
		fill_r->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		fill_r->set_color(s_hud_parse_color(r_entry.config.get("fill_color", "#D93B3BFF"), Color(0.85f, 0.23f, 0.23f, 1.0f)));
		fill_clip->add_child(fill_r);
		r_entry.bar_fill_rect = fill_r;

		String fill_path = String(r_entry.config.get("fill_image_path", "")).strip_edges();
		TextureRect *fill_tex = memnew(TextureRect);
		fill_tex->set_mouse_filter(MOUSE_FILTER_IGNORE);
		fill_tex->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		fill_tex->set_stretch_mode(TextureRect::STRETCH_SCALE);
		fill_tex->set_texture_filter(tf);
		if (!fill_path.is_empty()) {
			Ref<Texture2D> t = ClockGearsWindowStyleManager::load_texture_safe(fill_path);
			if (t.is_valid()) {
				fill_tex->set_texture(t);
				fill_tex->set_visible(true);
				fill_r->set_visible(false);
			} else {
				fill_tex->set_visible(false);
			}
		} else {
			fill_tex->set_visible(false);
		}
		fill_clip->add_child(fill_tex);
		r_entry.bar_fill_tex = fill_tex;

		Color bcol = s_hud_parse_color(r_entry.config.get("border_color", "#E2E8F0FF"), Color(0.9f, 0.92f, 0.96f, 1.0f));
		auto mk_border = [&](const char *p_bname) {
			ColorRect *br = memnew(ColorRect);
			br->set_name(p_bname);
			br->set_mouse_filter(MOUSE_FILTER_IGNORE);
			br->set_color(bcol);
			content->add_child(br);
			return br;
		};
		r_entry.bar_border_top = mk_border("BorderTop");
		r_entry.bar_border_bottom = mk_border("BorderBottom");
		r_entry.bar_border_left = mk_border("BorderLeft");
		r_entry.bar_border_right = mk_border("BorderRight");

		Label *lbl = memnew(Label);
		lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
		lbl->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		lbl->add_theme_color_override("font_color", s_hud_parse_color(r_entry.config.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1)));
		lbl->add_theme_color_override("font_outline_color", Color(0.05f, 0.05f, 0.08f, 0.95f));
		lbl->add_theme_constant_override("outline_size", 2);
		lbl->set_visible(bool(r_entry.config.get("show_numbers", true)));
		content->add_child(lbl);
		r_entry.bar_label = lbl;
	} else if (r_entry.type == "hearts") {
		Control *hbox = memnew(Control);
		hbox->set_mouse_filter(MOUSE_FILTER_IGNORE);
		hbox->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		hbox->set_clip_contents(true);
		content->add_child(hbox);
		r_entry.hearts_container = hbox;
	} else if (r_entry.type == "currency" || r_entry.type == "item") {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_mouse_filter(MOUSE_FILTER_IGNORE);
		hb->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		hb->add_theme_constant_override("separation", 6);
		content->add_child(hb);
		r_entry.hbox = hb;

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
		lbl->add_theme_color_override("font_color", s_hud_parse_color(r_entry.config.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1)));
		lbl->add_theme_color_override("font_outline_color", Color(0.05f, 0.05f, 0.08f, 0.95f));
		lbl->add_theme_constant_override("outline_size", 2);
		hb->add_child(lbl);
		r_entry.value_label = lbl;
	} else if (r_entry.type == "text") {
		Label *lbl = memnew(Label);
		lbl->set_mouse_filter(MOUSE_FILTER_IGNORE);
		lbl->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		lbl->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		String al = String(r_entry.config.get("align", "left"));
		lbl->set_horizontal_alignment((al == "center") ? HORIZONTAL_ALIGNMENT_CENTER : ((al == "right") ? HORIZONTAL_ALIGNMENT_RIGHT : HORIZONTAL_ALIGNMENT_LEFT));
		lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		lbl->add_theme_color_override("font_color", s_hud_parse_color(r_entry.config.get("font_color", "#FFFFFFFF"), Color(1, 1, 1, 1)));
		lbl->add_theme_color_override("font_outline_color", Color(0.05f, 0.05f, 0.08f, 0.95f));
		lbl->add_theme_constant_override("outline_size", 2);
		content->add_child(lbl);
		r_entry.text_label = lbl;
	} else {
		TextureRect *img_r = memnew(TextureRect);
		img_r->set_mouse_filter(MOUSE_FILTER_IGNORE);
		img_r->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		img_r->set_texture_filter(tf);
		String sm = String(r_entry.config.get("scale_mode", "fit"));
		if (sm == "stretch") {
			img_r->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			img_r->set_stretch_mode(TextureRect::STRETCH_SCALE);
		} else if (sm == "tile") {
			img_r->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			img_r->set_stretch_mode(TextureRect::STRETCH_TILE);
		} else if (sm == "native") {
			img_r->set_expand_mode(TextureRect::EXPAND_KEEP_SIZE);
			img_r->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		} else {
			img_r->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			img_r->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		}
		float op = CLAMP(float(r_entry.config.get("opacity", 1.0)), 0.0f, 1.0f);
		img_r->set_modulate(Color(1, 1, 1, op));
		String ipath = String(r_entry.config.get("image_path", "")).strip_edges();
		if (!ipath.is_empty()) {
			img_r->set_texture(ClockGearsWindowStyleManager::load_texture_safe(ipath));
		}
		content->add_child(img_r);
		r_entry.image_rect = img_r;
	}
}

void ClockGearsHudOverlay::_apply_widget_window_style(WidgetEntry &r_entry, float p_scale) {
	if (!r_entry.bg_panel || !r_entry.content_root || !r_entry.root) {
		return;
	}
	String sid = r_entry.window_style_id.strip_edges().to_lower();
	if (sid.is_empty() || sid == "none") {
		r_entry.bg_panel->set_visible(false);
		r_entry.content_root->set_position(Vector2(0, 0));
		r_entry.content_root->set_size(r_entry.root->get_size());
		return;
	}

	r_entry.bg_panel->set_visible(true);
	Dictionary surface_overrides;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (sid != "default" && sid != "hud" && dm) {
		Dictionary specific = dm->get_window_style(sid);
		if (!specific.is_empty()) {
			surface_overrides = specific;
		}
	}
	ClockGearsWindowStyleManager::apply_window_style(r_entry.bg_panel, "hud", surface_overrides);
	Ref<StyleBox> sb = r_entry.bg_panel->get_theme_stylebox("panel");
	float pad_l = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_LEFT)) : (4.0f * p_scale);
	float pad_t = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_TOP)) : (4.0f * p_scale);
	float pad_r = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_RIGHT)) : (4.0f * p_scale);
	float pad_b = sb.is_valid() ? MAX(0.0f, sb->get_content_margin(SIDE_BOTTOM)) : (4.0f * p_scale);

	Size2 rsz = r_entry.root->get_size();
	float cw = MAX(4.0f, rsz.x - pad_l - pad_r);
	float ch = MAX(4.0f, rsz.y - pad_t - pad_b);
	r_entry.content_root->set_position(Vector2(pad_l, pad_t));
	r_entry.content_root->set_size(Vector2(cw, ch));
}

Size2 ClockGearsHudOverlay::_resolve_effective_viewport_size() const {
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

void ClockGearsHudOverlay::_relayout_widgets(bool p_refresh_styles) {
	Size2 vp_sz = _resolve_effective_viewport_size();
	if (!p_refresh_styles && vp_sz == last_layout_size) {
		return;
	}
	last_layout_size = vp_sz;

	float sx = vp_sz.x / LOGICAL_WIDTH;
	float sy = vp_sz.y / LOGICAL_HEIGHT;
	float s = MAX(0.25f, MIN(sx, sy));
	current_ui_scale = s;

	Size2 stage_sz(LOGICAL_WIDTH * s, LOGICAL_HEIGHT * s);
	Vector2 stage_origin((vp_sz.x - stage_sz.x) * 0.5f, (vp_sz.y - stage_sz.y) * 0.5f);
	current_stage_rect = Rect2(stage_origin, stage_sz);

	for (int i = 0; i < widgets.size(); i++) {
		WidgetEntry &w = widgets.write[i];
		if (!w.root) {
			continue;
		}
		Vector2 af = s_anchor_factors(w.anchor);
		float base_x = af.x * (LOGICAL_WIDTH - float(w.width)) + float(w.offset_x);
		float base_y = af.y * (LOGICAL_HEIGHT - float(w.height)) + float(w.offset_y);
		Vector2 screen_pos(Math::round(stage_origin.x + base_x * s), Math::round(stage_origin.y + base_y * s));
		Vector2 screen_sz(Math::round(float(w.width) * s), Math::round(float(w.height) * s));
		screen_sz.x = MAX(4.0f, screen_sz.x);
		screen_sz.y = MAX(4.0f, screen_sz.y);

		w.root->set_position(screen_pos);
		w.root->set_size(screen_sz);
		w.root->set_z_index(w.layer_order);
		_apply_widget_window_style(w, s);

		Size2 csz = w.content_root ? w.content_root->get_size() : screen_sz;
		if (w.type == "bar") {
			float bpx = MAX(1.0f, Math::round(1.5f * s));
			if (w.bar_border_top) {
				w.bar_border_top->set_position(Vector2(0, 0));
				w.bar_border_top->set_size(Vector2(csz.x, bpx));
			}
			if (w.bar_border_bottom) {
				w.bar_border_bottom->set_position(Vector2(0, csz.y - bpx));
				w.bar_border_bottom->set_size(Vector2(csz.x, bpx));
			}
			if (w.bar_border_left) {
				w.bar_border_left->set_position(Vector2(0, 0));
				w.bar_border_left->set_size(Vector2(bpx, csz.y));
			}
			if (w.bar_border_right) {
				w.bar_border_right->set_position(Vector2(csz.x - bpx, 0));
				w.bar_border_right->set_size(Vector2(bpx, csz.y));
			}
			if (w.bar_fill_tex) {
				w.bar_fill_tex->set_position(Vector2(0, 0));
				w.bar_fill_tex->set_size(csz);
			}
			if (w.bar_label) {
				int fsz = MAX(8, int(Math::round(float(w.config.get("font_size", 13)) * s)));
				w.bar_label->add_theme_font_size_override("font_size", fsz);
			}
		} else if (w.type == "currency" || w.type == "item") {
			if (w.hbox) {
				w.hbox->add_theme_constant_override("separation", MAX(2, int(Math::round(6.0f * s))));
			}
			if (w.icon_rect) {
				float isz = MAX(12.0f, MIN(csz.y - 2.0f, 24.0f * s));
				w.icon_rect->set_custom_minimum_size(Vector2(isz, isz));
			}
			if (w.value_label) {
				int fsz = MAX(8, int(Math::round(float(w.config.get("font_size", 15)) * s)));
				w.value_label->add_theme_font_size_override("font_size", fsz);
			}
		} else if (w.type == "text") {
			if (w.text_label) {
				int fsz = MAX(8, int(Math::round(float(w.config.get("font_size", 16)) * s)));
				w.text_label->add_theme_font_size_override("font_size", fsz);
			}
		}
		// Reaplicar geometría interna proporcional de barra / corazones según el nuevo tamaño
		_update_widget_data(w, "resize", false);
	}
}

int ClockGearsHudOverlay::_resolve_leader_actor_id(ClockGearsDataManager *p_dm) const {
	if (leader_actor_id_override > 0) {
		return leader_actor_id_override;
	}
	if (!p_dm) {
		return 1;
	}
	Array party = p_dm->get_party_actor_ids();
	if (!party.is_empty()) {
		int fid = int(party[0]);
		if (fid > 0) {
			return fid;
		}
	}
	Array actors = p_dm->get_category_array(CAT_ACTORS);
	if (!actors.is_empty() && actors[0].get_type() == Variant::DICTIONARY) {
		int aid = int(Dictionary(actors[0]).get("id", 1));
		if (aid > 0) {
			return aid;
		}
	}
	return 1;
}

void ClockGearsHudOverlay::_update_widget_data(WidgetEntry &r_entry, const String &p_reason, bool p_count_update) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary snap;
	snap["id"] = r_entry.id;
	snap["name"] = r_entry.name;
	snap["type"] = r_entry.type;
	snap["reason"] = p_reason;

	float s = current_ui_scale;
	Size2 csz = r_entry.content_root ? r_entry.content_root->get_size() : Vector2(float(r_entry.width) * s, float(r_entry.height) * s);

	if (r_entry.type == "bar") {
		String vs = String(r_entry.config.get("value_source", "hp"));
		int64_t val = 0;
		int64_t max_val = 100;
		if (vs == "variable") {
			String vname = String(r_entry.config.get("variable_name", ""));
			val = dm ? int64_t(dm->get_variable(vname)) : 45;
			max_val = MAX(1, int64_t(r_entry.config.get("variable_max", 100)));
		} else {
			int aid = _resolve_leader_actor_id(dm);
			Dictionary st = dm ? dm->get_actor_runtime_state(aid) : Dictionary();
			if (st.is_empty()) {
				val = (vs == "mp") ? 20 : ((vs == "exp") ? 35 : 80);
				max_val = (vs == "mp") ? 30 : 100;
			} else if (vs == "mp") {
				val = MAX(0, int64_t(st.get("mp", 0)));
				max_val = MAX(1, int64_t(st.get("max_mp", 20)));
			} else if (vs == "exp") {
				int lv = MAX(1, int(st.get("level", 1)));
				int64_t cur_exp = MAX<int64_t>(0, int64_t(st.get("exp", 0)));
				int exp_base = 30, exp_extra = 20, exp_a = 30, exp_b = 30;
				Dictionary act = dm ? dm->get_record_by_id(CAT_ACTORS, aid) : Dictionary();
				int class_id = int(act.get("class_id", 1));
				Dictionary cls = dm ? dm->get_record_by_id(CAT_CLASSES, class_id) : Dictionary();
				if (!cls.is_empty()) {
					exp_base = int(cls.get("exp_base", 30));
					exp_extra = int(cls.get("exp_extra", 20));
					exp_a = int(cls.get("exp_acc_a", 30));
					exp_b = int(cls.get("exp_acc_b", 30));
				}
				ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
				int64_t lv_start = (fm && lv > 1) ? fm->calculate_exp_for_level(lv, exp_base, exp_extra, exp_a, exp_b) : 0;
				int64_t lv_next = fm ? fm->calculate_exp_for_level(lv + 1, exp_base, exp_extra, exp_a, exp_b) : (lv_start + 100);
				val = MAX<int64_t>(0, cur_exp - lv_start);
				max_val = MAX<int64_t>(1, lv_next - lv_start);
			} else {
				val = MAX(0, int64_t(st.get("hp", 0)));
				max_val = MAX(1, int64_t(st.get("max_hp", 100)));
			}
		}
		float ratio = CLAMP(float(double(val) / double(max_val)), 0.0f, 1.0f);
		float bpx = MAX(1.0f, Math::round(1.5f * s));
		float inner_w = MAX(0.0f, csz.x - 2.0f * bpx);
		float inner_h = MAX(2.0f, csz.y - 2.0f * bpx);
		if (r_entry.bar_fill_clip) {
			r_entry.bar_fill_clip->set_position(Vector2(bpx, bpx));
			r_entry.bar_fill_clip->set_size(Vector2(Math::round(inner_w * ratio), inner_h));
		}
		if (r_entry.bar_fill_tex && r_entry.bar_fill_tex->is_visible()) {
			r_entry.bar_fill_tex->set_size(Vector2(inner_w, inner_h));
		}
		String prefix = String(r_entry.config.get("label_text", "")).strip_edges();
		String num_txt = String::num_int64(val) + " / " + String::num_int64(max_val);
		String full_txt = prefix.is_empty() ? num_txt : (prefix + " " + num_txt);
		if (r_entry.bar_label) {
			r_entry.bar_label->set_text(full_txt);
			r_entry.bar_label->set_visible(bool(r_entry.config.get("show_numbers", true)));
		}
		snap["value_source"] = vs;
		snap["value"] = val;
		snap["max"] = max_val;
		snap["ratio"] = ratio;
		snap["text"] = full_txt;
	} else if (r_entry.type == "hearts") {
		int hp_per_heart = MAX(1, int(r_entry.config.get("hp_per_heart", 20)));
		int per_row = MAX(1, int(r_entry.config.get("max_hearts_per_row", 10)));
		int max_rows = MAX(1, int(r_entry.config.get("max_rows", 2)));
		int hsz_base = CLAMP(int(r_entry.config.get("heart_size", 20)), 8, 128);
		int sp_base = CLAMP(int(r_entry.config.get("spacing", 4)), 0, 64);

		int aid = _resolve_leader_actor_id(dm);
		Dictionary st = dm ? dm->get_actor_runtime_state(aid) : Dictionary();
		int cur_hp = st.is_empty() ? 70 : MAX(0, int(st.get("hp", 0)));
		int max_hp = st.is_empty() ? 100 : MAX(1, int(st.get("max_hp", 100)));

		int needed = int(Math::ceil(double(max_hp) / double(hp_per_heart)));
		int max_visible = per_row * max_rows;
		int total_hearts = CLAMP(needed, 1, max_visible);

		Ref<Texture2D> full_tex = ClockGearsWindowStyleManager::load_texture_safe(String(r_entry.config.get("full_image_path", "")));
		if (full_tex.is_null()) {
			full_tex = fallback_heart_full;
		}
		Ref<Texture2D> half_tex = ClockGearsWindowStyleManager::load_texture_safe(String(r_entry.config.get("half_image_path", "")));
		if (half_tex.is_null()) {
			half_tex = fallback_heart_half;
		}
		Ref<Texture2D> empty_tex = ClockGearsWindowStyleManager::load_texture_safe(String(r_entry.config.get("empty_image_path", "")));
		if (empty_tex.is_null()) {
			empty_tex = fallback_heart_empty;
		}

		TextureFilter tf = r_entry.pixel_art ? CanvasItem::TEXTURE_FILTER_NEAREST : CanvasItem::TEXTURE_FILTER_LINEAR;
		while (r_entry.heart_nodes.size() < total_hearts && r_entry.hearts_container) {
			TextureRect *hr = memnew(TextureRect);
			hr->set_mouse_filter(MOUSE_FILTER_IGNORE);
			hr->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			hr->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			r_entry.hearts_container->add_child(hr);
			r_entry.heart_nodes.push_back(hr);
		}
		while (r_entry.heart_nodes.size() > total_hearts) {
			int last = r_entry.heart_nodes.size() - 1;
			TextureRect *hr = r_entry.heart_nodes[last];
			r_entry.heart_nodes.remove_at(last);
			if (hr) {
				if (hr->get_parent() == r_entry.hearts_container) {
					r_entry.hearts_container->remove_child(hr);
				}
				hr->queue_free();
			}
		}

		float hsz = MAX(6.0f, Math::round(float(hsz_base) * s));
		float sp = Math::round(float(sp_base) * s);
		int full_count = 0;
		int half_count = 0;
		int empty_count = 0;
		for (int hi = 0; hi < total_hearts; hi++) {
			int rem = cur_hp - (hi * hp_per_heart);
			Ref<Texture2D> tex;
			if (rem >= hp_per_heart) {
				tex = full_tex;
				full_count++;
			} else if (rem > 0) {
				tex = half_tex;
				half_count++;
			} else {
				tex = empty_tex;
				empty_count++;
			}
			TextureRect *hr = r_entry.heart_nodes[hi];
			if (hr) {
				int row = hi / per_row;
				int col = hi % per_row;
				hr->set_texture(tex);
				hr->set_texture_filter(tf);
				hr->set_position(Vector2(float(col) * (hsz + sp), float(row) * (hsz + sp)));
				hr->set_size(Vector2(hsz, hsz));
				hr->set_visible(row < max_rows);
			}
		}
		snap["hp"] = cur_hp;
		snap["max_hp"] = max_hp;
		snap["hp_per_heart"] = hp_per_heart;
		snap["total_hearts"] = total_hearts;
		snap["full_hearts"] = full_count;
		snap["half_hearts"] = half_count;
		snap["empty_hearts"] = empty_count;
	} else if (r_entry.type == "currency") {
		int gold = dm ? dm->get_gold() : 250;
		String unit = dm ? dm->get_currency_unit() : "G";
		bool show_ic = bool(r_entry.config.get("show_icon", true));
		bool show_unit = bool(r_entry.config.get("show_currency_name", true));
		String txt = show_unit ? (String::num_int64(gold) + " " + unit) : String::num_int64(gold);

		if (r_entry.icon_rect) {
			Ref<Texture2D> tex = ClockGearsWindowStyleManager::load_texture_safe(String(r_entry.config.get("icon_path", "")));
			if (tex.is_null()) {
				tex = fallback_coin_tex;
			}
			r_entry.icon_rect->set_texture(tex);
			r_entry.icon_rect->set_visible(show_ic);
		}
		if (r_entry.value_label) {
			r_entry.value_label->set_text(txt);
		}
		snap["gold"] = gold;
		snap["currency_unit"] = unit;
		snap["text"] = txt;
	} else if (r_entry.type == "item") {
		String ikey = String(r_entry.config.get("item_key", ""));
		Dictionary info;
		int qty = s_resolve_item_live_qty(dm, ikey, info);
		Dictionary rec = info.get("record", Dictionary());
		String iname = bool(info.get("found", false)) ? String(rec.get("name", ikey)).strip_edges() : ikey;
		if (iname.is_empty()) {
			iname = "Objeto";
		}
		String custom_ic = String(r_entry.config.get("icon_path", "")).strip_edges();
		String db_ic = bool(info.get("found", false)) ? String(rec.get("icon_path", rec.get("graphic_path", ""))).strip_edges() : String();
		Ref<Texture2D> tex = ClockGearsWindowStyleManager::load_texture_safe(custom_ic.is_empty() ? db_ic : custom_ic);
		if (tex.is_null()) {
			tex = fallback_item_tex;
		}
		if (r_entry.icon_rect) {
			r_entry.icon_rect->set_texture(tex);
		}
		bool show_name = bool(r_entry.config.get("show_name", false));
		String txt = show_name ? (iname + " x" + String::num_int64(qty)) : ("x" + String::num_int64(qty));
		if (r_entry.value_label) {
			r_entry.value_label->set_text(txt);
		}
		snap["item_key"] = ikey;
		snap["item_name"] = iname;
		snap["qty"] = qty;
		snap["hide_when_zero"] = bool(r_entry.config.get("hide_when_zero", false));
		snap["text"] = txt;
	} else if (r_entry.type == "text") {
		String tmode = String(r_entry.config.get("text_mode", "fixed"));
		String raw_txt = String(r_entry.config.get("text", ""));
		String out_txt = raw_txt;
		if (tmode == "variable") {
			String vname = String(r_entry.config.get("variable_name", ""));
			int vval = dm ? dm->get_variable(vname) : 0;
			String vstr = String::num_int64(vval);
			if (raw_txt.is_empty()) {
				out_txt = vstr;
			} else if (raw_txt.contains("{val}")) {
				out_txt = raw_txt.replace("{val}", vstr);
			} else {
				out_txt = raw_txt + vstr;
			}
			snap["variable_name"] = vname;
			snap["variable_value"] = vval;
		}
		if (r_entry.text_label) {
			r_entry.text_label->set_text(out_txt);
		}
		snap["text_mode"] = tmode;
		snap["text"] = out_txt;
	} else {
		snap["image_path"] = String(r_entry.config.get("image_path", ""));
	}

	if (p_count_update) {
		r_entry.update_count++;
		total_update_count++;
		if (log_updates) {
			print_line(vformat("[ClockGears HUD] Widget '%s' actualizado (evento: %s, total=%d)", r_entry.name, p_reason, r_entry.update_count));
		}
	}
	snap["update_count"] = r_entry.update_count;
	r_entry.last_snapshot = snap;
}

void ClockGearsHudOverlay::_update_widget_visibility(WidgetEntry &r_entry) {
	if (!r_entry.root) {
		return;
	}
	bool vis = true;
	if (!preview_mode) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm) {
			if (!dm->is_hud_widget_runtime_visible(r_entry.id, r_entry.name)) {
				vis = false;
			}
			if (vis && r_entry.visibility_mode == "switch") {
				vis = !r_entry.visibility_switch.is_empty() && dm->get_switch(r_entry.visibility_switch);
			}
		}
		if (vis && r_entry.hide_during_messages_and_menus && context_blocked) {
			vis = false;
		}
		if (vis && r_entry.type == "item" && bool(r_entry.config.get("hide_when_zero", false))) {
			int qty = int(r_entry.last_snapshot.get("qty", 0));
			if (qty <= 0) {
				vis = false;
			}
		}
	}
	r_entry.root->set_visible(vis);
}

bool ClockGearsHudOverlay::_widget_matches_topic(const WidgetEntry &p_entry, const String &p_topic, const String &p_key) const {
	if (p_topic == "all" || p_topic == "hud") {
		return true;
	}
	if (p_topic == "window_style") {
		return !p_entry.window_style_id.is_empty();
	}
	if (p_topic == "hud_visibility") {
		if (p_key.is_empty()) {
			return true;
		}
		String lk = p_key.to_lower();
		return lk == p_entry.id.to_lower() || lk == p_entry.name.strip_edges().to_lower();
	}
	if (p_topic == "switch") {
		if (p_entry.visibility_mode != "switch") {
			return false;
		}
		return p_key.is_empty() || p_entry.visibility_switch == p_key || p_entry.visibility_switch.to_lower() == p_key.to_lower();
	}
	if (p_topic == "variable") {
		if (p_entry.type == "bar" && String(p_entry.config.get("value_source", "")) == "variable") {
			return String(p_entry.config.get("variable_name", "")) == p_key;
		}
		if (p_entry.type == "text" && String(p_entry.config.get("text_mode", "")) == "variable") {
			return String(p_entry.config.get("variable_name", "")) == p_key;
		}
		return false;
	}
	if (p_topic == "gold" || p_topic == "currency_unit") {
		return p_entry.type == "currency";
	}
	if (p_topic == "item") {
		if (p_entry.type != "item") {
			return false;
		}
		if (p_key.is_empty()) {
			return true;
		}
		return s_item_keys_equivalent(ClockGearsDataManager::get_singleton(), String(p_entry.config.get("item_key", "")), p_key);
	}
	if (p_topic == "actor_hp") {
		return p_entry.type == "hearts" || (p_entry.type == "bar" && String(p_entry.config.get("value_source", "hp")) == "hp");
	}
	if (p_topic == "actor_mp") {
		return p_entry.type == "bar" && String(p_entry.config.get("value_source", "")) == "mp";
	}
	if (p_topic == "actor_exp") {
		return p_entry.type == "bar" && String(p_entry.config.get("value_source", "")) == "exp";
	}
	if (p_topic == "actor_stats") {
		return p_entry.type == "bar" || p_entry.type == "hearts";
	}
	return false;
}

void ClockGearsHudOverlay::_on_runtime_state_changed(const String &p_topic, const String &p_key) {
	if (p_topic == "hud") {
		if (!has_explicit_config) {
			reload_from_data_manager();
		}
		return;
	}
	if (p_topic == "window_style") {
		refresh_window_styles();
	}
	for (int i = 0; i < widgets.size(); i++) {
		WidgetEntry &w = widgets.write[i];
		if (!_widget_matches_topic(w, p_topic, p_key)) {
			continue;
		}
		if (p_topic == "hud_visibility" || p_topic == "switch") {
			_update_widget_visibility(w);
			w.update_count++;
			total_update_count++;
			w.last_snapshot["update_count"] = w.update_count;
		} else {
			_update_widget_data(w, p_topic, true);
			_update_widget_visibility(w);
		}
	}
}

void ClockGearsHudOverlay::_on_data_changed(const String &p_category, int p_id) {
	if (p_category == "hud") {
		if (!has_explicit_config) {
			reload_from_data_manager();
		}
		return;
	}
	if (p_category == "window_styles") {
		refresh_window_styles();
	} else if (p_category == "system") {
		_on_runtime_state_changed("currency_unit", String());
	} else if (p_category == "actors" || p_category == "classes") {
		_on_runtime_state_changed("actor_stats", String::num_int64(p_id));
	}
}

void ClockGearsHudOverlay::_on_viewport_resized() {
	_relayout_widgets(true);
}

void ClockGearsHudOverlay::refresh_all_widgets(const String &p_reason) {
	for (int i = 0; i < widgets.size(); i++) {
		_update_widget_data(widgets.write[i], p_reason, true);
		_update_widget_visibility(widgets.write[i]);
	}
}

void ClockGearsHudOverlay::refresh_window_styles() {
	for (int i = 0; i < widgets.size(); i++) {
		_apply_widget_window_style(widgets.write[i], current_ui_scale);
	}
}

void ClockGearsHudOverlay::set_context_blocked(bool p_blocked) {
	if (context_blocked == p_blocked) {
		return;
	}
	context_blocked = p_blocked;
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i].hide_during_messages_and_menus) {
			_update_widget_visibility(widgets.write[i]);
		}
	}
}

void ClockGearsHudOverlay::set_preview_mode(bool p_preview) {
	if (preview_mode == p_preview) {
		return;
	}
	preview_mode = p_preview;
	for (int i = 0; i < widgets.size(); i++) {
		_update_widget_data(widgets.write[i], "preview_mode", false);
		_update_widget_visibility(widgets.write[i]);
	}
}

void ClockGearsHudOverlay::set_leader_actor_id(int p_actor_id) {
	leader_actor_id_override = MAX(0, p_actor_id);
	refresh_all_widgets("leader_changed");
}

void ClockGearsHudOverlay::set_viewport_size_override(const Size2 &p_size) {
	viewport_size_override = p_size;
	_relayout_widgets(true);
}

Control *ClockGearsHudOverlay::get_widget_node(const String &p_id_or_name) const {
	String target = p_id_or_name.strip_edges().to_lower();
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i].id.to_lower() == target || widgets[i].name.strip_edges().to_lower() == target) {
			return widgets[i].root;
		}
	}
	return nullptr;
}

Dictionary ClockGearsHudOverlay::get_widget_state(const String &p_id_or_name) const {
	String target = p_id_or_name.strip_edges().to_lower();
	for (int i = 0; i < widgets.size(); i++) {
		const WidgetEntry &w = widgets[i];
		if (w.id.to_lower() == target || w.name.strip_edges().to_lower() == target) {
			Dictionary out = w.last_snapshot.duplicate(true);
			out["found"] = true;
			out["id"] = w.id;
			out["name"] = w.name;
			out["type"] = w.type;
			out["anchor"] = w.anchor;
			out["layer_order"] = w.layer_order;
			out["window_style_id"] = w.window_style_id;
			out["visible"] = w.root ? w.root->is_visible() : false;
			out["update_count"] = w.update_count;
			if (w.root) {
				out["screen_rect"] = Rect2(w.root->get_position(), w.root->get_size());
			}
			Vector2 af = s_anchor_factors(w.anchor);
			out["base_rect"] = Rect2(
					af.x * (LOGICAL_WIDTH - float(w.width)) + float(w.offset_x),
					af.y * (LOGICAL_HEIGHT - float(w.height)) + float(w.offset_y),
					float(w.width),
					float(w.height));
			return out;
		}
	}
	Dictionary nf;
	nf["found"] = false;
	return nf;
}

Array ClockGearsHudOverlay::get_all_widget_states() const {
	Array out;
	for (int i = 0; i < widgets.size(); i++) {
		out.push_back(get_widget_state(widgets[i].id));
	}
	return out;
}

int ClockGearsHudOverlay::get_widget_update_count(const String &p_id_or_name) const {
	String target = p_id_or_name.strip_edges().to_lower();
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i].id.to_lower() == target || widgets[i].name.strip_edges().to_lower() == target) {
			return widgets[i].update_count;
		}
	}
	return 0;
}

void ClockGearsHudOverlay::reset_update_counters() {
	total_update_count = 0;
	for (int i = 0; i < widgets.size(); i++) {
		widgets.write[i].update_count = 0;
		widgets.write[i].last_snapshot["update_count"] = 0;
	}
}

Dictionary ClockGearsHudOverlay::get_layout_bounds_info() const {
	Size2 vp_sz = _resolve_effective_viewport_size();
	Rect2 vp_rect(Vector2(0, 0), vp_sz);

	Array clipped;
	Array widget_rects;
	Vector<Rect2> vis_rects;
	Vector<String> vis_ids;

	for (int i = 0; i < widgets.size(); i++) {
		const WidgetEntry &w = widgets[i];
		if (!w.root) {
			continue;
		}
		Rect2 r(w.root->get_position(), w.root->get_size());
		bool vis = w.root->is_visible();
		Dictionary row;
		row["id"] = w.id;
		row["name"] = w.name;
		row["anchor"] = w.anchor;
		row["visible"] = vis;
		row["screen_rect"] = r;
		widget_rects.push_back(row);

		if (vis) {
			if (r.position.x < -0.5f || r.position.y < -0.5f ||
					(r.position.x + r.size.x) > (vp_sz.x + 0.5f) ||
					(r.position.y + r.size.y) > (vp_sz.y + 0.5f)) {
				clipped.push_back(w.id);
			}
			vis_rects.push_back(r);
			vis_ids.push_back(w.id);
		}
	}

	Array overlaps;
	for (int i = 0; i < vis_rects.size(); i++) {
		for (int j = i + 1; j < vis_rects.size(); j++) {
			Rect2 a = vis_rects[i];
			Rect2 b = vis_rects[j];
			Rect2 inter = a.intersection(b);
			if (inter.size.x > 1.0f && inter.size.y > 1.0f) {
				Dictionary pair;
				pair["a"] = vis_ids[i];
				pair["b"] = vis_ids[j];
				pair["intersection"] = inter;
				overlaps.push_back(pair);
			}
		}
	}

	Dictionary out;
	out["viewport_size"] = vp_sz;
	out["viewport_rect"] = vp_rect;
	out["stage_rect"] = current_stage_rect;
	out["scale"] = current_ui_scale;
	out["clipped_widgets"] = clipped;
	out["overlapping_pairs"] = overlaps;
	out["widget_rects"] = widget_rects;
	return out;
}

void ClockGearsHudOverlay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("apply_hud_config", "cfg"), &ClockGearsHudOverlay::apply_hud_config);
	ClassDB::bind_method(D_METHOD("reload_from_data_manager"), &ClockGearsHudOverlay::reload_from_data_manager);
	ClassDB::bind_method(D_METHOD("refresh_all_widgets", "reason"), &ClockGearsHudOverlay::refresh_all_widgets, DEFVAL("manual"));
	ClassDB::bind_method(D_METHOD("refresh_window_styles"), &ClockGearsHudOverlay::refresh_window_styles);
	ClassDB::bind_method(D_METHOD("set_context_blocked", "blocked"), &ClockGearsHudOverlay::set_context_blocked);
	ClassDB::bind_method(D_METHOD("is_context_blocked"), &ClockGearsHudOverlay::is_context_blocked);
	ClassDB::bind_method(D_METHOD("set_preview_mode", "preview"), &ClockGearsHudOverlay::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsHudOverlay::is_preview_mode);
	ClassDB::bind_method(D_METHOD("set_leader_actor_id", "actor_id"), &ClockGearsHudOverlay::set_leader_actor_id);
	ClassDB::bind_method(D_METHOD("get_leader_actor_id"), &ClockGearsHudOverlay::get_leader_actor_id);
	ClassDB::bind_method(D_METHOD("set_viewport_size_override", "size"), &ClockGearsHudOverlay::set_viewport_size_override);
	ClassDB::bind_method(D_METHOD("get_viewport_size_override"), &ClockGearsHudOverlay::get_viewport_size_override);
	ClassDB::bind_method(D_METHOD("set_log_updates", "enable"), &ClockGearsHudOverlay::set_log_updates);
	ClassDB::bind_method(D_METHOD("is_logging_updates"), &ClockGearsHudOverlay::is_logging_updates);
	ClassDB::bind_method(D_METHOD("get_widget_count"), &ClockGearsHudOverlay::get_widget_count);
	ClassDB::bind_method(D_METHOD("get_widget_node", "id_or_name"), &ClockGearsHudOverlay::get_widget_node);
	ClassDB::bind_method(D_METHOD("get_widget_state", "id_or_name"), &ClockGearsHudOverlay::get_widget_state);
	ClassDB::bind_method(D_METHOD("get_all_widget_states"), &ClockGearsHudOverlay::get_all_widget_states);
	ClassDB::bind_method(D_METHOD("get_widget_update_count", "id_or_name"), &ClockGearsHudOverlay::get_widget_update_count);
	ClassDB::bind_method(D_METHOD("get_total_update_count"), &ClockGearsHudOverlay::get_total_update_count);
	ClassDB::bind_method(D_METHOD("reset_update_counters"), &ClockGearsHudOverlay::reset_update_counters);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info"), &ClockGearsHudOverlay::get_layout_bounds_info);
}
