#include "clockgears_title_screen.h"
#include "../core/clockgears_data_manager.h"
#include "../editor/clockgears_theme.h"
#include "clockgears_audio_manager.h"
#include "clockgears_credits.h"
#include "clockgears_game_menu.h"
#include "clockgears_ui_design_layer.h"
#include "clockgears_window_style.h"

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

Ref<Texture2D> ClockGearsTitleScreen::load_runtime_texture(const String &p_path) {
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

void ClockGearsTitleScreen::apply_screen_background(ColorRect *p_color_rect, TextureRect *p_tex_rect, const Dictionary &p_bg_cfg, const String &p_default_color) {
	String bg_col = String(p_bg_cfg.get("color", p_default_color));
	if (p_color_rect) {
		p_color_rect->set_color(Color::html_is_valid(bg_col) ? Color::html(bg_col) : Color::html(p_default_color));
	}
	if (!p_tex_rect) {
		return;
	}
	String bg_filter = String(p_bg_cfg.get("filter", "nearest"));
	p_tex_rect->set_texture_filter(bg_filter == "linear" ? TEXTURE_FILTER_LINEAR : TEXTURE_FILTER_NEAREST);

	String bg_scale = String(p_bg_cfg.get("scale_mode", "fit"));
	if (bg_scale == "fill") {
		p_tex_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_COVERED);
	} else if (bg_scale == "stretch") {
		p_tex_rect->set_stretch_mode(TextureRect::STRETCH_SCALE);
	} else {
		p_tex_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	}

	Ref<Texture2D> bg_tex;
	if (String(p_bg_cfg.get("type", "color")) == "image") {
		bg_tex = load_runtime_texture(String(p_bg_cfg.get("image_path", "")));
	}
	if (bg_tex.is_valid()) {
		p_tex_rect->set_texture(bg_tex);
		p_tex_rect->show();
	} else {
		p_tex_rect->set_texture(Ref<Texture2D>());
		p_tex_rect->hide();
	}
}

Vector<Button *> ClockGearsTitleScreen::populate_save_slots_into(VBoxContainer *p_slots_box, Button *p_back_btn, bool p_allow_empty, const Callable &p_on_slot_pressed) {
	Vector<Button *> nav_buttons;
	if (!p_slots_box) {
		return nav_buttons;
	}
	for (int i = p_slots_box->get_child_count() - 1; i >= 0; i--) {
		Node *ch = p_slots_box->get_child(i);
		p_slots_box->remove_child(ch);
		memdelete(ch);
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return nav_buttons;
	}
	Array slots = dm->list_save_slots();
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
		bool selectable = exists || p_allow_empty;
		Button *btn = memnew(Button(text));
		btn->set_h_size_flags(SIZE_EXPAND_FILL);
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		btn->set_custom_minimum_size(Size2(220, 36));
		btn->set_clip_text(true);
		btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		ClockGearsWindowStyleManager::apply_button_style(btn, "save_list");
		btn->set_disabled(!selectable);
		btn->set_focus_mode(selectable ? FOCUS_ALL : FOCUS_NONE);
		btn->set_modulate(exists ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.75f, 0.78f, 0.86f, p_allow_empty ? 0.82f : 0.55f));
		if (selectable) {
			if (p_on_slot_pressed.is_valid()) {
				btn->connect("pressed", p_on_slot_pressed.bind(slot_idx));
			}
			nav_buttons.push_back(btn);
		}
		p_slots_box->add_child(btn);
	}
	if (p_back_btn) {
		nav_buttons.push_back(p_back_btn);
	}
	for (int i = 0; i < nav_buttons.size(); i++) {
		Button *b = nav_buttons[i];
		Button *prev = nav_buttons[(i - 1 + nav_buttons.size()) % nav_buttons.size()];
		Button *next = nav_buttons[(i + 1) % nav_buttons.size()];
		b->set_focus_neighbor(SIDE_TOP, b->get_path_to(prev));
		b->set_focus_neighbor(SIDE_BOTTOM, b->get_path_to(next));
	}
	return nav_buttons;
}

