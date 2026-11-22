#include "clockgears_preferences_dialog.h"
#include "core/clockgears_data_manager.h"
#include "core/config/project_settings.h"
#include "editor/clockgears_theme.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/panel_container.h"
#include "scene/main/window.h"

void ClockGearsPreferencesDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_preferences"), &ClockGearsPreferencesDialog::open_preferences);
	ClassDB::bind_method(D_METHOD("is_dim_inactive_layers_checked"), &ClockGearsPreferencesDialog::is_dim_inactive_layers_checked);
	ClassDB::bind_method(D_METHOD("set_dim_inactive_layers_checked", "enabled"), &ClockGearsPreferencesDialog::set_dim_inactive_layers_checked);
	ClassDB::bind_method(D_METHOD("get_inactive_layer_opacity_value"), &ClockGearsPreferencesDialog::get_inactive_layer_opacity_value);
	ClassDB::bind_method(D_METHOD("set_inactive_layer_opacity_value", "pct"), &ClockGearsPreferencesDialog::set_inactive_layer_opacity_value);
	ClassDB::bind_method(D_METHOD("get_selected_editor_theme_id"), &ClockGearsPreferencesDialog::get_selected_editor_theme_id);
	ClassDB::bind_method(D_METHOD("select_editor_theme_by_id", "theme_id"), &ClockGearsPreferencesDialog::select_editor_theme_by_id);
	ClassDB::bind_method(D_METHOD("get_editor_theme_option_button"), &ClockGearsPreferencesDialog::get_editor_theme_option_button);
	ClassDB::bind_method(D_METHOD("get_glow_effects_level_selected"), &ClockGearsPreferencesDialog::get_glow_effects_level_selected);
	ClassDB::bind_method(D_METHOD("select_glow_effects_level", "level"), &ClockGearsPreferencesDialog::select_glow_effects_level);
	ClassDB::bind_method(D_METHOD("get_glow_effects_option_button"), &ClockGearsPreferencesDialog::get_glow_effects_option_button);
	ClassDB::bind_method(D_METHOD("is_glow_effects_checked"), &ClockGearsPreferencesDialog::is_glow_effects_checked);
	ClassDB::bind_method(D_METHOD("set_glow_effects_checked", "enabled"), &ClockGearsPreferencesDialog::set_glow_effects_checked);
	ClassDB::bind_method(D_METHOD("get_long_help_delay_sec_value"), &ClockGearsPreferencesDialog::get_long_help_delay_sec_value);
	ClassDB::bind_method(D_METHOD("set_long_help_delay_sec_value", "seconds"), &ClockGearsPreferencesDialog::set_long_help_delay_sec_value);
	ClassDB::bind_method(D_METHOD("get_long_help_delay_spin"), &ClockGearsPreferencesDialog::get_long_help_delay_spin);
	ClassDB::bind_method(D_METHOD("apply_and_save_preferences"), &ClockGearsPreferencesDialog::apply_and_save_preferences);
}

void ClockGearsPreferencesDialog::_on_preset_selected(int p_idx) {
	switch (p_idx) {
		case 0: // 960 x 540
			width_spin->set_value(960);
			height_spin->set_value(540);
			break;
		case 1: // 1280 x 720
			width_spin->set_value(1280);
			height_spin->set_value(720);
			break;
		case 2: // 1920 x 1080
			width_spin->set_value(1920);
			height_spin->set_value(1080);
			break;
		case 3: // 816 x 624 (RPG Maker MZ)
			width_spin->set_value(816);
			height_spin->set_value(624);
			break;
		case 4: // 640 x 480 (Retro 4:3)
			width_spin->set_value(640);
			height_spin->set_value(480);
			break;
		case 5: // 320 x 240 (Pixel Art Clásico)
			width_spin->set_value(320);
			height_spin->set_value(240);
			break;
		default:
			break;
	}
}

void ClockGearsPreferencesDialog::_on_win_preset_selected(int p_idx) {
	switch (p_idx) {
		case 0: // Igual que interna
			win_width_spin->set_value(0);
			win_height_spin->set_value(0);
			break;
		case 1: // 960 x 540
			win_width_spin->set_value(960);
			win_height_spin->set_value(540);
			break;
		case 2: // 1280 x 720
			win_width_spin->set_value(1280);
			win_height_spin->set_value(720);
			break;
		case 3: // 1920 x 1080
			win_width_spin->set_value(1920);
			win_height_spin->set_value(1080);
			break;
		case 4: // 2560 x 1440
			win_width_spin->set_value(2560);
			win_height_spin->set_value(1440);
			break;
		case 5: // 3840 x 2160
			win_width_spin->set_value(3840);
			win_height_spin->set_value(2160);
			break;
		default:
			break;
	}
}

