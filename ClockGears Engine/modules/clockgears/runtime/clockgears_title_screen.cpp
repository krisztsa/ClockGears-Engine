#include "clockgears_title_screen.h"
#include "../core/clockgears_data_manager.h"
#include "../editor/clockgears_theme.h"
#include "clockgears_audio_manager.h"

#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "scene/gui/center_container.h"
#include "scene/gui/margin_container.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

static bool s_is_light_theme_active() {
	return ClockGearsTheme::get_singleton() && ClockGearsTheme::get_singleton()->is_light_mode();
}

static Ref<StyleBoxFlat> s_make_title_panel_style(const Color &p_bg, const Color &p_border, int p_radius = 10, int p_margin = 16) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(true);
	sb->set_bg_color(p_bg);
	sb->set_border_width_all(1);
	sb->set_border_color(p_border);
	sb->set_corner_radius_all(p_radius);
	sb->set_content_margin_all(p_margin);
	return sb;
}

static Ref<StyleBoxFlat> s_make_title_button_style(const Color &p_bg, const Color &p_border, int p_radius = 8, int p_border_w = 1) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(true);
	sb->set_bg_color(p_bg);
	sb->set_border_width_all(p_border_w);
	sb->set_border_color(p_border);
	sb->set_corner_radius_all(p_radius);
	sb->set_content_margin_individual(14, 7, 14, 7);
	return sb;
}

static void s_style_menu_button(Button *p_btn, bool p_dimmed_disabled = false) {
	if (!p_btn) {
		return;
	}
	const bool light = s_is_light_theme_active();
	const Color accent = ClockGearsTheme::color_accent_amber();
	const Color txt = light ? Color::html("#1e2026") : ClockGearsTheme::color_text_primary();
	const Color txt_dis = light ? Color(0.35f, 0.38f, 0.44f, p_dimmed_disabled ? 0.45f : 0.60f) : Color(0.65f, 0.68f, 0.75f, p_dimmed_disabled ? 0.42f : 0.55f);

	const Color bg_norm = light ? Color(0.89f, 0.90f, 0.93f, 0.96f) : Color(0.15f, 0.17f, 0.22f, 0.94f);
	const Color bg_hov = light ? Color(0.83f, 0.86f, 0.91f, 0.98f) : Color(0.21f, 0.24f, 0.31f, 0.98f);
	const Color bg_prs = light ? Color(accent.r, accent.g, accent.b, 0.22f) : Color(0.24f, 0.20f, 0.14f, 0.98f);
	const Color bg_dis = light ? Color(0.91f, 0.92f, 0.94f, p_dimmed_disabled ? 0.45f : 0.70f) : Color(0.10f, 0.11f, 0.15f, p_dimmed_disabled ? 0.45f : 0.70f);
	const Color border_norm = light ? Color(0.0f, 0.0f, 0.0f, 0.18f) : Color(1.0f, 1.0f, 1.0f, 0.15f);

	p_btn->add_theme_style_override("normal", s_make_title_button_style(bg_norm, border_norm, 8, 1));
	p_btn->add_theme_style_override("hover", s_make_title_button_style(bg_hov, Color(accent.r, accent.g, accent.b, 0.75f), 8, 1));
	p_btn->add_theme_style_override("pressed", s_make_title_button_style(bg_prs, accent, 8, 1));
	p_btn->add_theme_style_override("hover_pressed", s_make_title_button_style(bg_prs, accent, 8, 1));
	p_btn->add_theme_style_override("focus", s_make_title_button_style(bg_hov, accent, 8, 2));
	p_btn->add_theme_style_override("disabled", s_make_title_button_style(bg_dis, Color(border_norm.r, border_norm.g, border_norm.b, 0.07f), 8, 1));

	p_btn->add_theme_color_override("font_color", txt);
	p_btn->add_theme_color_override("font_hover_color", light ? txt : Color(1, 1, 1, 1));
	p_btn->add_theme_color_override("font_pressed_color", accent);
	p_btn->add_theme_color_override("font_focus_color", light ? txt : Color(1, 1, 1, 1));
	p_btn->add_theme_color_override("font_disabled_color", txt_dis);
	p_btn->set_custom_minimum_size(Size2(220, 36));
}

bool ClockGearsTitleScreen::ensure_title_screen_scene(const String &p_scene_path) {
	String path = p_scene_path.strip_edges();
	if (path.is_empty()) {
		path = "res://title_screen.tscn";
	}
	if (FileAccess::exists(path)) {
		return true;
	}
	String base_dir = path.get_base_dir();
	if (!base_dir.is_empty()) {
		DirAccess::make_dir_recursive_absolute(base_dir);
	}
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	if (f.is_null()) {
		return false;
	}
	f->store_string(
			"[gd_scene format=3]\n\n"
			"[node name=\"TitleScreen\" type=\"ClockGearsTitleScreen\"]\n"
			"layout_mode = 3\n"
			"anchors_preset = 15\n"
			"anchor_right = 1.0\n"
			"anchor_bottom = 1.0\n"
			"grow_horizontal = 2\n"
			"grow_vertical = 2\n");
	f->flush();
	f->close();
	return true;
}

Ref<Texture2D> ClockGearsTitleScreen::_load_texture(const String &p_path) const {
	if (p_path.strip_edges().is_empty()) {
		return Ref<Texture2D>();
	}
	if (ResourceLoader::exists(p_path)) {
		Ref<Texture2D> tex = ResourceLoader::load(p_path);
		if (tex.is_valid()) {
			return tex;
		}
	}
	if (FileAccess::exists(p_path)) {
		Ref<Image> img = memnew(Image);
		if (ImageLoader::load_image(p_path, img) == OK && !img->is_empty()) {
			return ImageTexture::create_from_image(img);
		}
	}
	return Ref<Texture2D>();
}

