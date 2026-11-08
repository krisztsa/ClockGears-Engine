#include "clockgears_preferences_dialog.h"
#include "core/config/project_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/panel_container.h"
#include "scene/resources/style_box_flat.h"

static Ref<StyleBoxFlat> _pref_card_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.11, 0.12, 0.14));
	style->set_corner_radius_all(6);
	style->set_content_margin_all(10);
	style->set_border_width_all(1);
	style->set_border_color(Color(1, 1, 1, 0.08));
	return style;
}

void ClockGearsPreferencesDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_preferences"), &ClockGearsPreferencesDialog::open_preferences);
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

	ps->save();
	print_line(vformat(CG_TR("[ClockGears Engine] Preferencias guardadas: %dx%d"), int(width_spin->get_value()), int(height_spin->get_value())));
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
	_load_settings();
	popup_centered(Size2(640, 480) * EDSCALE);
}

ClockGearsPreferencesDialog::ClockGearsPreferencesDialog() {
	set_title(CG_TR("Preferencias del Proyecto - ClockGears Engine"));
	set_min_size(Size2(620, 440) * EDSCALE);
	set_ok_button_text(CG_TR("Guardar y Aplicar"));

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
	VBoxContainer *res_tab = memnew(VBoxContainer);
	res_tab->set_name(CG_TR("Resolución y Pantalla"));
	res_tab->add_theme_constant_override("separation", 10);
	tabs->add_child(res_tab);

	PanelContainer *res_card = memnew(PanelContainer);
	res_card->add_theme_style_override("panel", _pref_card_style());
	res_tab->add_child(res_card);

	VBoxContainer *res_box = memnew(VBoxContainer);
	res_box->add_theme_constant_override("separation", 8);
	res_card->add_child(res_box);

	Label *res_title = memnew(Label(CG_TR("Resolución Interna del Juego (Viewport):")));
	res_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
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

	HBoxContainer *checks_hb = memnew(HBoxContainer);
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

	// =============================================================
	// Pestaña 2: Exportación Simplificada
	// =============================================================
	VBoxContainer *exp_tab = memnew(VBoxContainer);
	exp_tab->set_name(CG_TR("Exportación Simplificada"));
	exp_tab->add_theme_constant_override("separation", 10);
	tabs->add_child(exp_tab);

	PanelContainer *exp_card = memnew(PanelContainer);
	exp_card->add_theme_style_override("panel", _pref_card_style());
	exp_tab->add_child(exp_card);

	VBoxContainer *exp_box = memnew(VBoxContainer);
	exp_box->add_theme_constant_override("separation", 8);
	exp_card->add_child(exp_box);

	Label *exp_title = memnew(Label(CG_TR("Rutas de Salida para Compilación:")));
	exp_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
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

	exp_box->add_child(memnew(HSeparator));

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