void ClockGearsPreferencesDialog::_load_settings() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (!ps) {
		return;
	}

	int w = ps->get_setting("display/window/size/viewport_width", 960);
	int h = ps->get_setting("display/window/size/viewport_height", 540);
	width_spin->set_value(w);
	height_spin->set_value(h);

	if (w == 960 && h == 540) preset_opt->select(0);
	else if (w == 1280 && h == 720) preset_opt->select(1);
	else if (w == 1920 && h == 1080) preset_opt->select(2);
	else if (w == 816 && h == 624) preset_opt->select(3);
	else if (w == 640 && h == 480) preset_opt->select(4);
	else if (w == 320 && h == 240) preset_opt->select(5);
	else preset_opt->select(6);

	int ww = ps->get_setting("display/window/size/window_width_override", 0);
	int wh = ps->get_setting("display/window/size/window_height_override", 0);
	if (win_width_spin) win_width_spin->set_value(ww);
	if (win_height_spin) win_height_spin->set_value(wh);

	if (win_preset_opt) {
		if (ww == 0 && wh == 0) win_preset_opt->select(0);
		else if (ww == 960 && wh == 540) win_preset_opt->select(1);
		else if (ww == 1280 && wh == 720) win_preset_opt->select(2);
		else if (ww == 1920 && wh == 1080) win_preset_opt->select(3);
		else if (ww == 2560 && wh == 1440) win_preset_opt->select(4);
		else if (ww == 3840 && wh == 2160) win_preset_opt->select(5);
		else win_preset_opt->select(6);
	}

	if (integer_scale_check) {
		String scale_mode = ps->get_setting("display/window/stretch/scale_mode", "integer");
		integer_scale_check->set_pressed(scale_mode == "integer");
	}

	if (fullscreen_check) {
		int mode_int = ps->get_setting("display/window/size/mode", 0);
		fullscreen_check->set_pressed(mode_int == 3 || mode_int == 4);
	}

	String mode = ps->get_setting("display/window/stretch/mode", "canvas_items");
	if (mode == "viewport") {
		stretch_mode_opt->select(0);
	} else if (mode == "canvas_items") {
		stretch_mode_opt->select(1);
	} else {
		stretch_mode_opt->select(2);
	}

	String aspect = ps->get_setting("display/window/stretch/aspect", "keep");
	if (aspect == "keep") {
		stretch_aspect_opt->select(0);
	} else if (aspect == "keep_width") {
		stretch_aspect_opt->select(1);
	} else if (aspect == "keep_height") {
		stretch_aspect_opt->select(2);
	} else {
		stretch_aspect_opt->select(3);
	}

	windows_path_edit->set_text(ps->get_setting("clockgears/export/windows_path", "builds/windows/Game.exe"));
	linux_path_edit->set_text(ps->get_setting("clockgears/export/linux_path", "builds/linux/Game.x86_64"));
	android_path_edit->set_text(ps->get_setting("clockgears/export/android_path", "builds/android/Game.apk"));
	build_type_opt->select(ps->get_setting("clockgears/export/build_type", 1));
	pck_check->set_pressed(ps->get_setting("clockgears/export/package_pck", true));

	bool dim_enabled = bool(ps->get_setting("clockgears/editor/dim_inactive_layers", true));
	int dim_pct = CLAMP(int(ps->get_setting("clockgears/editor/inactive_layer_opacity_pct", 40)), 10, 100);
	if (dim_inactive_layers_check) {
		dim_inactive_layers_check->set_pressed(dim_enabled);
	}
	if (inactive_layer_opacity_spin) {
		inactive_layer_opacity_spin->set_value(dim_pct);
		inactive_layer_opacity_spin->set_editable(dim_enabled);
	}

	String cur_theme = ClockGearsTheme::get_singleton() ? ClockGearsTheme::get_singleton()->get_active_theme_id() : String(ps->get_setting("clockgears/editor/theme_id", "dark"));
	if (editor_theme_opt) {
		int found_idx = 0;
		for (int i = 0; i < editor_theme_opt->get_item_count(); i++) {
			if (String(editor_theme_opt->get_item_metadata(i)) == cur_theme) {
				found_idx = i;
				break;
			}
		}
		editor_theme_opt->select(found_idx);
	}
	if (glow_effects_opt) {
		const int glow_lvl = ClockGearsTheme::get_glow_effects_level();
		int idx = glow_effects_opt->get_item_index(glow_lvl);
		if (idx < 0) {
			idx = glow_effects_opt->get_item_index(int(ClockGearsTheme::GLOW_SOFT));
		}
		glow_effects_opt->select(MAX(0, idx));
	}
	if (long_help_delay_spin) {
		long_help_delay_spin->set_value(ClockGearsTheme::get_long_help_delay_sec());
	}
}