void ClockGearsTitleScreen::_apply_theme_styling() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_control(this);
	}
	const bool light = s_is_light_theme_active();
	const Color card_bg = light ? Color(0.97f, 0.98f, 0.99f, 0.94f) : Color(0.08f, 0.09f, 0.13f, 0.92f);
	const Color modal_bg = light ? Color(0.98f, 0.98f, 1.0f, 0.98f) : Color(0.07f, 0.08f, 0.12f, 0.97f);
	const Color border_main = light ? Color(0.25f, 0.30f, 0.42f, 0.35f) : Color(0.35f, 0.42f, 0.58f, 0.78f);
	const Color border_modal = Color(ClockGearsTheme::color_accent_amber().r, ClockGearsTheme::color_accent_amber().g, ClockGearsTheme::color_accent_amber().b, 0.55f);

	if (main_menu_panel) {
		main_menu_panel->add_theme_style_override("panel", s_make_title_panel_style(card_bg, border_main, 10, 14));
	}
	if (continue_panel) {
		continue_panel->add_theme_style_override("panel", s_make_title_panel_style(modal_bg, border_modal, 12, 16));
	}
	if (options_panel) {
		options_panel->add_theme_style_override("panel", s_make_title_panel_style(modal_bg, border_modal, 12, 16));
	}
	if (btn_continue_back) {
		s_style_menu_button(btn_continue_back);
		btn_continue_back->set_custom_minimum_size(Size2(180, 34));
	}
	if (btn_options_reset) {
		s_style_menu_button(btn_options_reset);
		btn_options_reset->set_custom_minimum_size(Size2(150, 34));
	}
	if (btn_options_back) {
		s_style_menu_button(btn_options_back);
		btn_options_back->set_custom_minimum_size(Size2(150, 34));
	}
}

void ClockGearsTitleScreen::_build_base_ui() {
	if (bg_color_rect) {
		return;
	}
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_clip_contents(true);
	set_focus_mode(FOCUS_ALL);

	bg_color_rect = memnew(ColorRect);
	bg_color_rect->set_name("BackgroundColor");
	bg_color_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	bg_color_rect->set_color(Color::html("#12151e"));
	bg_color_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(bg_color_rect);

	bg_texture_rect = memnew(TextureRect);
	bg_texture_rect->set_name("BackgroundTexture");
	bg_texture_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	bg_texture_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	bg_texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	bg_texture_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg_texture_rect->hide();
	add_child(bg_texture_rect);

	MarginContainer *layout_margin = memnew(MarginContainer);
	layout_margin->set_name("TitleMargins");
	layout_margin->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	layout_margin->add_theme_constant_override("margin_left", 24);
	layout_margin->add_theme_constant_override("margin_top", 20);
	layout_margin->add_theme_constant_override("margin_right", 24);
	layout_margin->add_theme_constant_override("margin_bottom", 20);
	add_child(layout_margin);

	VBoxContainer *root_vb = memnew(VBoxContainer);
	root_vb->set_name("TitleLayout");
	root_vb->set_h_size_flags(SIZE_EXPAND_FILL);
	root_vb->set_v_size_flags(SIZE_EXPAND_FILL);
	root_vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	root_vb->add_theme_constant_override("separation", 18);
	layout_margin->add_child(root_vb);

	title_label = memnew(Label);
	title_label->set_name("ProjectTitleLabel");
	title_label->set_h_size_flags(SIZE_EXPAND_FILL);
	title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	title_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	title_label->add_theme_font_size_override("font_size", 34);
	title_label->add_theme_color_override("font_color", Color(0.96f, 0.94f, 0.88f, 1.0f));
	title_label->add_theme_color_override("font_shadow_color", Color(0.0f, 0.0f, 0.0f, 0.78f));
	title_label->add_theme_constant_override("shadow_offset_x", 2);
	title_label->add_theme_constant_override("shadow_offset_y", 2);
	root_vb->add_child(title_label);

	title_texture_rect = memnew(TextureRect);
	title_texture_rect->set_name("ProjectTitleLogo");
	title_texture_rect->set_custom_minimum_size(Size2(380, 110));
	title_texture_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	title_texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	title_texture_rect->set_h_size_flags(SIZE_SHRINK_CENTER);
	title_texture_rect->hide();
	root_vb->add_child(title_texture_rect);

	main_menu_panel = memnew(PanelContainer);
	main_menu_panel->set_name("MainMenuPanel");
	main_menu_panel->set_h_size_flags(SIZE_SHRINK_CENTER);
	root_vb->add_child(main_menu_panel);

	entries_box = memnew(VBoxContainer);
	entries_box->set_name("EntriesBox");
	entries_box->add_theme_constant_override("separation", 8);
	main_menu_panel->add_child(entries_box);

	// Subpanel overlay: Continuar / Ranuras de Guardado
	ColorRect *cont_scrim = memnew(ColorRect);
	cont_scrim->set_name("ContinueOverlay");
	cont_scrim->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	cont_scrim->set_color(Color(0.0f, 0.0f, 0.0f, 0.45f));
	cont_scrim->hide();
	add_child(cont_scrim);
	continue_overlay = cont_scrim;

	CenterContainer *cont_center = memnew(CenterContainer);
	cont_center->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	cont_scrim->add_child(cont_center);

	continue_panel = memnew(PanelContainer);
	continue_panel->set_name("ContinuePanel");
	continue_panel->set_custom_minimum_size(Size2(540, 380));
	cont_center->add_child(continue_panel);

	VBoxContainer *cont_vb = memnew(VBoxContainer);
	cont_vb->add_theme_constant_override("separation", 10);
	continue_panel->add_child(cont_vb);

	Label *cont_title = memnew(Label(String::utf8("Seleccionar partida guardada")));
	cont_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	cont_title->add_theme_font_size_override("font_size", 20);
	cont_vb->add_child(cont_title);

	ScrollContainer *slots_scroll = memnew(ScrollContainer);
	slots_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	slots_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	cont_vb->add_child(slots_scroll);

	slots_box = memnew(VBoxContainer);
	slots_box->set_h_size_flags(SIZE_EXPAND_FILL);
	slots_box->add_theme_constant_override("separation", 6);
	slots_scroll->add_child(slots_box);

	btn_continue_back = memnew(Button(String::utf8("Volver")));
	btn_continue_back->set_h_size_flags(SIZE_SHRINK_CENTER);
	btn_continue_back->connect("pressed", callable_mp(this, &ClockGearsTitleScreen::close_submenus));
	cont_vb->add_child(btn_continue_back);

	// Subpanel overlay: Opciones
	ColorRect *opt_scrim = memnew(ColorRect);
	opt_scrim->set_name("OptionsOverlay");
	opt_scrim->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	opt_scrim->set_color(Color(0.0f, 0.0f, 0.0f, 0.45f));
	opt_scrim->hide();
	add_child(opt_scrim);
	options_overlay = opt_scrim;

	CenterContainer *opt_center = memnew(CenterContainer);
	opt_center->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	opt_scrim->add_child(opt_center);

	options_panel = memnew(PanelContainer);
	options_panel->set_name("OptionsPanel");
	options_panel->set_custom_minimum_size(Size2(480, 320));
	opt_center->add_child(options_panel);

	VBoxContainer *opt_vb = memnew(VBoxContainer);
	opt_vb->add_theme_constant_override("separation", 12);
	options_panel->add_child(opt_vb);

	Label *opt_title = memnew(Label(String::utf8("Opciones")));
	opt_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	opt_title->add_theme_font_size_override("font_size", 20);
	opt_vb->add_child(opt_title);

	auto make_slider_row = [&](const String &p_label_txt, double p_min, double p_max, double p_step, HSlider *&r_slider, Label *&r_val_lbl) {
		HBoxContainer *row = memnew(HBoxContainer);
		row->add_theme_constant_override("separation", 10);
		opt_vb->add_child(row);

		Label *lbl = memnew(Label(p_label_txt));
		lbl->set_custom_minimum_size(Size2(180, 0));
		row->add_child(lbl);

		r_slider = memnew(HSlider);
		r_slider->set_min(p_min);
		r_slider->set_max(p_max);
		r_slider->set_step(p_step);
		r_slider->set_h_size_flags(SIZE_EXPAND_FILL);
		r_slider->set_v_size_flags(SIZE_SHRINK_CENTER);
		row->add_child(r_slider);

		r_val_lbl = memnew(Label("100%"));
		r_val_lbl->set_custom_minimum_size(Size2(52, 0));
		r_val_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		row->add_child(r_val_lbl);
	};

	make_slider_row(String::utf8("Volumen música"), 0.0, 100.0, 1.0, music_slider, music_val_label);
	music_slider->connect("value_changed", callable_mp(this, &ClockGearsTitleScreen::_on_music_slider_changed));

	make_slider_row(String::utf8("Volumen efectos"), 0.0, 100.0, 1.0, sfx_slider, sfx_val_label);
	sfx_slider->connect("value_changed", callable_mp(this, &ClockGearsTitleScreen::_on_sfx_slider_changed));

	make_slider_row(String::utf8("Brillo"), 50.0, 150.0, 5.0, brightness_slider, brightness_val_label);
	brightness_slider->connect("value_changed", callable_mp(this, &ClockGearsTitleScreen::_on_brightness_slider_changed));

	HBoxContainer *fs_row = memnew(HBoxContainer);
	fs_row->add_theme_constant_override("separation", 10);
	opt_vb->add_child(fs_row);
	Label *fs_lbl = memnew(Label(String::utf8("Pantalla completa")));
	fs_lbl->set_custom_minimum_size(Size2(180, 0));
	fs_row->add_child(fs_lbl);
	fullscreen_check = memnew(CheckBox);
	fullscreen_check->set_text(String::utf8("Activar"));
	fullscreen_check->connect("toggled", callable_mp(this, &ClockGearsTitleScreen::_on_fullscreen_toggled));
	fs_row->add_child(fullscreen_check);

	HBoxContainer *opt_btns = memnew(HBoxContainer);
	opt_btns->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	opt_btns->add_theme_constant_override("separation", 12);
	opt_vb->add_child(opt_btns);

	btn_options_reset = memnew(Button(String::utf8("Predeterminados")));
	btn_options_reset->connect("pressed", callable_mp(this, &ClockGearsTitleScreen::_on_reset_options_pressed));
	opt_btns->add_child(btn_options_reset);

	btn_options_back = memnew(Button(String::utf8("Volver")));
	btn_options_back->connect("pressed", callable_mp(this, &ClockGearsTitleScreen::close_submenus));
	opt_btns->add_child(btn_options_back);

	// Focus neighbor links for Options panel
	music_slider->set_focus_neighbor(SIDE_BOTTOM, music_slider->get_path_to(sfx_slider));
	sfx_slider->set_focus_neighbor(SIDE_TOP, sfx_slider->get_path_to(music_slider));
	sfx_slider->set_focus_neighbor(SIDE_BOTTOM, sfx_slider->get_path_to(brightness_slider));
	brightness_slider->set_focus_neighbor(SIDE_TOP, brightness_slider->get_path_to(sfx_slider));
	brightness_slider->set_focus_neighbor(SIDE_BOTTOM, brightness_slider->get_path_to(fullscreen_check));
	fullscreen_check->set_focus_neighbor(SIDE_TOP, fullscreen_check->get_path_to(brightness_slider));
	fullscreen_check->set_focus_neighbor(SIDE_BOTTOM, fullscreen_check->get_path_to(btn_options_back));
	btn_options_reset->set_focus_neighbor(SIDE_TOP, btn_options_reset->get_path_to(fullscreen_check));
	btn_options_reset->set_focus_neighbor(SIDE_RIGHT, btn_options_reset->get_path_to(btn_options_back));
	btn_options_back->set_focus_neighbor(SIDE_TOP, btn_options_back->get_path_to(fullscreen_check));
	btn_options_back->set_focus_neighbor(SIDE_LEFT, btn_options_back->get_path_to(btn_options_reset));

	_apply_theme_styling();
}