// --- ClockGearsOptionsBox ---

void ClockGearsOptionsBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("sync_from_dm"), &ClockGearsOptionsBox::sync_from_dm);
	ClassDB::bind_method(D_METHOD("apply_values", "music", "sfx", "brightness", "fullscreen", "ctx"), &ClockGearsOptionsBox::apply_values, DEFVAL(Variant()));
	ADD_SIGNAL(MethodInfo("back_requested"));
}

void ClockGearsOptionsBox::sync_from_dm() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !music_slider || !sfx_slider || !brightness_slider || !fullscreen_check) {
		return;
	}
	syncing = true;
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
	syncing = false;
}

void ClockGearsOptionsBox::apply_values(double p_music, double p_sfx, double p_brightness, bool p_fullscreen, Node *p_ctx) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary opts;
	opts["music_volume"] = p_music;
	opts["sfx_volume"] = p_sfx;
	opts["brightness"] = p_brightness;
	opts["fullscreen"] = p_fullscreen;
	Node *ctx = p_ctx ? p_ctx : this;
	if (!persist_on_change) {
		Dictionary safe_opts = opts.duplicate(true);
		safe_opts["fullscreen"] = false;
		dm->set_player_options(safe_opts, false);
		dm->apply_player_options(ctx);
		syncing = true;
		if (music_slider) music_slider->set_value(p_music * 100.0);
		if (music_val_label) music_val_label->set_text(vformat("%d%%", int(Math::round(p_music * 100.0))));
		if (sfx_slider) sfx_slider->set_value(p_sfx * 100.0);
		if (sfx_val_label) sfx_val_label->set_text(vformat("%d%%", int(Math::round(p_sfx * 100.0))));
		if (brightness_slider) brightness_slider->set_value(p_brightness * 100.0);
		if (brightness_val_label) brightness_val_label->set_text(vformat("%d%%", int(Math::round(p_brightness * 100.0))));
		if (fullscreen_check) fullscreen_check->set_pressed(p_fullscreen);
		syncing = false;
		return;
	}
	dm->set_player_options(opts, true);
	dm->apply_player_options(ctx);
	sync_from_dm();
}

void ClockGearsOptionsBox::_commit_from_controls() {
	if (syncing || !music_slider || !sfx_slider || !brightness_slider || !fullscreen_check) {
		return;
	}
	apply_values(
			music_slider->get_value() / 100.0,
			sfx_slider->get_value() / 100.0,
			brightness_slider->get_value() / 100.0,
			fullscreen_check->is_pressed(),
			this);
}

void ClockGearsOptionsBox::_on_music_changed(double p_val) {
	if (music_val_label) {
		music_val_label->set_text(vformat("%d%%", int(Math::round(p_val))));
	}
	_commit_from_controls();
}

void ClockGearsOptionsBox::_on_sfx_changed(double p_val) {
	if (sfx_val_label) {
		sfx_val_label->set_text(vformat("%d%%", int(Math::round(p_val))));
	}
	_commit_from_controls();
}

void ClockGearsOptionsBox::_on_brightness_changed(double p_val) {
	if (brightness_val_label) {
		brightness_val_label->set_text(vformat("%d%%", int(Math::round(p_val))));
	}
	_commit_from_controls();
}

void ClockGearsOptionsBox::_on_fullscreen_toggled(bool p_pressed) {
	_commit_from_controls();
}

void ClockGearsOptionsBox::_on_reset_pressed() {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		dm->set_player_options(ClockGearsDataManager::get_default_player_options(), persist_on_change);
		dm->apply_player_options(this);
		sync_from_dm();
	}
}

void ClockGearsOptionsBox::_on_back_pressed() {
	emit_signal("back_requested");
}

void ClockGearsOptionsBox::apply_button_theme() {
	if (btn_reset) {
		ClockGearsWindowStyleManager::apply_button_style(btn_reset, "options");
		btn_reset->set_custom_minimum_size(Size2(150, 34));
	}
	if (btn_back) {
		ClockGearsWindowStyleManager::apply_button_style(btn_back, "options");
		btn_back->set_custom_minimum_size(Size2(150, 34));
	}
}