void ClockGearsPreferencesDialog::_save_settings() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (!ps) {
		return;
	}

	ps->set_setting("display/window/size/viewport_width", int(width_spin->get_value()));
	ps->set_setting("display/window/size/viewport_height", int(height_spin->get_value()));

	if (win_width_spin) ps->set_setting("display/window/size/window_width_override", int(win_width_spin->get_value()));
	if (win_height_spin) ps->set_setting("display/window/size/window_height_override", int(win_height_spin->get_value()));

	if (integer_scale_check) {
		ps->set_setting("display/window/stretch/scale_mode", integer_scale_check->is_pressed() ? "integer" : "fractional");
	}
	if (fullscreen_check) {
		ps->set_setting("display/window/size/mode", fullscreen_check->is_pressed() ? 3 : 0);
	}

	static const char *stretch_modes[] = { "viewport", "canvas_items", "disabled" };
	int sm_idx = CLAMP(stretch_mode_opt->get_selected(), 0, 2);
	ps->set_setting("display/window/stretch/mode", stretch_modes[sm_idx]);

	static const char *stretch_aspects[] = { "keep", "keep_width", "keep_height", "expand" };
	int sa_idx = CLAMP(stretch_aspect_opt->get_selected(), 0, 3);
	ps->set_setting("display/window/stretch/aspect", stretch_aspects[sa_idx]);

	ps->set_setting("clockgears/export/windows_path", windows_path_edit->get_text());
	ps->set_setting("clockgears/export/linux_path", linux_path_edit->get_text());
	ps->set_setting("clockgears/export/android_path", android_path_edit->get_text());
	ps->set_setting("clockgears/export/build_type", build_type_opt->get_selected());
	ps->set_setting("clockgears/export/package_pck", pck_check->is_pressed());

	if (dim_inactive_layers_check) {
		ps->set_setting("clockgears/editor/dim_inactive_layers", dim_inactive_layers_check->is_pressed());
	}
	if (inactive_layer_opacity_spin) {
		ps->set_setting("clockgears/editor/inactive_layer_opacity_pct", CLAMP(int(inactive_layer_opacity_spin->get_value()), 10, 100));
	}
	if (editor_theme_opt) {
		String th_id = get_selected_editor_theme_id();
		ps->set_setting("clockgears/editor/theme_id", th_id);
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->set_active_theme_id(th_id);
		}
	}
	if (glow_effects_opt) {
		const int lvl = get_glow_effects_level_selected();
		ps->set_setting("clockgears/editor/glow_effects_level", lvl);
		ps->set_setting("clockgears/editor/glow_effects", lvl != int(ClockGearsTheme::GLOW_DISABLED));
		ClockGearsTheme::set_glow_effects_level(lvl);
	}
	if (long_help_delay_spin) {
		const int sec = CLAMP(int(Math::round(long_help_delay_spin->get_value())), 1, 6);
		ps->set_setting("clockgears/editor/long_help_delay_sec", sec);
		ClockGearsTheme::set_long_help_delay_sec(sec);
	}

	ps->save();
	if (ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager::get_singleton()->sync_editor_layer_dimming_from_settings();
	}
	print_line(vformat(CG_TR("[ClockGears Engine] Preferencias guardadas: %dx%d"), int(width_spin->get_value()), int(height_spin->get_value())));
}

void ClockGearsPreferencesDialog::_on_dim_inactive_toggled(bool p_pressed) {
	if (inactive_layer_opacity_spin) {
		inactive_layer_opacity_spin->set_editable(p_pressed);
	}
}

void ClockGearsPreferencesDialog::_on_editor_theme_selected(int p_idx) {
	if (!editor_theme_opt || p_idx < 0 || p_idx >= editor_theme_opt->get_item_count()) {
		return;
	}
	String th_id = String(editor_theme_opt->get_item_metadata(p_idx));
	if (ClockGearsTheme::get_singleton()) {
		if (!ClockGearsTheme::get_singleton()->set_active_theme_id(th_id)) {
			// Si el tema falla al cargar, vuelve al actual ("dark") sin error
			for (int i = 0; i < editor_theme_opt->get_item_count(); i++) {
				if (String(editor_theme_opt->get_item_metadata(i)) == ClockGearsTheme::get_singleton()->get_active_theme_id()) {
					editor_theme_opt->select(i);
					break;
				}
			}
		}
	}
}

String ClockGearsPreferencesDialog::get_selected_editor_theme_id() const {
	if (!editor_theme_opt) {
		return "dark";
	}
	int sel = editor_theme_opt->get_selected();
	if (sel >= 0 && sel < editor_theme_opt->get_item_count()) {
		String id = String(editor_theme_opt->get_item_metadata(sel));
		if (!id.is_empty()) {
			return id;
		}
	}
	return "dark";
}