void ClockGearsTitleScreen::_apply_title_config(const Dictionary &p_cfg, const Dictionary &p_sys_cfg, bool p_check_saves) {
	_build_base_ui();
	_apply_theme_styling();

	Dictionary cfg = ClockGearsDataManager::normalize_title_menu_config(p_cfg);
	Dictionary bg = cfg.get("background", Dictionary());
	String bg_col = String(bg.get("color", "#12151e"));
	bg_color_rect->set_color(Color::html_is_valid(bg_col) ? Color::html(bg_col) : Color::html("#12151e"));

	String bg_filter = String(bg.get("filter", "nearest"));
	bg_texture_rect->set_texture_filter(bg_filter == "linear" ? TEXTURE_FILTER_LINEAR : TEXTURE_FILTER_NEAREST);

	String bg_scale = String(bg.get("scale_mode", "fit"));
	if (bg_scale == "fill") {
		bg_texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_COVERED);
	} else if (bg_scale == "stretch") {
		bg_texture_rect->set_stretch_mode(TextureRect::STRETCH_SCALE);
	} else {
		bg_texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	}

	Ref<Texture2D> bg_tex;
	if (String(bg.get("type", "color")) == "image") {
		bg_tex = _load_texture(String(bg.get("image_path", "")));
	}
	if (bg_tex.is_valid()) {
		bg_texture_rect->set_texture(bg_tex);
		bg_texture_rect->show();
	} else {
		bg_texture_rect->set_texture(Ref<Texture2D>());
		bg_texture_rect->hide();
	}

	Dictionary title_cfg = cfg.get("title", Dictionary());
	String t_type = String(title_cfg.get("type", "text"));
	String t_filter = String(title_cfg.get("filter", "nearest"));
	title_texture_rect->set_texture_filter(t_filter == "linear" ? TEXTURE_FILTER_LINEAR : TEXTURE_FILTER_NEAREST);

	Ref<Texture2D> t_tex;
	if (t_type == "image") {
		t_tex = _load_texture(String(title_cfg.get("image_path", "")));
	}
	if (t_tex.is_valid()) {
		title_texture_rect->set_texture(t_tex);
		title_texture_rect->show();
		title_label->hide();
	} else {
		String custom_title = String(title_cfg.get("text", "")).strip_edges();
		String fallback_title = String(p_sys_cfg.get("game_title", "")).strip_edges();
		if (fallback_title.is_empty() && ClockGearsDataManager::get_singleton()) {
			fallback_title = ClockGearsDataManager::get_singleton()->get_effective_game_title();
		}
		if (fallback_title.is_empty()) {
			fallback_title = "ClockGears RPG";
		}
		title_label->set_text(custom_title.is_empty() ? fallback_title : custom_title);
		title_label->show();
		title_texture_rect->set_texture(Ref<Texture2D>());
		title_texture_rect->hide();
	}

	String bgm_path = String(cfg.get("bgm_path", "")).strip_edges();
	applied_bgm_path = bgm_path;
	if (!preview_mode && !editor_test_popup_mode && !(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
		if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
			if (!bgm_path.is_empty()) {
				am->play_bgm(bgm_path, 1.0f, 1.0f, 0.2f, 0.0f);
			}
		}
	}

	for (int i = entries_box->get_child_count() - 1; i >= 0; i--) {
		Node *ch = entries_box->get_child(i);
		entries_box->remove_child(ch);
		memdelete(ch);
	}
	visible_entry_ids.clear();
	visible_entry_texts.clear();

	Array raw_entries = cfg.get("entries", Array());
	Vector<Dictionary> sorted_entries;
	for (int i = 0; i < raw_entries.size(); i++) {
		if (raw_entries[i].get_type() == Variant::DICTIONARY) {
			sorted_entries.push_back(Dictionary(raw_entries[i]));
		}
	}
	struct OrderCmp {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("order", 0)) < int(b.get("order", 0));
		}
	};
	sorted_entries.sort_custom<OrderCmp>();

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	const bool credits_ok = bool(cfg.get("credits_configured", false));
	const bool has_saves = dm ? dm->has_any_save() : false;

	Vector<Button *> created_buttons;
	for (int i = 0; i < sorted_entries.size(); i++) {
		const Dictionary &e = sorted_entries[i];
		if (!bool(e.get("enabled", true))) {
			continue;
		}
		String id = String(e.get("id", "")).strip_edges().to_lower();
		if (id.is_empty()) {
			continue;
		}
		if (id == "credits" && !credits_ok) {
			continue;
		}
		if (id == "continue" && p_check_saves && !has_saves) {
			continue;
		}
		String txt = String(e.get("text", id));
		visible_entry_ids.push_back(id);
		visible_entry_texts.push_back(txt);

		Button *btn = memnew(Button(txt));
		btn->set_name(vformat("Entry_%s", id));
		btn->set_meta("entry_id", id);
		s_style_menu_button(btn);
		if (preview_mode) {
			btn->set_mouse_filter(MOUSE_FILTER_IGNORE);
			btn->set_focus_mode(FOCUS_NONE);
		} else {
			btn->set_focus_mode(FOCUS_ALL);
			btn->connect("pressed", callable_mp(this, &ClockGearsTitleScreen::_on_entry_button_pressed).bind(id));
		}
		entries_box->add_child(btn);
		created_buttons.push_back(btn);
	}

	for (int i = 0; i < created_buttons.size(); i++) {
		Button *b = created_buttons[i];
		Button *prev = created_buttons[(i - 1 + created_buttons.size()) % created_buttons.size()];
		Button *next = created_buttons[(i + 1) % created_buttons.size()];
		b->set_focus_neighbor(SIDE_TOP, b->get_path_to(prev));
		b->set_focus_neighbor(SIDE_BOTTOM, b->get_path_to(next));
	}
	if (!preview_mode && !created_buttons.is_empty() && is_inside_tree()) {
		created_buttons[0]->grab_focus();
	}

	_sync_options_controls_from_dm();
}