ClockGearsOptionsBox::ClockGearsOptionsBox() {
	add_theme_constant_override("separation", 12);

	Label *opt_title = memnew(Label(String::utf8("Opciones")));
	opt_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	opt_title->add_theme_font_size_override("font_size", 20);
	add_child(opt_title);

	auto make_slider_row = [&](const String &p_label_txt, double p_min, double p_max, double p_step, HSlider *&r_slider, Label *&r_val_lbl) {
		HBoxContainer *row = memnew(HBoxContainer);
		row->add_theme_constant_override("separation", 10);
		add_child(row);

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
	music_slider->connect("value_changed", callable_mp(this, &ClockGearsOptionsBox::_on_music_changed));

	make_slider_row(String::utf8("Volumen efectos"), 0.0, 100.0, 1.0, sfx_slider, sfx_val_label);
	sfx_slider->connect("value_changed", callable_mp(this, &ClockGearsOptionsBox::_on_sfx_changed));

	make_slider_row(String::utf8("Brillo"), 50.0, 150.0, 5.0, brightness_slider, brightness_val_label);
	brightness_slider->connect("value_changed", callable_mp(this, &ClockGearsOptionsBox::_on_brightness_changed));

	HBoxContainer *fs_row = memnew(HBoxContainer);
	fs_row->add_theme_constant_override("separation", 10);
	add_child(fs_row);
	Label *fs_lbl = memnew(Label(String::utf8("Pantalla completa")));
	fs_lbl->set_custom_minimum_size(Size2(180, 0));
	fs_row->add_child(fs_lbl);
	fullscreen_check = memnew(CheckBox);
	fullscreen_check->set_text(String::utf8("Activar"));
	fullscreen_check->connect("toggled", callable_mp(this, &ClockGearsOptionsBox::_on_fullscreen_toggled));
	fs_row->add_child(fullscreen_check);

	HBoxContainer *opt_btns = memnew(HBoxContainer);
	opt_btns->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	opt_btns->add_theme_constant_override("separation", 12);
	add_child(opt_btns);

	btn_reset = memnew(Button(String::utf8("Predeterminados")));
	btn_reset->set_clip_text(true);
	btn_reset->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	btn_reset->connect("pressed", callable_mp(this, &ClockGearsOptionsBox::_on_reset_pressed));
	opt_btns->add_child(btn_reset);

	btn_back = memnew(Button(String::utf8("Volver")));
	btn_back->set_clip_text(true);
	btn_back->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	btn_back->connect("pressed", callable_mp(this, &ClockGearsOptionsBox::_on_back_pressed));
	opt_btns->add_child(btn_back);

	music_slider->set_focus_neighbor(SIDE_BOTTOM, music_slider->get_path_to(sfx_slider));
	sfx_slider->set_focus_neighbor(SIDE_TOP, sfx_slider->get_path_to(music_slider));
	sfx_slider->set_focus_neighbor(SIDE_BOTTOM, sfx_slider->get_path_to(brightness_slider));
	brightness_slider->set_focus_neighbor(SIDE_TOP, brightness_slider->get_path_to(sfx_slider));
	brightness_slider->set_focus_neighbor(SIDE_BOTTOM, brightness_slider->get_path_to(fullscreen_check));
	fullscreen_check->set_focus_neighbor(SIDE_TOP, fullscreen_check->get_path_to(brightness_slider));
	fullscreen_check->set_focus_neighbor(SIDE_BOTTOM, fullscreen_check->get_path_to(btn_back));
	btn_reset->set_focus_neighbor(SIDE_TOP, btn_reset->get_path_to(fullscreen_check));
	btn_reset->set_focus_neighbor(SIDE_RIGHT, btn_reset->get_path_to(btn_back));
	btn_back->set_focus_neighbor(SIDE_TOP, btn_back->get_path_to(fullscreen_check));
	btn_back->set_focus_neighbor(SIDE_LEFT, btn_back->get_path_to(btn_reset));
	apply_button_theme();
}

Ref<Texture2D> ClockGearsTitleScreen::_load_texture(const String &p_path) const {
	return load_runtime_texture(p_path);
}

void ClockGearsTitleScreen::_apply_theme_styling() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_control(this);
	}
	if (main_menu_panel) {
		ClockGearsWindowStyleManager::apply_window_style(main_menu_panel, "title_buttons");
	}
	if (entries_box) {
		ClockGearsWindowStyleManager::apply_buttons_in_container(entries_box, "title_buttons");
		for (int i = 0; i < entries_box->get_child_count(); i++) {
			Button *b = Object::cast_to<Button>(entries_box->get_child(i));
			if (b) {
				b->set_custom_minimum_size(Size2(220, 36));
			}
		}
	}
	if (continue_panel) {
		ClockGearsWindowStyleManager::apply_window_style(continue_panel, "save_list");
	}
	if (slots_box) {
		ClockGearsWindowStyleManager::apply_buttons_in_container(slots_box, "save_list");
	}
	if (options_panel) {
		ClockGearsWindowStyleManager::apply_window_style(options_panel, "options");
	}
	if (btn_continue_back) {
		ClockGearsWindowStyleManager::apply_button_style(btn_continue_back, "save_list");
		btn_continue_back->set_custom_minimum_size(Size2(180, 34));
	}
	if (btn_options_reset) {
		ClockGearsWindowStyleManager::apply_button_style(btn_options_reset, "options");
		btn_options_reset->set_custom_minimum_size(Size2(150, 34));
	}
	if (btn_options_back) {
		ClockGearsWindowStyleManager::apply_button_style(btn_options_back, "options");
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

	custom_ui_layer = memnew(ClockGearsUiDesignLayer);
	custom_ui_layer->set_name("CustomTitleUiLayer");
	custom_ui_layer->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	custom_ui_layer->set_window_type_fallback("title_buttons");
	custom_ui_layer->hide();
	custom_ui_layer->connect("action_triggered", callable_mp(this, &ClockGearsTitleScreen::_on_custom_ui_action));
	add_child(custom_ui_layer);

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
	btn_continue_back->set_clip_text(true);
	btn_continue_back->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
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

	options_box = memnew(ClockGearsOptionsBox);
	options_panel->add_child(options_box);
	options_box->connect("back_requested", callable_mp(this, &ClockGearsTitleScreen::close_submenus));
	music_slider = options_box->get_music_slider();
	music_val_label = options_box->get_music_val_label();
	sfx_slider = options_box->get_sfx_slider();
	sfx_val_label = options_box->get_sfx_val_label();
	brightness_slider = options_box->get_brightness_slider();
	brightness_val_label = options_box->get_brightness_val_label();
	fullscreen_check = options_box->get_fullscreen_check();
	btn_options_reset = options_box->get_reset_button();
	btn_options_back = options_box->get_back_button();

	credits_player = memnew(ClockGearsCreditsPlayer);
	credits_player->set_name("TitleCreditsPlayer");
	credits_player->set_visible(false);
	credits_player->connect("credits_finished", callable_mp(this, &ClockGearsTitleScreen::_on_title_credits_finished));
	add_child(credits_player);

	_apply_theme_styling();
}