bool ClockGearsPreferencesDialog::select_editor_theme_by_id(const String &p_theme_id) {
	bool ok = true;
	if (ClockGearsTheme::get_singleton()) {
		ok = ClockGearsTheme::get_singleton()->set_active_theme_id(p_theme_id);
	}
	String effective_id = ClockGearsTheme::get_singleton() ? ClockGearsTheme::get_singleton()->get_active_theme_id() : (ok ? p_theme_id : String("dark"));
	if (editor_theme_opt) {
		for (int i = 0; i < editor_theme_opt->get_item_count(); i++) {
			if (String(editor_theme_opt->get_item_metadata(i)) == effective_id) {
				editor_theme_opt->select(i);
				break;
			}
		}
	}
	return ok;
}

bool ClockGearsPreferencesDialog::is_dim_inactive_layers_checked() const {
	return dim_inactive_layers_check ? dim_inactive_layers_check->is_pressed() : true;
}

void ClockGearsPreferencesDialog::set_dim_inactive_layers_checked(bool p_enabled) {
	if (dim_inactive_layers_check) {
		dim_inactive_layers_check->set_pressed(p_enabled);
		_on_dim_inactive_toggled(p_enabled);
	}
}

int ClockGearsPreferencesDialog::get_inactive_layer_opacity_value() const {
	return inactive_layer_opacity_spin ? CLAMP(int(inactive_layer_opacity_spin->get_value()), 10, 100) : 40;
}

void ClockGearsPreferencesDialog::set_inactive_layer_opacity_value(int p_pct) {
	if (inactive_layer_opacity_spin) {
		inactive_layer_opacity_spin->set_value(CLAMP(p_pct, 10, 100));
	}
}

void ClockGearsPreferencesDialog::_on_glow_effects_selected(int p_idx) {
	if (!glow_effects_opt || p_idx < 0 || p_idx >= glow_effects_opt->get_item_count()) {
		return;
	}
	const int lvl = CLAMP(glow_effects_opt->get_item_id(p_idx), int(ClockGearsTheme::GLOW_DISABLED), int(ClockGearsTheme::GLOW_INTENSE));
	ClockGearsTheme::set_glow_effects_level(lvl);
}

int ClockGearsPreferencesDialog::get_glow_effects_level_selected() const {
	if (!glow_effects_opt) {
		return int(ClockGearsTheme::GLOW_SOFT);
	}
	const int sel = glow_effects_opt->get_selected();
	if (sel >= 0 && sel < glow_effects_opt->get_item_count()) {
		return CLAMP(glow_effects_opt->get_item_id(sel), int(ClockGearsTheme::GLOW_DISABLED), int(ClockGearsTheme::GLOW_INTENSE));
	}
	return int(ClockGearsTheme::GLOW_SOFT);
}

bool ClockGearsPreferencesDialog::select_glow_effects_level(int p_level) {
	const int lvl = CLAMP(p_level, int(ClockGearsTheme::GLOW_DISABLED), int(ClockGearsTheme::GLOW_INTENSE));
	if (glow_effects_opt) {
		int idx = glow_effects_opt->get_item_index(lvl);
		if (idx >= 0) {
			glow_effects_opt->select(idx);
		}
	}
	ClockGearsTheme::set_glow_effects_level(lvl);
	return true;
}

bool ClockGearsPreferencesDialog::is_glow_effects_checked() const {
	return get_glow_effects_level_selected() != int(ClockGearsTheme::GLOW_DISABLED);
}

void ClockGearsPreferencesDialog::set_glow_effects_checked(bool p_enabled) {
	select_glow_effects_level(p_enabled ? int(ClockGearsTheme::GLOW_SOFT) : int(ClockGearsTheme::GLOW_DISABLED));
}

void ClockGearsPreferencesDialog::_on_long_help_delay_changed(double p_val) {
	ClockGearsTheme::set_long_help_delay_sec(CLAMP(int(Math::round(p_val)), 1, 6));
}

int ClockGearsPreferencesDialog::get_long_help_delay_sec_value() const {
	if (long_help_delay_spin) {
		return CLAMP(int(Math::round(long_help_delay_spin->get_value())), 1, 6);
	}
	return ClockGearsTheme::get_long_help_delay_sec();
}

void ClockGearsPreferencesDialog::set_long_help_delay_sec_value(int p_sec) {
	const int clamped = CLAMP(p_sec, 1, 6);
	if (long_help_delay_spin) {
		long_help_delay_spin->set_value(clamped);
	}
	ClockGearsTheme::set_long_help_delay_sec(clamped);
}