void ClockGearsTitleScreen::rebuild_title_screen() {
	if (preview_mode && !preview_title_config.is_empty()) {
		_apply_title_config(preview_title_config, preview_system_config, false);
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		_build_base_ui();
		return;
	}
	Dictionary sys = dm->get_system_config();
	_apply_title_config(dm->get_title_menu_config(), sys, true);
}

void ClockGearsTitleScreen::set_preview_mode(bool p_enable) {
	preview_mode = p_enable;
	if (preview_mode) {
		auto_quit_tree = false;
	}
	if (!preview_title_config.is_empty()) {
		_apply_title_config(preview_title_config, preview_system_config, !preview_mode);
	} else {
		rebuild_title_screen();
	}
}

void ClockGearsTitleScreen::set_editor_test_popup_mode(bool p_enable) {
	editor_test_popup_mode = p_enable;
	if (p_enable) {
		preview_mode = false;
		auto_quit_tree = false;
	}
}

void ClockGearsTitleScreen::apply_title_config_preview(const Dictionary &p_title_cfg, const Dictionary &p_sys_cfg) {
	preview_title_config = p_title_cfg.duplicate(true);
	preview_system_config = p_sys_cfg.duplicate(true);
	_apply_title_config(preview_title_config, preview_system_config, !preview_mode);
}