void ClockGearsTitleScreen::_apply_title_config(const Dictionary &p_cfg, const Dictionary &p_sys_cfg, bool p_check_saves) {
	_build_base_ui();
	_apply_theme_styling();

	Dictionary cfg = ClockGearsDataManager::normalize_title_menu_config(p_cfg);
	Dictionary bg = cfg.get("background", Dictionary());
	apply_screen_background(bg_color_rect, bg_texture_rect, bg, "#12151e");

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
	if (ClockGearsDataManager *dm_bgm = ClockGearsDataManager::get_singleton()) {
		if (dm_bgm->has_custom_ui_design("title")) {
			Dictionary td = dm_bgm->get_ui_screen_design("title");
			String custom_bgm = String(td.get("bgm_path", "")).strip_edges();
			if (!custom_bgm.is_empty()) {
				bgm_path = custom_bgm;
			}
		}
	}
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
	const bool credits_ok = dm ? (dm->has_credits_lines() || dm->has_custom_ui_design("credits")) : (bool(cfg.get("credits_configured", false)) && !Array(cfg.get("credits", Array())).is_empty());
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
		btn->set_custom_minimum_size(Size2(220, 36));
		btn->set_clip_text(true);
		btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		ClockGearsWindowStyleManager::apply_button_style(btn, "title_buttons");
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

	custom_ui_active = (dm && dm->has_custom_ui_design("title"));
	if (custom_ui_active && custom_ui_layer) {
		if (title_label) title_label->hide();
		if (title_texture_rect) title_texture_rect->hide();
		if (main_menu_panel) main_menu_panel->hide();
		custom_ui_layer->set_window_type_fallback("title_buttons");
		custom_ui_layer->set_preview_mode(preview_mode);
		custom_ui_layer->clear_disabled_actions();
		custom_ui_layer->set_action_disabled("continue", p_check_saves && !has_saves);
		custom_ui_layer->set_action_disabled("credits", !credits_ok);
		custom_ui_layer->apply_screen_design("title");
		custom_ui_layer->show();
		if (!preview_mode) {
			custom_ui_layer->play_transition_in();
		}

		Vector<Button *> custom_btns = custom_ui_layer->get_focus_ordered_buttons();
		for (int i = 0; i < custom_btns.size(); i++) {
			Button *b = custom_btns[i];
			Button *prev = custom_btns[(i - 1 + custom_btns.size()) % custom_btns.size()];
			Button *next = custom_btns[(i + 1) % custom_btns.size()];
			b->set_focus_neighbor(SIDE_TOP, b->get_path_to(prev));
			b->set_focus_neighbor(SIDE_BOTTOM, b->get_path_to(next));
		}
		if (!preview_mode && !custom_btns.is_empty() && is_inside_tree()) {
			custom_btns[0]->grab_focus();
		}
	} else {
		if (main_menu_panel) {
			main_menu_panel->show();
		}
		if (custom_ui_layer) {
			custom_ui_layer->hide();
			custom_ui_layer->clear_elements();
		}
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
	if (options_box) {
		options_box->set_persist_on_change(!p_enable);
	}
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
	populate_save_slots_into(slots_box, btn_continue_back, false, callable_mp(this, &ClockGearsTitleScreen::_on_save_slot_pressed));
}

void ClockGearsTitleScreen::_sync_options_controls_from_dm() {
	if (options_box) {
		options_box->sync_from_dm();
	}
}

void ClockGearsTitleScreen::_commit_options_from_controls() {
}

void ClockGearsTitleScreen::_on_music_slider_changed(double p_val) {
}

void ClockGearsTitleScreen::_on_sfx_slider_changed(double p_val) {
}

void ClockGearsTitleScreen::_on_brightness_slider_changed(double p_val) {
}

void ClockGearsTitleScreen::_on_fullscreen_toggled(bool p_pressed) {
}

void ClockGearsTitleScreen::_on_reset_options_pressed() {
	if (options_box) {
		options_box->apply_values(0.8, 0.8, 1.0, false, this);
	}
}

void ClockGearsTitleScreen::_on_entry_button_pressed(const String &p_id) {
	trigger_entry(p_id);
}

ClockGearsGameMenu *ClockGearsTitleScreen::_ensure_custom_sub_menu() {
	if (!custom_sub_menu) {
		custom_sub_menu = memnew(ClockGearsGameMenu);
		custom_sub_menu->set_name("TitleCustomSubMenu");
		custom_sub_menu->set_auto_quit_tree(false);
		custom_sub_menu->set_editor_test_popup_mode(editor_test_popup_mode);
		custom_sub_menu->connect("menu_closed", callable_mp(this, &ClockGearsTitleScreen::_on_custom_sub_menu_closed));
		add_child(custom_sub_menu);
	}
	return custom_sub_menu;
}

void ClockGearsTitleScreen::_on_custom_sub_menu_closed() {
	if (custom_ui_active && custom_ui_layer) {
		custom_ui_layer->show();
		custom_ui_layer->refresh_live_values();
		if (!preview_mode && is_inside_tree()) {
			Vector<Button *> cbtns = custom_ui_layer->get_focus_ordered_buttons();
			if (!cbtns.is_empty()) {
				cbtns[0]->grab_focus();
			}
		}
		return;
	}
	if (main_menu_panel) {
		main_menu_panel->show();
	}
}

bool ClockGearsTitleScreen::_execute_title_step(const Dictionary &p_step) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String step_type = String(p_step.get("type", p_step.get("action", ""))).strip_edges().to_lower();
	if (step_type.is_empty()) {
		return false;
	}
	if (_execute_title_action(step_type)) {
		return true;
	}
	if (step_type == "set_switch" && dm) {
		String sw = String(p_step.get("switch_name", p_step.get("switch", p_step.get("name", "")))).strip_edges();
		if (!sw.is_empty()) {
			Variant raw_v = p_step.get("value", true);
			bool val = (raw_v.get_type() == Variant::STRING && String(raw_v).strip_edges().to_lower() == "toggle") ? !dm->get_switch(sw) : bool(raw_v);
			dm->set_switch(sw, val);
			return true;
		}
		return false;
	}
	if (step_type == "set_variable" && dm) {
		String vn = String(p_step.get("variable_name", "")).strip_edges();
		if (vn.is_empty()) {
			vn = String(p_step.get("var_name", p_step.get("variable", p_step.get("name", "")))).strip_edges();
		}
		if (!vn.is_empty()) {
			dm->set_variable(vn, int(p_step.get("value", 0)));
			return true;
		}
		return false;
	}
	if (step_type == "play_sound") {
		String sp = String(p_step.get("sound_path", p_step.get("sound", p_step.get("path", "")))).strip_edges();
		if (!sp.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_se(sp, 1.0f, float(p_step.get("pitch", 1.0f)));
			}
			return true;
		}
		return false;
	}
	if (step_type == "hud_visibility" && dm) {
		dm->set_hud_runtime_visible(bool(p_step.get("visible", true)));
		return true;
	}
	if (step_type == "load_game") {
		int slot = int(p_step.get("slot", 0));
		if (slot > 0) {
			return select_save_slot(slot);
		}
		open_continue_menu();
		return true;
	}
	if (step_type == "open_menu" || step_type == "open_system_screen" ||
			step_type == "items" || step_type == "equip" || step_type == "status" ||
			step_type == "save" || step_type == "save_game" || (dm && dm->has_menu(step_type))) {
		String target = String(p_step.get("target", p_step.get("menu_id", p_step.get("screen_id", p_step.get("screen", step_type))))).strip_edges();
		if (target == "save_game") target = "game_menu_save";
		ClockGearsGameMenu *sm = _ensure_custom_sub_menu();
		if (sm) {
			if (custom_ui_layer) custom_ui_layer->hide();
			return sm->open_menu_by_id(target, false, false, true);
		}
	}
	return false;
}