void ClockGearsPreferencesDialog::_refresh_pref_theme_accents() {
	const bool is_brass = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
	const bool is_light = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "light");
	const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, EDSCALE);
	Vector<Node *> stack;
	stack.push_back(this);
	while (!stack.is_empty()) {
		Node *n = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);
		for (int i = 0; i < n->get_child_count(); i++) {
			stack.push_back(n->get_child(i));
		}
		if (Label *lb = Object::cast_to<Label>(n)) {
			if (lb->has_meta("_cg_pref_section_title")) {
				lb->add_theme_color_override("font_color", tk.accent_neon);
			}
		}
	}
}

void ClockGearsPreferencesDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(this);
		}
		_refresh_pref_theme_accents();
	} else if (p_what == NOTIFICATION_THEME_CHANGED) {
		_refresh_pref_theme_accents();
	}
}

void ClockGearsPreferencesDialog::apply_and_save_preferences() {
	_save_settings();
}

void ClockGearsPreferencesDialog::_on_dialog_confirmed() {
	_save_settings();
}

void ClockGearsPreferencesDialog::_on_export_now_pressed() {
	_save_settings();
	hide();
	print_line(CG_TR("[ClockGears Engine] Iniciando exportación simplificada según preferencias..."));
}

void ClockGearsPreferencesDialog::open_preferences() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	_load_settings();
	Size2 target_sz = Size2(640, 500) * EDSCALE;
	if (Window *pw = (get_parent() ? get_parent()->get_window() : nullptr)) {
		Size2i wsz = pw->get_size();
		if (wsz.x > 0 && wsz.y > 0) {
			target_sz.x = MIN(target_sz.x, float(wsz.x) - 32.0f);
			target_sz.y = MIN(target_sz.y, float(wsz.y) - 32.0f);
		}
	}
	popup_centered(target_sz);
}