void ClockGearsTitleScreen::_populate_save_slots() {
	if (!slots_box) {
		return;
	}
	for (int i = slots_box->get_child_count() - 1; i >= 0; i--) {
		Node *ch = slots_box->get_child(i);
		slots_box->remove_child(ch);
		memdelete(ch);
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Array slots = dm->list_save_slots();
	Vector<Button *> nav_buttons;
	for (int i = 0; i < slots.size(); i++) {
		Dictionary info = slots[i];
		int slot_idx = int(info.get("slot_index", i + 1));
		bool exists = bool(info.get("exists", false));
		String text;
		if (exists) {
			String mname = String(info.get("map_name", ""));
			String sdate = String(info.get("saved_at", ""));
			String ptime = String(info.get("playtime_formatted", "00:00:00"));
			text = vformat(String::utf8("Ranura %02d — %s | %s | %s"), slot_idx, mname, sdate, ptime);
		} else {
			text = vformat(String::utf8("Ranura %02d — Vacío"), slot_idx);
		}
		Button *btn = memnew(Button(text));
		btn->set_h_size_flags(SIZE_EXPAND_FILL);
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		s_style_menu_button(btn, !exists);
		btn->set_disabled(!exists);
		btn->set_focus_mode(exists ? FOCUS_ALL : FOCUS_NONE);
		btn->set_modulate(exists ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.75f, 0.78f, 0.86f, 0.55f));
		if (exists) {
			btn->connect("pressed", callable_mp(this, &ClockGearsTitleScreen::_on_save_slot_pressed).bind(slot_idx));
			nav_buttons.push_back(btn);
		}
		slots_box->add_child(btn);
	}
	if (btn_continue_back) {
		nav_buttons.push_back(btn_continue_back);
	}
	for (int i = 0; i < nav_buttons.size(); i++) {
		Button *b = nav_buttons[i];
		Button *prev = nav_buttons[(i - 1 + nav_buttons.size()) % nav_buttons.size()];
		Button *next = nav_buttons[(i + 1) % nav_buttons.size()];
		b->set_focus_neighbor(SIDE_TOP, b->get_path_to(prev));
		b->set_focus_neighbor(SIDE_BOTTOM, b->get_path_to(next));
	}
}

void ClockGearsTitleScreen::_sync_options_controls_from_dm() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !music_slider || !sfx_slider || !brightness_slider || !fullscreen_check) {
		return;
	}
	syncing_options_ui = true;
	dm->ensure_player_options_loaded();
	Dictionary opts = dm->get_player_options();
	double mv = double(opts.get("music_volume", 0.8)) * 100.0;
	double sv = double(opts.get("sfx_volume", 0.8)) * 100.0;
	double bv = double(opts.get("brightness", 1.0)) * 100.0;
	bool fs = bool(opts.get("fullscreen", false));

	music_slider->set_value(mv);
	music_val_label->set_text(vformat("%d%%", int(Math::round(mv))));
	sfx_slider->set_value(sv);
	sfx_val_label->set_text(vformat("%d%%", int(Math::round(sv))));
	brightness_slider->set_value(bv);
	brightness_val_label->set_text(vformat("%d%%", int(Math::round(bv))));
	fullscreen_check->set_pressed(fs);
	syncing_options_ui = false;
}

void ClockGearsTitleScreen::_commit_options_from_controls() {
	if (syncing_options_ui || !music_slider || !sfx_slider || !brightness_slider || !fullscreen_check) {
		return;
	}
	apply_options_values(
			music_slider->get_value() / 100.0,
			sfx_slider->get_value() / 100.0,
			brightness_slider->get_value() / 100.0,
			fullscreen_check->is_pressed());
}

void ClockGearsTitleScreen::_on_music_slider_changed(double p_val) {
	if (music_val_label) {
		music_val_label->set_text(vformat("%d%%", int(Math::round(p_val))));
	}
	_commit_options_from_controls();
}

void ClockGearsTitleScreen::_on_sfx_slider_changed(double p_val) {
	if (sfx_val_label) {
		sfx_val_label->set_text(vformat("%d%%", int(Math::round(p_val))));
	}
	_commit_options_from_controls();
}

void ClockGearsTitleScreen::_on_brightness_slider_changed(double p_val) {
	if (brightness_val_label) {
		brightness_val_label->set_text(vformat("%d%%", int(Math::round(p_val))));
	}
	_commit_options_from_controls();
}

void ClockGearsTitleScreen::_on_fullscreen_toggled(bool p_pressed) {
	_commit_options_from_controls();
}

void ClockGearsTitleScreen::_on_reset_options_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		dm->set_player_options(ClockGearsDataManager::get_default_player_options(), true);
		_sync_options_controls_from_dm();
	}
}

void ClockGearsTitleScreen::_on_entry_button_pressed(const String &p_id) {
	trigger_entry(p_id);
}

void ClockGearsTitleScreen::_on_save_slot_pressed(int p_slot) {
	select_save_slot(p_slot);
}

bool ClockGearsTitleScreen::trigger_entry(const String &p_id) {
	String id = p_id.strip_edges().to_lower();
	if (!visible_entry_ids.has(id)) {
		return false;
	}
	if (id == "new_game") {
		return trigger_new_game();
	}
	if (id == "continue") {
		open_continue_menu();
		return true;
	}
	if (id == "options") {
		open_options_menu();
		return true;
	}
	if (id == "quit") {
		trigger_quit();
		return true;
	}
	return false;
}

bool ClockGearsTitleScreen::trigger_new_game() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	dm->reset_runtime_state();
	Dictionary sys = dm->get_system_config();
	sys["runtime_has_transfer"] = false;
	sys.erase("runtime_has_exact_pos");
	dm->set_system_config(sys);

	String start_map = String(sys.get("start_map", "")).strip_edges();
	last_started_map_path = start_map;
	emit_signal("new_game_started", start_map);

	if (is_inside_tree() && get_tree() && !start_map.is_empty() && ResourceLoader::exists(start_map)) {
		return get_tree()->change_scene_to_file(start_map) == OK;
	}
	return !start_map.is_empty();
}

void ClockGearsTitleScreen::open_continue_menu() {
	_build_base_ui();
	_populate_save_slots();
	if (options_overlay) {
		options_overlay->hide();
	}
	if (options_panel) {
		options_panel->hide();
	}
	if (main_menu_panel) {
		main_menu_panel->hide();
	}
	if (continue_overlay) {
		continue_overlay->show();
	}
	if (continue_panel) {
		continue_panel->show();
	}
	if (is_inside_tree()) {
		Vector<Button *> nav = _get_active_nav_buttons();
		if (!nav.is_empty() && nav[0]) {
			nav[0]->grab_focus();
		}
	}
}