void ClockGearsTitleScreen::_on_custom_ui_action(const String &p_action, const String &p_elem_id) {
	Array steps;
	if (custom_ui_layer && !p_elem_id.is_empty()) {
		steps = custom_ui_layer->get_element_actions(p_elem_id);
	}
	if (!steps.is_empty()) {
		for (int i = 0; i < steps.size(); i++) {
			if (steps[i].get_type() == Variant::DICTIONARY) {
				_execute_title_step(steps[i]);
			}
		}
	} else {
		Dictionary single;
		single["type"] = p_action;
		_execute_title_step(single);
	}
	if (custom_ui_layer && custom_ui_layer->is_visible()) {
		custom_ui_layer->refresh_live_values();
	}
}

bool ClockGearsTitleScreen::_execute_title_action(const String &p_action) {
	String id = p_action.strip_edges().to_lower();
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
	if (id == "credits") {
		open_credits();
		return true;
	}
	if (id == "quit") {
		trigger_quit();
		return true;
	}
	return false;
}

void ClockGearsTitleScreen::_on_save_slot_pressed(int p_slot) {
	select_save_slot(p_slot);
}

bool ClockGearsTitleScreen::trigger_entry(const String &p_id) {
	String id = p_id.strip_edges().to_lower();
	if (custom_ui_active && custom_ui_layer) {
		if (custom_ui_layer->trigger_action(id)) {
			return true;
		}
	}
	if (!visible_entry_ids.has(id)) {
		return false;
	}
	return _execute_title_action(id);
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
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !preview_mode && dm->has_custom_ui_design("game_menu_save")) {
		if (main_menu_panel) main_menu_panel->hide();
		if (custom_ui_layer) custom_ui_layer->hide();
		if (ClockGearsGameMenu *sm = _ensure_custom_sub_menu()) {
			sm->open_menu_by_id("game_menu_save", false, false, true);
			return;
		}
	}
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
	if (custom_ui_layer) {
		custom_ui_layer->hide();
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
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !preview_mode && dm->has_custom_ui_design("game_menu_options")) {
		if (main_menu_panel) main_menu_panel->hide();
		if (custom_ui_layer) custom_ui_layer->hide();
		if (ClockGearsGameMenu *sm = _ensure_custom_sub_menu()) {
			sm->open_menu_by_id("game_menu_options", false, false, true);
			return;
		}
	}
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
	if (custom_ui_layer) {
		custom_ui_layer->hide();
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

void ClockGearsTitleScreen::open_credits() {
	_build_base_ui();
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
		main_menu_panel->hide();
	}
	if (custom_ui_layer) {
		custom_ui_layer->hide();
	}
	if (credits_player) {
		credits_player->start_credits();
	}
}