ClockGearsPreferencesDialog::ClockGearsPreferencesDialog() {
	set_title(CG_TR("Preferencias del Proyecto - ClockGears Engine"));
	set_min_size(Size2(520, 380) * EDSCALE);
	set_ok_button_text(CG_TR("Guardar y Aplicar"));
	if (get_ok_button()) {
		get_ok_button()->set_theme_type_variation("ClockGearsPrimaryButton");
	}

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 8);
	add_child(main_vb);

	tabs = memnew(TabContainer);
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_vb->add_child(tabs);

	// =============================================================
	// Pestaña 1: Resolución
	// =============================================================
	ScrollContainer *res_scroll = memnew(ScrollContainer);
	res_scroll->set_name(CG_TR("Resolución y Pantalla"));
	res_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	res_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	tabs->add_child(res_scroll);

	VBoxContainer *res_tab = memnew(VBoxContainer);
	res_tab->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	res_tab->add_theme_constant_override("separation", 10);
	res_scroll->add_child(res_tab);

	// Tarjeta destacada arriba del todo (visible sin hacer scroll): Tema visual del editor
	PanelContainer *theme_card = memnew(PanelContainer);
	theme_card->set_theme_type_variation("ClockGearsContainer");
	res_tab->add_child(theme_card);

	VBoxContainer *theme_box = memnew(VBoxContainer);
	theme_box->add_theme_constant_override("separation", 6);
	theme_card->add_child(theme_box);

	Label *theme_title = memnew(Label(CG_TR("Apariencia y Tema del Editor:")));
	theme_title->set_meta("_cg_pref_section_title", true);
	theme_box->add_child(theme_title);

	HBoxContainer *theme_hb = memnew(HBoxContainer);
	theme_hb->add_theme_constant_override("separation", 8);
	theme_hb->add_child(memnew(Label(CG_TR("Tema Visual (Cambio en caliente):"))));
	editor_theme_opt = memnew(OptionButton);
	editor_theme_opt->add_item(CG_TR("ClockGears Oscuro (Por defecto)"));
	editor_theme_opt->set_item_metadata(0, "dark");
	editor_theme_opt->add_item(CG_TR("ClockGears Claro"));
	editor_theme_opt->set_item_metadata(1, "light");
	editor_theme_opt->add_item(CG_TR("ClockGears Latón"));
	editor_theme_opt->set_item_metadata(2, "brass");
	editor_theme_opt->select(0);
	editor_theme_opt->set_tooltip_text(CG_TR("Selecciona el tema del editor (Oscuro, Claro o ClockGears Latón) y aplícalo al instante sin reiniciar."));
	editor_theme_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	editor_theme_opt->connect(SceneStringName(item_selected), callable_mp(this, &ClockGearsPreferencesDialog::_on_editor_theme_selected));
	theme_hb->add_child(editor_theme_opt);
	theme_box->add_child(theme_hb);

	HBoxContainer *glow_hb = memnew(HBoxContainer);
	glow_hb->add_theme_constant_override("separation", 8);
	glow_hb->add_child(memnew(Label(CG_TR("Efectos de brillo (Al instante):"))));
	glow_effects_opt = memnew(OptionButton);
	glow_effects_opt->add_item(CG_TR("Desactivado (Bordes cobrizos sin halo)"), int(ClockGearsTheme::GLOW_DISABLED));
	glow_effects_opt->add_item(CG_TR("Suave (Por defecto)"), int(ClockGearsTheme::GLOW_SOFT));
	glow_effects_opt->add_item(CG_TR("Intenso (Halo más marcado en el elemento activo)"), int(ClockGearsTheme::GLOW_INTENSE));
	glow_effects_opt->select(1);
	glow_effects_opt->set_tooltip_text(CG_TR("Ajusta al instante el halo cobrizo y la intensidad del brillo en la interfaz sin reiniciar."));
	glow_effects_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	glow_effects_opt->connect(SceneStringName(item_selected), callable_mp(this, &ClockGearsPreferencesDialog::_on_glow_effects_selected));
	glow_hb->add_child(glow_effects_opt);
	theme_box->add_child(glow_hb);

	HBoxContainer *help_delay_hb = memnew(HBoxContainer);
	help_delay_hb->add_theme_constant_override("separation", 8);
	help_delay_hb->add_child(memnew(Label(String::utf8("Retardo de las ayudas largas:"))));
	long_help_delay_spin = memnew(SpinBox);
	long_help_delay_spin->set_min(1);
	long_help_delay_spin->set_max(6);
	long_help_delay_spin->set_step(1);
	long_help_delay_spin->set_value(ClockGearsTheme::get_long_help_delay_sec());
	long_help_delay_spin->set_suffix(" s");
	long_help_delay_spin->set_custom_minimum_size(Size2(110 * EDSCALE, 0));
	long_help_delay_spin->set_tooltip_text(String::utf8("Segundos con el cursor quieto antes de mostrar la explicación larga (1 a 6 s). Se aplica al instante."));
	long_help_delay_spin->connect(SceneStringName(value_changed), callable_mp(this, &ClockGearsPreferencesDialog::_on_long_help_delay_changed));
	help_delay_hb->add_child(long_help_delay_spin);
	theme_box->add_child(help_delay_hb);

	PanelContainer *res_card = memnew(PanelContainer);
	res_card->set_theme_type_variation("ClockGearsContainer");
	res_tab->add_child(res_card);

	VBoxContainer *res_box = memnew(VBoxContainer);
	res_box->add_theme_constant_override("separation", 8);
	res_card->add_child(res_box);

	Label *res_title = memnew(Label(CG_TR("Resolución Interna del Juego (Viewport):")));
	res_title->set_meta("_cg_pref_section_title", true);
	res_box->add_child(res_title);

	HBoxContainer *preset_hb = memnew(HBoxContainer);
	preset_hb->add_child(memnew(Label(CG_TR("Plantillas Predefinidas:"))));
	preset_opt = memnew(OptionButton);
	preset_opt->add_item(CG_TR("960 x 540 (nHD 16:9 - Pixel Art Nativo)"));
	preset_opt->add_item(CG_TR("1280 x 720 (HD Estándar 16:9)"));
	preset_opt->add_item(CG_TR("1920 x 1080 (Full HD 16:9)"));
	preset_opt->add_item(CG_TR("816 x 624 (RPG Maker MZ Nativo)"));
	preset_opt->add_item(CG_TR("640 x 480 (Retro 4:3)"));
	preset_opt->add_item(CG_TR("320 x 240 (Pixel Art Clásico)"));
	preset_opt->add_item(CG_TR("Personalizado"));
	preset_opt->select(0);
	preset_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	preset_opt->connect("item_selected", callable_mp(this, &ClockGearsPreferencesDialog::_on_preset_selected));
	preset_hb->add_child(preset_opt);
	res_box->add_child(preset_hb);

	HBoxContainer *dim_hb = memnew(HBoxContainer);
	dim_hb->add_child(memnew(Label(CG_TR("Ancho (Interno):"))));
	width_spin = memnew(SpinBox);
	width_spin->set_min(160);
	width_spin->set_max(7680);
	width_spin->set_value(960);
	width_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dim_hb->add_child(width_spin);

	dim_hb->add_child(memnew(Label(CG_TR("Alto (Interno):"))));
	height_spin = memnew(SpinBox);
	height_spin->set_min(120);
	height_spin->set_max(4320);
	height_spin->set_value(540);
	height_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dim_hb->add_child(height_spin);
	res_box->add_child(dim_hb);

	res_box->add_child(memnew(HSeparator));

	Label *win_title = memnew(Label(CG_TR("Tamaño de Ventana (Window Size Override):")));
	win_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	res_box->add_child(win_title);

	HBoxContainer *win_preset_hb = memnew(HBoxContainer);
	win_preset_hb->add_child(memnew(Label(CG_TR("Plantilla de Ventana:"))));
	win_preset_opt = memnew(OptionButton);
	win_preset_opt->add_item(CG_TR("Igual que interna (1x)"));
	win_preset_opt->add_item(CG_TR("960 x 540 (1x nHD)"));
	win_preset_opt->add_item(CG_TR("1280 x 720 (HD 16:9)"));
	win_preset_opt->add_item(CG_TR("1920 x 1080 (Full HD 16:9)"));
	win_preset_opt->add_item(CG_TR("2560 x 1440 (2K QHD 16:9)"));
	win_preset_opt->add_item(CG_TR("3840 x 2160 (4K UHD 16:9)"));
	win_preset_opt->add_item(CG_TR("Personalizado"));
	win_preset_opt->select(0);
	win_preset_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	win_preset_opt->connect("item_selected", callable_mp(this, &ClockGearsPreferencesDialog::_on_win_preset_selected));
	win_preset_hb->add_child(win_preset_opt);
	res_box->add_child(win_preset_hb);

	HBoxContainer *win_dim_hb = memnew(HBoxContainer);
	win_dim_hb->add_child(memnew(Label(CG_TR("Ancho Ventana (0=Auto):"))));
	win_width_spin = memnew(SpinBox);
	win_width_spin->set_min(0);
	win_width_spin->set_max(7680);
	win_width_spin->set_value(0);
	win_width_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	win_dim_hb->add_child(win_width_spin);

	win_dim_hb->add_child(memnew(Label(CG_TR("Alto Ventana (0=Auto):"))));
	win_height_spin = memnew(SpinBox);
	win_height_spin->set_min(0);
	win_height_spin->set_max(4320);
	win_height_spin->set_value(0);
	win_height_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	win_dim_hb->add_child(win_height_spin);
	res_box->add_child(win_dim_hb);

	res_box->add_child(memnew(HSeparator));

	Label *mode_title = memnew(Label(CG_TR("Escalado, Proporción y Pantalla:")));
	mode_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	res_box->add_child(mode_title);

	VBoxContainer *checks_hb = memnew(VBoxContainer);
	checks_hb->add_theme_constant_override("separation", 4);
	integer_scale_check = memnew(CheckBox(CG_TR("Escala Entera (Integer Scaling - Pixel Perfect)")));
	integer_scale_check->set_pressed(true);
	checks_hb->add_child(integer_scale_check);

	fullscreen_check = memnew(CheckBox(CG_TR("Pantalla Completa al Iniciar (Fullscreen)")));
	fullscreen_check->set_pressed(false);
	checks_hb->add_child(fullscreen_check);
	res_box->add_child(checks_hb);

	HBoxContainer *modes_hb = memnew(HBoxContainer);
	modes_hb->add_child(memnew(Label(CG_TR("Modo de Estiramiento:"))));
	stretch_mode_opt = memnew(OptionButton);
	stretch_mode_opt->add_item(CG_TR("Viewport (Pixel Art nítido)"));
	stretch_mode_opt->add_item(CG_TR("Canvas Items (Suavizado HD)"));
	stretch_mode_opt->add_item(CG_TR("Desactivado (1:1 fijo)"));
	stretch_mode_opt->select(0);
	stretch_mode_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	modes_hb->add_child(stretch_mode_opt);

	modes_hb->add_child(memnew(Label(CG_TR("Proporción:"))));
	stretch_aspect_opt = memnew(OptionButton);
	stretch_aspect_opt->add_item(CG_TR("Mantener (Keep Ratio)"));
	stretch_aspect_opt->add_item(CG_TR("Mantener Ancho"));
	stretch_aspect_opt->add_item(CG_TR("Mantener Alto"));
	stretch_aspect_opt->add_item(CG_TR("Expandir a Pantalla"));
	stretch_aspect_opt->select(0);
	stretch_aspect_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	modes_hb->add_child(stretch_aspect_opt);
	res_box->add_child(modes_hb);

	// Tarjeta de edición de capas en el editor
	PanelContainer *layer_edit_card = memnew(PanelContainer);
	layer_edit_card->set_theme_type_variation("ClockGearsContainer");
	res_tab->add_child(layer_edit_card);

	VBoxContainer *layer_edit_box = memnew(VBoxContainer);
	layer_edit_box->add_theme_constant_override("separation", 8);
	layer_edit_card->add_child(layer_edit_box);

	Label *le_title = memnew(Label(CG_TR("Edición de Capas en el Mapa (Solo Editor):")));
	le_title->set_meta("_cg_pref_section_title", true);
	layer_edit_box->add_child(le_title);

	dim_inactive_layers_check = memnew(CheckBox(CG_TR("Atenuar capas inactivas al editar (capa actual, eventos y jugador quedan al 100 %)")));
	dim_inactive_layers_check->set_pressed(true);
	dim_inactive_layers_check->connect("toggled", callable_mp(this, &ClockGearsPreferencesDialog::_on_dim_inactive_toggled));
	layer_edit_box->add_child(dim_inactive_layers_check);

	HBoxContainer *dim_pct_hb = memnew(HBoxContainer);
	dim_pct_hb->add_theme_constant_override("separation", 8);
	dim_pct_hb->add_child(memnew(Label(CG_TR("Opacidad de las demás capas (10 a 100 %):"))));
	inactive_layer_opacity_spin = memnew(SpinBox);
	inactive_layer_opacity_spin->set_min(10);
	inactive_layer_opacity_spin->set_max(100);
	inactive_layer_opacity_spin->set_step(5);
	inactive_layer_opacity_spin->set_value(40);
	inactive_layer_opacity_spin->set_suffix(" %");
	inactive_layer_opacity_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dim_pct_hb->add_child(inactive_layer_opacity_spin);
	layer_edit_box->add_child(dim_pct_hb);

	// =============================================================
	// Pestaña 2: Exportación Simplificada
	// =============================================================
	ScrollContainer *exp_scroll = memnew(ScrollContainer);
	exp_scroll->set_name(CG_TR("Exportación Simplificada"));
	exp_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	exp_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	tabs->add_child(exp_scroll);

	VBoxContainer *exp_tab = memnew(VBoxContainer);
	exp_tab->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	exp_tab->add_theme_constant_override("separation", 10);
	exp_scroll->add_child(exp_tab);

	PanelContainer *exp_card = memnew(PanelContainer);
	exp_card->set_theme_type_variation("ClockGearsContainer");
	exp_tab->add_child(exp_card);

	VBoxContainer *exp_box = memnew(VBoxContainer);
	exp_box->add_theme_constant_override("separation", 8);
	exp_card->add_child(exp_box);

	Label *exp_title = memnew(Label(CG_TR("Rutas de Salida para Compilación:")));
	exp_title->set_meta("_cg_pref_section_title", true);
	exp_box->add_child(exp_title);

	HBoxContainer *win_hb = memnew(HBoxContainer);
	win_hb->add_child(memnew(Label(CG_TR("Windows (.exe):"))));
	windows_path_edit = memnew(LineEdit);
	windows_path_edit->set_text("builds/windows/Game.exe");
	windows_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	win_hb->add_child(windows_path_edit);
	exp_box->add_child(win_hb);

	HBoxContainer *lin_hb = memnew(HBoxContainer);
	lin_hb->add_child(memnew(Label(CG_TR("Linux (.x86_64):"))));
	linux_path_edit = memnew(LineEdit);
	linux_path_edit->set_text("builds/linux/Game.x86_64");
	linux_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lin_hb->add_child(linux_path_edit);
	exp_box->add_child(lin_hb);

	HBoxContainer *and_hb = memnew(HBoxContainer);
	and_hb->add_child(memnew(Label(CG_TR("Android (.apk):"))));
	android_path_edit = memnew(LineEdit);
	android_path_edit->set_text("builds/android/Game.apk");
	android_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	and_hb->add_child(android_path_edit);
	exp_box->add_child(and_hb);

	HSeparator *exp_sep = memnew(HSeparator);
	exp_sep->set_theme_type_variation("ClockGearsSectionHeaderLine");
	exp_box->add_child(exp_sep);

	HBoxContainer *build_hb = memnew(HBoxContainer);
	build_hb->add_child(memnew(Label(CG_TR("Tipo de Compilación:"))));
	build_type_opt = memnew(OptionButton);
	build_type_opt->add_item(CG_TR("Depuración (Debug)"));
	build_type_opt->add_item(CG_TR("Lanzamiento Final (Release Optimizada)"));
	build_type_opt->select(1);
	build_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	build_hb->add_child(build_type_opt);
	exp_box->add_child(build_hb);

	pck_check = memnew(CheckBox(CG_TR("Empaquetar todos los recursos en un único archivo de datos (.pck)")));
	pck_check->set_pressed(true);
	exp_box->add_child(pck_check);

	export_now_btn = memnew(Button(CG_TR("Exportar Juego Ahora (1 Clic)")));
	export_now_btn->set_custom_minimum_size(Size2(0, 36) * EDSCALE);
	export_now_btn->connect("pressed", callable_mp(this, &ClockGearsPreferencesDialog::_on_export_now_pressed));
	exp_tab->add_child(export_now_btn);

	connect("confirmed", callable_mp(this, &ClockGearsPreferencesDialog::_on_dialog_confirmed));
}