bool ClockGearsTitleScreen::select_save_slot(int p_slot) {
	if (editor_test_popup_mode) {
		last_started_map_path = "preview_save_slot_" + itos(p_slot);
		emit_signal("save_loaded", p_slot, last_started_map_path);
		return true;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	bool ok = dm->start_game_from_save(p_slot, this);
	if (ok) {
		String loaded_map = dm->get_pending_loaded_map_path();
		last_started_map_path = loaded_map;
		emit_signal("save_loaded", p_slot, loaded_map);
	}
	return ok;
}

PackedStringArray ClockGearsTitleScreen::get_displayed_save_slot_texts() const {
	PackedStringArray out;
	if (!slots_box) {
		return out;
	}
	for (int i = 0; i < slots_box->get_child_count(); i++) {
		if (Button *b = Object::cast_to<Button>(slots_box->get_child(i))) {
			out.push_back(b->get_text());
		}
	}
	return out;
}

void ClockGearsTitleScreen::open_options_menu() {
	_build_base_ui();
	_sync_options_controls_from_dm();
	if (continue_overlay) {
		continue_overlay->hide();
	}
	if (continue_panel) {
		continue_panel->hide();
	}
	if (main_menu_panel) {
		main_menu_panel->hide();
	}
	if (options_overlay) {
		options_overlay->show();
	}
	if (options_panel) {
		options_panel->show();
	}
	if (is_inside_tree() && music_slider) {
		music_slider->grab_focus();
	}
}

void ClockGearsTitleScreen::close_submenus() {
	if (continue_overlay) {
		continue_overlay->hide();
	}
	if (continue_panel) {
		continue_panel->hide();
	}
	if (options_overlay) {
		options_overlay->hide();
	}
	if (options_panel) {
		options_panel->hide();
	}
	if (main_menu_panel) {
		main_menu_panel->show();
	}
	if (!preview_mode && entries_box && entries_box->get_child_count() > 0 && is_inside_tree()) {
		if (Button *b = Object::cast_to<Button>(entries_box->get_child(0))) {
			b->grab_focus();
		}
	}
}

bool ClockGearsTitleScreen::is_continue_menu_open() const {
	return continue_overlay && continue_overlay->is_visible() && continue_panel && continue_panel->is_visible();
}

bool ClockGearsTitleScreen::is_options_menu_open() const {
	return options_overlay && options_overlay->is_visible() && options_panel && options_panel->is_visible();
}

void ClockGearsTitleScreen::apply_options_values(double p_music, double p_sfx, double p_brightness, bool p_fullscreen) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary opts;
	opts["music_volume"] = p_music;
	opts["sfx_volume"] = p_sfx;
	opts["brightness"] = p_brightness;
	opts["fullscreen"] = p_fullscreen;
	if (editor_test_popup_mode) {
		Dictionary safe_opts = opts.duplicate(true);
		safe_opts["fullscreen"] = false;
		dm->set_player_options(safe_opts, false);
		dm->apply_player_options(this);
		syncing_options_ui = true;
		if (music_slider) music_slider->set_value(p_music * 100.0);
		if (music_val_label) music_val_label->set_text(vformat("%d%%", int(Math::round(p_music * 100.0))));
		if (sfx_slider) sfx_slider->set_value(p_sfx * 100.0);
		if (sfx_val_label) sfx_val_label->set_text(vformat("%d%%", int(Math::round(p_sfx * 100.0))));
		if (brightness_slider) brightness_slider->set_value(p_brightness * 100.0);
		if (brightness_val_label) brightness_val_label->set_text(vformat("%d%%", int(Math::round(p_brightness * 100.0))));
		if (fullscreen_check) fullscreen_check->set_pressed(p_fullscreen);
		syncing_options_ui = false;
		return;
	}
	dm->set_player_options(opts, true);
	dm->apply_player_options(this);
	_sync_options_controls_from_dm();
}

void ClockGearsTitleScreen::trigger_quit() {
	quit_requested_flag = true;
	emit_signal("quit_requested");
	if (auto_quit_tree && !preview_mode && !editor_test_popup_mode && is_inside_tree() && get_tree()) {
		get_tree()->quit();
	}
}

Vector<Button *> ClockGearsTitleScreen::_get_active_nav_buttons() const {
	Vector<Button *> out;
	if (is_continue_menu_open()) {
		if (slots_box) {
			for (int i = 0; i < slots_box->get_child_count(); i++) {
				Button *b = Object::cast_to<Button>(slots_box->get_child(i));
				if (b && b->is_visible() && !b->is_disabled()) {
					out.push_back(b);
				}
			}
		}
		if (btn_continue_back && btn_continue_back->is_visible()) {
			out.push_back(btn_continue_back);
		}
		return out;
	}
	if (is_options_menu_open()) {
		if (btn_options_reset && btn_options_reset->is_visible()) {
			out.push_back(btn_options_reset);
		}
		if (btn_options_back && btn_options_back->is_visible()) {
			out.push_back(btn_options_back);
		}
		return out;
	}
	if (entries_box) {
		for (int i = 0; i < entries_box->get_child_count(); i++) {
			Button *b = Object::cast_to<Button>(entries_box->get_child(i));
			if (b && b->is_visible() && !b->is_disabled()) {
				out.push_back(b);
			}
		}
	}
	return out;
}

bool ClockGearsTitleScreen::_move_focus_by(int p_delta) {
	if (!is_inside_tree() || !get_viewport()) {
		return false;
	}
	Control *cur = get_viewport()->gui_get_focus_owner();
	if (is_options_menu_open()) {
		Vector<Control *> controls;
		if (music_slider) controls.push_back(music_slider);
		if (sfx_slider) controls.push_back(sfx_slider);
		if (brightness_slider) controls.push_back(brightness_slider);
		if (fullscreen_check) controls.push_back(fullscreen_check);
		if (btn_options_reset) controls.push_back(btn_options_reset);
		if (btn_options_back) controls.push_back(btn_options_back);
		if (controls.is_empty()) {
			return false;
		}
		int idx = controls.find(cur);
		int next_idx = (idx < 0) ? (p_delta >= 0 ? 0 : controls.size() - 1) : ((idx + p_delta + controls.size()) % controls.size());
		controls[next_idx]->grab_focus();
		return true;
	}

	Vector<Button *> btns = _get_active_nav_buttons();
	if (btns.is_empty()) {
		return false;
	}
	int idx = -1;
	for (int i = 0; i < btns.size(); i++) {
		if (btns[i] == cur) {
			idx = i;
			break;
		}
	}
	int next_idx = (idx < 0) ? (p_delta >= 0 ? 0 : btns.size() - 1) : ((idx + p_delta + btns.size()) % btns.size());
	btns[next_idx]->grab_focus();
	return true;
}