void ClockGearsTitleScreen::_on_title_credits_finished(bool p_cancelled) {
	if (custom_ui_active && custom_ui_layer) {
		custom_ui_layer->show();
		Vector<Button *> cbtns = custom_ui_layer->get_focus_ordered_buttons();
		if (!preview_mode && !cbtns.is_empty() && is_inside_tree()) {
			cbtns[0]->grab_focus();
		}
		return;
	}
	if (main_menu_panel) {
		main_menu_panel->show();
	}
	if (!preview_mode && entries_box && entries_box->get_child_count() > 0 && is_inside_tree()) {
		Button *focus_target = nullptr;
		for (int i = 0; i < entries_box->get_child_count(); i++) {
			Button *b = Object::cast_to<Button>(entries_box->get_child(i));
			if (b && b->has_meta("entry_id") && String(b->get_meta("entry_id")) == "credits") {
				focus_target = b;
				break;
			}
		}
		if (!focus_target) {
			focus_target = Object::cast_to<Button>(entries_box->get_child(0));
		}
		if (focus_target) {
			focus_target->grab_focus();
		}
	}
}

void ClockGearsTitleScreen::close_submenus() {
	if (custom_sub_menu && custom_sub_menu->is_menu_open()) {
		custom_sub_menu->close_menu();
		return;
	}
	if (credits_player && credits_player->is_playing()) {
		credits_player->stop_credits(true);
		return;
	}
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
	if (custom_ui_active && custom_ui_layer) {
		custom_ui_layer->show();
		if (!preview_mode && is_inside_tree()) {
			Vector<Button *> cbtns = custom_ui_layer->get_focus_ordered_buttons();
			if (!cbtns.is_empty()) {
				cbtns[0]->grab_focus();
			}
		}
		return;
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
	if (custom_sub_menu && custom_sub_menu->is_menu_open() && custom_sub_menu->get_active_menu_id() == "game_menu_save") {
		return true;
	}
	return continue_overlay && continue_overlay->is_visible() && continue_panel && continue_panel->is_visible();
}

bool ClockGearsTitleScreen::is_options_menu_open() const {
	if (custom_sub_menu && custom_sub_menu->is_menu_open() && custom_sub_menu->get_active_menu_id() == "game_menu_options") {
		return true;
	}
	return options_overlay && options_overlay->is_visible() && options_panel && options_panel->is_visible();
}

bool ClockGearsTitleScreen::is_credits_open() const {
	return credits_player && credits_player->is_playing();
}

void ClockGearsTitleScreen::apply_options_values(double p_music, double p_sfx, double p_brightness, bool p_fullscreen) {
	if (options_box) {
		options_box->set_persist_on_change(!editor_test_popup_mode);
		options_box->apply_values(p_music, p_sfx, p_brightness, p_fullscreen, this);
		return;
	}
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
		return;
	}
	dm->set_player_options(opts, true);
	dm->apply_player_options(this);
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
	if (custom_ui_active && custom_ui_layer && custom_ui_layer->is_visible()) {
		return custom_ui_layer->get_focus_ordered_buttons();
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
	if (is_credits_open() && credits_player) {
		return credits_player->handle_credits_key(p_keycode, true);
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
	if (p_keycode == Key::UP || (custom_ui_active && p_keycode == Key::LEFT)) {
		return _move_focus_by(-1);
	}
	if (p_keycode == Key::DOWN || (custom_ui_active && p_keycode == Key::RIGHT)) {
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
	out["custom_design_active"] = custom_ui_active;

	if (custom_ui_active && custom_ui_layer && !is_continue_menu_open() && !is_options_menu_open()) {
		Dictionary cinfo = custom_ui_layer->get_layout_bounds_info(vp_sz);
		out["stage_rect"] = cinfo.get("stage_rect", Rect2());
		out["element_rects"] = cinfo.get("element_rects", Dictionary());
		out["button_rects"] = cinfo.get("button_rects", Array());
		out["all_within_viewport"] = cinfo.get("all_within_viewport", true);
		out["any_overlap"] = cinfo.get("any_overlap", false);
		out["clips"] = cinfo.get("clips", false);
		return out;
	}

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
					Ref<StyleBox> gen_sb = b->get_theme_stylebox("normal");
					Ref<ClockGearsWindowStyleBox> cg_sb = gen_sb;
					if (cg_sb.is_valid()) {
						corner_rad = cg_sb->get_scaled_corner_radius();
					} else {
						Ref<StyleBoxFlat> flat_sb = gen_sb;
						if (flat_sb.is_valid()) {
							corner_rad = flat_sb->get_corner_radius(CORNER_TOP_LEFT);
						}
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
	} else if (p_what == NOTIFICATION_RESIZED) {
		_apply_theme_styling();
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
	ClassDB::bind_method(D_METHOD("open_credits"), &ClockGearsTitleScreen::open_credits);
	ClassDB::bind_method(D_METHOD("close_submenus"), &ClockGearsTitleScreen::close_submenus);
	ClassDB::bind_method(D_METHOD("is_continue_menu_open"), &ClockGearsTitleScreen::is_continue_menu_open);
	ClassDB::bind_method(D_METHOD("is_options_menu_open"), &ClockGearsTitleScreen::is_options_menu_open);
	ClassDB::bind_method(D_METHOD("is_credits_open"), &ClockGearsTitleScreen::is_credits_open);
	ClassDB::bind_method(D_METHOD("get_credits_player"), &ClockGearsTitleScreen::get_credits_player);
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
	ClassDB::bind_method(D_METHOD("is_custom_ui_active"), &ClockGearsTitleScreen::is_custom_ui_active);
	ClassDB::bind_method(D_METHOD("get_custom_ui_layer"), &ClockGearsTitleScreen::get_custom_ui_layer);

	ADD_SIGNAL(MethodInfo("new_game_started", PropertyInfo(Variant::STRING, "start_map")));
	ADD_SIGNAL(MethodInfo("save_loaded", PropertyInfo(Variant::INT, "slot"), PropertyInfo(Variant::STRING, "map_path")));
	ADD_SIGNAL(MethodInfo("quit_requested"));
}

ClockGearsTitleScreen::ClockGearsTitleScreen() {
	set_process_unhandled_input(true);
}