bool ClockGearsTitleScreen::_activate_focused_button() {
	if (!is_inside_tree() || !get_viewport()) {
		return false;
	}
	Control *cur = get_viewport()->gui_get_focus_owner();
	Button *focused_btn = Object::cast_to<Button>(cur);
	if (!focused_btn || focused_btn->is_disabled() || !focused_btn->is_visible_in_tree()) {
		Vector<Button *> btns = _get_active_nav_buttons();
		if (btns.is_empty()) {
			return false;
		}
		focused_btn = btns[0];
	}
	if (CheckBox *chk = Object::cast_to<CheckBox>(focused_btn)) {
		chk->set_pressed(!chk->is_pressed());
		return true;
	}
	if (focused_btn == btn_continue_back || focused_btn == btn_options_back) {
		close_submenus();
		return true;
	}
	if (focused_btn == btn_options_reset) {
		_on_reset_options_pressed();
		return true;
	}
	if (focused_btn->has_meta("entry_id")) {
		return trigger_entry(String(focused_btn->get_meta("entry_id")));
	}
	focused_btn->emit_signal("pressed");
	return true;
}

bool ClockGearsTitleScreen::handle_navigation_key(Key p_keycode) {
	if (preview_mode || !is_visible_in_tree()) {
		return false;
	}
	if (p_keycode == Key::ESCAPE) {
		if (is_continue_menu_open() || is_options_menu_open()) {
			close_submenus();
			return true;
		}
		if (editor_test_popup_mode) {
			trigger_quit();
			return true;
		}
		return false;
	}
	if (p_keycode == Key::UP) {
		return _move_focus_by(-1);
	}
	if (p_keycode == Key::DOWN) {
		return _move_focus_by(1);
	}
	if (p_keycode == Key::ENTER || p_keycode == Key::KP_ENTER) {
		return _activate_focused_button();
	}
	return false;
}

void ClockGearsTitleScreen::unhandled_input(const Ref<InputEvent> &p_event) {
	if (preview_mode || !is_visible_in_tree() || p_event.is_null()) {
		return;
	}
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (handle_navigation_key(k->get_keycode())) {
			if (Viewport *vp = get_viewport()) {
				vp->set_input_as_handled();
			}
		}
	}
}

void ClockGearsTitleScreen::gui_input(const Ref<InputEvent> &p_event) {
	if (preview_mode || !is_visible_in_tree() || p_event.is_null()) {
		return;
	}
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (handle_navigation_key(k->get_keycode())) {
			accept_event();
		}
	}
}

Dictionary ClockGearsTitleScreen::get_layout_bounds_info(const Vector2 &p_viewport_size) const {
	Dictionary out;
	Vector2 vp_sz = p_viewport_size;
	if (vp_sz.x <= 0.0f || vp_sz.y <= 0.0f) {
		vp_sz = get_size();
		if (vp_sz.x <= 0.0f || vp_sz.y <= 0.0f) {
			vp_sz = get_viewport_rect().size;
		}
	}
	Rect2 vp_rect(Vector2(), vp_sz);
	out["viewport_rect"] = vp_rect;

	auto rel_rect = [&](const Control *p_ctrl) -> Rect2 {
		if (!p_ctrl || !p_ctrl->is_visible()) {
			return Rect2();
		}
		Vector2 origin = get_global_position();
		return Rect2(p_ctrl->get_global_position() - origin, p_ctrl->get_size());
	};

	const Control *active_title = (title_texture_rect && title_texture_rect->is_visible())
			? static_cast<const Control *>(title_texture_rect)
			: static_cast<const Control *>(title_label);
	Rect2 title_rect = rel_rect(active_title);
	Rect2 menu_rect = rel_rect(main_menu_panel);
	Rect2 cont_rect = is_continue_menu_open() ? rel_rect(continue_panel) : Rect2();
	Rect2 opt_rect = is_options_menu_open() ? rel_rect(options_panel) : Rect2();

	out["title_rect"] = title_rect;
	out["menu_panel_rect"] = menu_rect;
	out["continue_panel_rect"] = cont_rect;
	out["options_panel_rect"] = opt_rect;

	Array btn_rects;
	int corner_rad = 0;
	if (entries_box) {
		for (int i = 0; i < entries_box->get_child_count(); i++) {
			Button *b = Object::cast_to<Button>(entries_box->get_child(i));
			if (b && b->is_visible()) {
				Rect2 br = rel_rect(b);
				btn_rects.push_back(br);
				if (corner_rad == 0) {
					Ref<StyleBoxFlat> sb = b->get_theme_stylebox("normal");
					if (sb.is_valid()) {
						corner_rad = sb->get_corner_radius(CORNER_TOP_LEFT);
					}
				}
			}
		}
	}
	out["button_rects"] = btn_rects;
	out["button_corner_radius"] = corner_rad;

	auto rect_fits = [&](const Rect2 &r) -> bool {
		if (r.size.x <= 0.0f || r.size.y <= 0.0f) {
			return true;
		}
		return r.position.x >= -0.5f && r.position.y >= -0.5f &&
				r.position.x + r.size.x <= vp_rect.size.x + 0.5f &&
				r.position.y + r.size.y <= vp_rect.size.y + 0.5f;
	};

	bool all_fit = rect_fits(title_rect) && rect_fits(menu_rect) && rect_fits(cont_rect) && rect_fits(opt_rect);
	for (int i = 0; i < btn_rects.size(); i++) {
		if (!rect_fits(Rect2(btn_rects[i]))) {
			all_fit = false;
			break;
		}
	}
	out["all_within_viewport"] = all_fit;

	bool overlap = false;
	if (title_rect.size.x > 0 && menu_rect.size.x > 0 && title_rect.intersects(menu_rect)) {
		overlap = true;
	}
	for (int i = 0; i < btn_rects.size() && !overlap; i++) {
		Rect2 r1 = btn_rects[i];
		for (int j = i + 1; j < btn_rects.size(); j++) {
			Rect2 r2 = btn_rects[j];
			if (r1.intersects(r2)) {
				overlap = true;
				break;
			}
		}
	}
	out["any_overlap"] = overlap;
	return out;
}

String ClockGearsTitleScreen::get_title_text_displayed() const {
	return title_label ? title_label->get_text() : String();
}

bool ClockGearsTitleScreen::is_title_image_active() const {
	return title_texture_rect && title_texture_rect->is_visible() && title_texture_rect->get_texture().is_valid();
}

bool ClockGearsTitleScreen::is_background_image_active() const {
	return bg_texture_rect && bg_texture_rect->is_visible() && bg_texture_rect->get_texture().is_valid();
}

String ClockGearsTitleScreen::get_background_color_applied() const {
	return bg_color_rect ? bg_color_rect->get_color().to_html(false) : String();
}

String ClockGearsTitleScreen::get_background_scale_mode_applied() const {
	if (!bg_texture_rect) {
		return "fit";
	}
	switch (bg_texture_rect->get_stretch_mode()) {
		case TextureRect::STRETCH_KEEP_ASPECT_COVERED: return "fill";
		case TextureRect::STRETCH_SCALE: return "stretch";
		default: return "fit";
	}
}

String ClockGearsTitleScreen::get_background_filter_applied() const {
	if (!bg_texture_rect) {
		return "nearest";
	}
	return bg_texture_rect->get_texture_filter() == TEXTURE_FILTER_LINEAR ? "linear" : "nearest";
}

void ClockGearsTitleScreen::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		set_process_unhandled_input(!preview_mode);
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm && !preview_mode && !(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
			if (dm->get_category_array(CAT_ACTORS).is_empty() && FileAccess::exists("res://clockgears_data/database.json")) {
				dm->load_all("res://clockgears_data");
			}
			dm->ensure_player_options_loaded();
			dm->apply_player_options(this);
			dm->apply_game_window_title(this);
		}
		rebuild_title_screen();
	}
}

void ClockGearsTitleScreen::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsTitleScreen", D_METHOD("ensure_title_screen_scene", "scene_path"), &ClockGearsTitleScreen::ensure_title_screen_scene, DEFVAL("res://title_screen.tscn"));
	ClassDB::bind_method(D_METHOD("rebuild_title_screen"), &ClockGearsTitleScreen::rebuild_title_screen);
	ClassDB::bind_method(D_METHOD("set_preview_mode", "enable"), &ClockGearsTitleScreen::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsTitleScreen::is_preview_mode);
	ClassDB::bind_method(D_METHOD("set_editor_test_popup_mode", "enable"), &ClockGearsTitleScreen::set_editor_test_popup_mode);
	ClassDB::bind_method(D_METHOD("is_editor_test_popup_mode"), &ClockGearsTitleScreen::is_editor_test_popup_mode);
	ClassDB::bind_method(D_METHOD("apply_title_config_preview", "title_cfg", "sys_cfg"), &ClockGearsTitleScreen::apply_title_config_preview, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("handle_navigation_key", "keycode"), &ClockGearsTitleScreen::handle_navigation_key);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsTitleScreen::get_layout_bounds_info, DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("get_visible_entry_ids"), &ClockGearsTitleScreen::get_visible_entry_ids);
	ClassDB::bind_method(D_METHOD("get_visible_entry_texts"), &ClockGearsTitleScreen::get_visible_entry_texts);
	ClassDB::bind_method(D_METHOD("trigger_entry", "id"), &ClockGearsTitleScreen::trigger_entry);
	ClassDB::bind_method(D_METHOD("trigger_new_game"), &ClockGearsTitleScreen::trigger_new_game);
	ClassDB::bind_method(D_METHOD("open_continue_menu"), &ClockGearsTitleScreen::open_continue_menu);
	ClassDB::bind_method(D_METHOD("select_save_slot", "slot"), &ClockGearsTitleScreen::select_save_slot);
	ClassDB::bind_method(D_METHOD("get_displayed_save_slot_texts"), &ClockGearsTitleScreen::get_displayed_save_slot_texts);
	ClassDB::bind_method(D_METHOD("open_options_menu"), &ClockGearsTitleScreen::open_options_menu);
	ClassDB::bind_method(D_METHOD("close_submenus"), &ClockGearsTitleScreen::close_submenus);
	ClassDB::bind_method(D_METHOD("is_continue_menu_open"), &ClockGearsTitleScreen::is_continue_menu_open);
	ClassDB::bind_method(D_METHOD("is_options_menu_open"), &ClockGearsTitleScreen::is_options_menu_open);
	ClassDB::bind_method(D_METHOD("apply_options_values", "music", "sfx", "brightness", "fullscreen"), &ClockGearsTitleScreen::apply_options_values);
	ClassDB::bind_method(D_METHOD("trigger_quit"), &ClockGearsTitleScreen::trigger_quit);
	ClassDB::bind_method(D_METHOD("set_auto_quit_tree", "enable"), &ClockGearsTitleScreen::set_auto_quit_tree);
	ClassDB::bind_method(D_METHOD("is_auto_quit_tree"), &ClockGearsTitleScreen::is_auto_quit_tree);
	ClassDB::bind_method(D_METHOD("was_quit_requested"), &ClockGearsTitleScreen::was_quit_requested);
	ClassDB::bind_method(D_METHOD("get_title_text_displayed"), &ClockGearsTitleScreen::get_title_text_displayed);
	ClassDB::bind_method(D_METHOD("is_title_image_active"), &ClockGearsTitleScreen::is_title_image_active);
	ClassDB::bind_method(D_METHOD("is_background_image_active"), &ClockGearsTitleScreen::is_background_image_active);
	ClassDB::bind_method(D_METHOD("get_background_color_applied"), &ClockGearsTitleScreen::get_background_color_applied);
	ClassDB::bind_method(D_METHOD("get_background_scale_mode_applied"), &ClockGearsTitleScreen::get_background_scale_mode_applied);
	ClassDB::bind_method(D_METHOD("get_background_filter_applied"), &ClockGearsTitleScreen::get_background_filter_applied);
	ClassDB::bind_method(D_METHOD("get_applied_bgm_path"), &ClockGearsTitleScreen::get_applied_bgm_path);
	ClassDB::bind_method(D_METHOD("get_last_started_map_path"), &ClockGearsTitleScreen::get_last_started_map_path);

	ADD_SIGNAL(MethodInfo("new_game_started", PropertyInfo(Variant::STRING, "start_map")));
	ADD_SIGNAL(MethodInfo("save_loaded", PropertyInfo(Variant::INT, "slot"), PropertyInfo(Variant::STRING, "map_path")));
	ADD_SIGNAL(MethodInfo("quit_requested"));
}

ClockGearsTitleScreen::ClockGearsTitleScreen() {
	set_process_unhandled_input(true);
}
