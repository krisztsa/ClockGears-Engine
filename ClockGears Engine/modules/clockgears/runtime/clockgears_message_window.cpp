#include "clockgears_message_window.h"
#include "../core/clockgears_data_manager.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "scene/gui/separator.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/viewport.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/image_texture.h"

#define CG_MW_TR(text) String::utf8(text)

ClockGearsMessageWindow *ClockGearsMessageWindow::singleton = nullptr;

void ClockGearsMessageWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_message", "text", "speaker", "face_path", "pos", "bg"), &ClockGearsMessageWindow::show_message, DEFVAL(""), DEFVAL(""), DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("show_choices", "choices", "cancel_type", "default_idx"), &ClockGearsMessageWindow::show_choices, DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("show_number_input", "variable_id", "digits"), &ClockGearsMessageWindow::show_number_input, DEFVAL(3));
	ClassDB::bind_method(D_METHOD("advance_or_close"), &ClockGearsMessageWindow::advance_or_close);
	ClassDB::bind_method(D_METHOD("close_window"), &ClockGearsMessageWindow::close_window);
	ClassDB::bind_method(D_METHOD("set_face", "path", "frame", "expression"), &ClockGearsMessageWindow::set_face, DEFVAL(0), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_current_expression"), &ClockGearsMessageWindow::get_current_expression);
	ClassDB::bind_method(D_METHOD("get_face_frame"), &ClockGearsMessageWindow::get_face_frame);
	ClassDB::bind_method(D_METHOD("get_speaker_name"), &ClockGearsMessageWindow::get_speaker_name);
	ClassDB::bind_method(D_METHOD("open_history"), &ClockGearsMessageWindow::open_history);
	ClassDB::bind_method(D_METHOD("close_history"), &ClockGearsMessageWindow::close_history);
	ClassDB::bind_method(D_METHOD("is_history_open"), &ClockGearsMessageWindow::is_history_open);
	ClassDB::bind_method(D_METHOD("add_history_entry", "speaker", "text"), &ClockGearsMessageWindow::add_history_entry);
	ClassDB::bind_method(D_METHOD("get_history_count"), &ClockGearsMessageWindow::get_history_count);
	ClassDB::bind_method(D_METHOD("set_auto_mode", "enable"), &ClockGearsMessageWindow::set_auto_mode);
	ClassDB::bind_method(D_METHOD("get_auto_mode"), &ClockGearsMessageWindow::get_auto_mode);
	ClassDB::bind_method(D_METHOD("set_skip_mode", "enable"), &ClockGearsMessageWindow::set_skip_mode);
	ClassDB::bind_method(D_METHOD("get_skip_mode"), &ClockGearsMessageWindow::get_skip_mode);
	ClassDB::bind_method(D_METHOD("is_busy"), &ClockGearsMessageWindow::is_busy);

	ADD_SIGNAL(MethodInfo("message_completed"));
	ADD_SIGNAL(MethodInfo("choice_selected", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("number_entered", PropertyInfo(Variant::INT, "value")));
}

ClockGearsMessageWindow::ClockGearsMessageWindow() {
	if (!singleton) {
		singleton = this;
	}
	set_process(true);
	set_process_unhandled_input(true);
	set_focus_mode(FOCUS_NONE);
	set_mouse_filter(MOUSE_FILTER_PASS);

	// 1. Estilos Pixel Art
	style_window = memnew(StyleBoxFlat);
	style_window->set_anti_aliased(false);
	style_window->set_bg_color(Color(0.08f, 0.09f, 0.12f, 0.94f));
	style_window->set_border_width_all(2);
	style_window->set_border_color(Color(0.88f, 0.63f, 0.25f, 1.0f)); // Cobre (#e0a040)
	style_window->set_corner_radius_all(4);
	style_window->set_content_margin_all(10);

	style_dim = memnew(StyleBoxFlat);
	style_dim->set_anti_aliased(false);
	style_dim->set_bg_color(Color(0.02f, 0.03f, 0.05f, 0.75f));
	style_dim->set_border_width_all(0);
	style_dim->set_corner_radius_all(2);
	style_dim->set_content_margin_all(10);

	style_transparent = memnew(StyleBoxFlat);
	style_transparent->set_bg_color(Color(0, 0, 0, 0));
	style_transparent->set_border_width_all(0);
	style_transparent->set_content_margin_all(10);

	// 2. Ventana Principal de Mensaje
	main_panel = memnew(PanelContainer);
	main_panel->add_theme_style_override("panel", style_window);
	main_panel->set_focus_mode(FOCUS_NONE);
	main_panel->set_mouse_filter(MOUSE_FILTER_PASS);
	add_child(main_panel);

	// 3. Caja de Nombre (Name Box)
	name_panel = memnew(PanelContainer);
	Ref<StyleBoxFlat> name_sb = memnew(StyleBoxFlat);
	name_sb->set_anti_aliased(false);
	name_sb->set_bg_color(Color(0.12f, 0.13f, 0.17f, 0.95f));
	name_sb->set_border_width_all(2);
	name_sb->set_border_color(Color(0.88f, 0.63f, 0.25f, 1.0f));
	name_sb->set_corner_radius_all(3);
	name_sb->set_content_margin(SIDE_LEFT, 10);
	name_sb->set_content_margin(SIDE_RIGHT, 10);
	name_sb->set_content_margin(SIDE_TOP, 3);
	name_sb->set_content_margin(SIDE_BOTTOM, 3);
	name_panel->add_theme_style_override("panel", name_sb);
	name_panel->set_focus_mode(FOCUS_NONE);
	name_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	name_panel->hide();
	add_child(name_panel);

	name_label = memnew(Label);
	name_label->add_theme_font_size_override("font_size", 14);
	name_label->add_theme_color_override("font_color", Color(0.95f, 0.78f, 0.35f, 1.0f));
	name_label->set_focus_mode(FOCUS_NONE);
	name_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	name_panel->add_child(name_label);

	// 4. Contenedor horizontal: Retrato + Texto
	content_hbox = memnew(HBoxContainer);
	content_hbox->add_theme_constant_override("separation", 12);
	content_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	content_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	content_hbox->set_focus_mode(FOCUS_NONE);
	content_hbox->set_mouse_filter(MOUSE_FILTER_PASS);
	main_panel->add_child(content_hbox);

	face_frame = memnew(PanelContainer);
	Ref<StyleBoxFlat> ff_sb = memnew(StyleBoxFlat);
	ff_sb->set_anti_aliased(false);
	ff_sb->set_bg_color(Color(0.04f, 0.05f, 0.07f, 0.8f));
	ff_sb->set_border_width_all(1);
	ff_sb->set_border_color(Color(0.4f, 0.45f, 0.55f, 0.6f));
	ff_sb->set_content_margin_all(4);
	face_frame->add_theme_style_override("panel", ff_sb);
	face_frame->set_custom_minimum_size(Size2(88, 88));
	face_frame->set_focus_mode(FOCUS_NONE);
	face_frame->set_mouse_filter(MOUSE_FILTER_IGNORE);
	face_frame->hide();
	content_hbox->add_child(face_frame);

	face_rect = memnew(TextureRect);
	face_rect->set_custom_minimum_size(Size2(80, 80));
	face_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	face_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	face_rect->set_texture_filter(TEXTURE_FILTER_NEAREST);
	face_rect->set_focus_mode(FOCUS_NONE);
	face_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	face_frame->add_child(face_rect);

	text_vbox = memnew(VBoxContainer);
	text_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	text_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	text_vbox->set_focus_mode(FOCUS_NONE);
	text_vbox->set_mouse_filter(MOUSE_FILTER_PASS);
	content_hbox->add_child(text_vbox);

	rich_text_label = memnew(RichTextLabel);
	rich_text_label->set_use_bbcode(true);
	rich_text_label->set_h_size_flags(SIZE_EXPAND_FILL);
	rich_text_label->set_v_size_flags(SIZE_EXPAND_FILL);
	rich_text_label->set_scroll_active(false);
	rich_text_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	rich_text_label->set_texture_filter(TEXTURE_FILTER_NEAREST);
	rich_text_label->add_theme_font_size_override("normal_font_size", 16);
	rich_text_label->add_theme_font_size_override("bold_font_size", 16);
	rich_text_label->add_theme_font_size_override("italics_font_size", 16);
	rich_text_label->add_theme_font_size_override("bold_italics_font_size", 16);
	rich_text_label->add_theme_color_override("default_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
	rich_text_label->set_focus_mode(FOCUS_NONE);
	rich_text_label->set_mouse_filter(MOUSE_FILTER_IGNORE); // No robar ui_accept ni eventos de ratón
	text_vbox->add_child(rich_text_label);

	// Fila inferior para AUTO/SKIP y Flecha de continuar
	HBoxContainer *bottom_bar = memnew(HBoxContainer);
	bottom_bar->set_focus_mode(FOCUS_NONE);
	bottom_bar->set_mouse_filter(MOUSE_FILTER_IGNORE);
	text_vbox->add_child(bottom_bar);

	// 5. Barra de Modos (AUTO / SKIP) informativos (sin botones para no interferir con la tecla de acción)
	mode_bar = memnew(HBoxContainer);
	mode_bar->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
	mode_bar->add_theme_constant_override("separation", 10);
	mode_bar->set_focus_mode(FOCUS_NONE);
	mode_bar->set_mouse_filter(MOUSE_FILTER_IGNORE);
	mode_bar->set_h_size_flags(SIZE_EXPAND_FILL);
	bottom_bar->add_child(mode_bar);

	auto_indicator = memnew(Label("[Tab] AUTO"));
	auto_indicator->add_theme_font_size_override("font_size", 11);
	auto_indicator->set_focus_mode(FOCUS_NONE);
	auto_indicator->set_mouse_filter(MOUSE_FILTER_IGNORE);
	mode_bar->add_child(auto_indicator);

	skip_indicator = memnew(Label("[Ctrl] SKIP"));
	skip_indicator->add_theme_font_size_override("font_size", 11);
	skip_indicator->set_focus_mode(FOCUS_NONE);
	skip_indicator->set_mouse_filter(MOUSE_FILTER_IGNORE);
	mode_bar->add_child(skip_indicator);

	_update_mode_indicators();

	// Flecha de Continuar ▼
	continue_arrow = memnew(Label(CG_MW_TR("▼")));
	continue_arrow->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	continue_arrow->add_theme_font_size_override("font_size", 14);
	continue_arrow->add_theme_color_override("font_color", Color(0.88f, 0.63f, 0.25f, 0.95f));
	continue_arrow->set_focus_mode(FOCUS_NONE);
	continue_arrow->set_mouse_filter(MOUSE_FILTER_IGNORE);
	continue_arrow->hide();
	bottom_bar->add_child(continue_arrow);

	// Ventana flotante de Oro ($)
	gold_panel = memnew(PanelContainer);
	Ref<StyleBoxFlat> gold_sb = memnew(StyleBoxFlat);
	gold_sb->set_anti_aliased(false);
	gold_sb->set_bg_color(Color(0.08f, 0.09f, 0.12f, 0.95f));
	gold_sb->set_border_width_all(2);
	gold_sb->set_border_color(Color(0.88f, 0.63f, 0.25f, 1.0f));
	gold_sb->set_content_margin_all(8);
	gold_panel->add_theme_style_override("panel", gold_sb);
	gold_panel->set_focus_mode(FOCUS_NONE);
	gold_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	gold_panel->hide();
	add_child(gold_panel);

	gold_label = memnew(Label(CG_MW_TR("0 G")));
	gold_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	gold_label->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.35f, 1.0f));
	gold_label->set_focus_mode(FOCUS_NONE);
	gold_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	gold_panel->add_child(gold_label);

	// Ventana de Opciones (Choices)
	choices_panel = memnew(PanelContainer);
	choices_panel->add_theme_style_override("panel", style_window);
	choices_panel->set_focus_mode(FOCUS_NONE);
	choices_panel->set_mouse_filter(MOUSE_FILTER_PASS);
	choices_panel->hide();
	add_child(choices_panel);

	choices_vbox = memnew(VBoxContainer);
	choices_vbox->add_theme_constant_override("separation", 4);
	choices_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	choices_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	choices_vbox->set_focus_mode(FOCUS_NONE);
	choices_panel->add_child(choices_vbox);

	// Ventana de Entrada de Número (Number Input)
	number_panel = memnew(PanelContainer);
	number_panel->add_theme_style_override("panel", style_window);
	number_panel->set_focus_mode(FOCUS_NONE);
	number_panel->set_mouse_filter(MOUSE_FILTER_PASS);
	number_panel->hide();
	add_child(number_panel);

	number_hbox = memnew(HBoxContainer);
	number_hbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	number_hbox->add_theme_constant_override("separation", 8);
	number_hbox->set_focus_mode(FOCUS_NONE);
	number_panel->add_child(number_hbox);

	// Historial de Diálogos (Backlog)
	history_panel = memnew(PanelContainer);
	history_panel->add_theme_style_override("panel", style_window);
	history_panel->set_focus_mode(FOCUS_NONE);
	history_panel->hide();
	add_child(history_panel);

	VBoxContainer *hist_vb = memnew(VBoxContainer);
	hist_vb->add_theme_constant_override("separation", 8);
	hist_vb->set_focus_mode(FOCUS_NONE);
	history_panel->add_child(hist_vb);

	HBoxContainer *hist_hdr = memnew(HBoxContainer);
	hist_hdr->set_focus_mode(FOCUS_NONE);
	Label *hist_title = memnew(Label(CG_MW_TR("Historial de Conversación")));
	hist_title->add_theme_color_override("font_color", Color(0.95f, 0.8f, 0.3f));
	hist_title->set_h_size_flags(SIZE_EXPAND_FILL);
	hist_title->set_focus_mode(FOCUS_NONE);
	hist_hdr->add_child(hist_title);

	history_close_btn = memnew(Button(CG_MW_TR("Cerrar (Esc)")));
	history_close_btn->set_focus_mode(FOCUS_NONE); // NO robar foco de teclado
	history_close_btn->connect("pressed", callable_mp(this, &ClockGearsMessageWindow::close_history));
	hist_hdr->add_child(history_close_btn);
	hist_vb->add_child(hist_hdr);

	history_scroll = memnew(ScrollContainer);
	history_scroll->set_h_size_flags(SIZE_EXPAND_FILL);
	history_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	history_scroll->set_focus_mode(FOCUS_NONE);
	hist_vb->add_child(history_scroll);

	history_vbox = memnew(VBoxContainer);
	history_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	history_vbox->add_theme_constant_override("separation", 8);
	history_vbox->set_focus_mode(FOCUS_NONE);
	history_scroll->add_child(history_vbox);

	hide();
}

ClockGearsMessageWindow::~ClockGearsMessageWindow() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void ClockGearsMessageWindow::_update_mode_indicators() {
	if (auto_indicator) {
		if (is_auto_mode) {
			auto_indicator->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1.0f));
			auto_indicator->set_text("[Tab] AUTO *");
		} else {
			auto_indicator->add_theme_color_override("font_color", Color(0.5f, 0.55f, 0.65f, 0.5f));
			auto_indicator->set_text("[Tab] AUTO");
		}
	}
	if (skip_indicator) {
		if (is_skip_mode) {
			skip_indicator->add_theme_color_override("font_color", Color(0.2f, 0.9f, 1.0f, 1.0f));
			skip_indicator->set_text("[Ctrl] SKIP *");
		} else {
			skip_indicator->add_theme_color_override("font_color", Color(0.5f, 0.55f, 0.65f, 0.5f));
			skip_indicator->set_text("[Ctrl] SKIP");
		}
	}
}

void ClockGearsMessageWindow::_update_window_position_layout() {
	if (!main_panel) return;

	Size2 vp_size = Size2(1280, 720);
	Viewport *vp = get_viewport();
	if (vp) {
		Rect2 vr = vp->get_visible_rect();
		if (vr.size.x > 100 && vr.size.y > 100) {
			vp_size = vr.size;
		}
	}

	set_position(Vector2(0, 0));
	set_size(vp_size);
	set_modulate(Color(1, 1, 1, 1));
	set_self_modulate(Color(1, 1, 1, 1));

	float margin_x = MAX(20.0f, vp_size.x * 0.04f);
	float win_w = vp_size.x - (margin_x * 2.0f);
	float win_h = CLAMP(vp_size.y * 0.28f, 130.0f, 220.0f);
	float win_x = margin_x;
	float win_y = 0.0f;

	switch (window_pos) {
		case POS_TOP:
			win_y = 16.0f;
			break;
		case POS_MIDDLE:
			win_y = (vp_size.y - win_h) * 0.5f;
			break;
		case POS_BOTTOM:
		default:
			win_y = vp_size.y - win_h - 16.0f;
			break;
	}

	main_panel->set_anchor(SIDE_LEFT, 0.0f);
	main_panel->set_anchor(SIDE_TOP, 0.0f);
	main_panel->set_anchor(SIDE_RIGHT, 0.0f);
	main_panel->set_anchor(SIDE_BOTTOM, 0.0f);
	main_panel->set_position(Vector2(win_x, win_y));
	main_panel->set_size(Size2(win_w, win_h));
	main_panel->set_modulate(Color(1, 1, 1, 1));
	main_panel->show();

	if (name_panel) {
		name_panel->set_anchor(SIDE_LEFT, 0.0f);
		name_panel->set_anchor(SIDE_TOP, 0.0f);
		name_panel->set_anchor(SIDE_RIGHT, 0.0f);
		name_panel->set_anchor(SIDE_BOTTOM, 0.0f);
		name_panel->set_position(Vector2(win_x + 16.0f, MAX(0.0f, win_y - 28.0f)));
		float name_w = MAX(100.0f, name_label ? name_label->get_combined_minimum_size().x + 24.0f : 100.0f);
		name_panel->set_size(Size2(name_w, 26.0f));
	}

	if (gold_panel) {
		gold_panel->set_anchor(SIDE_LEFT, 0.0f);
		gold_panel->set_anchor(SIDE_TOP, 0.0f);
		gold_panel->set_anchor(SIDE_RIGHT, 0.0f);
		gold_panel->set_anchor(SIDE_BOTTOM, 0.0f);
		gold_panel->set_position(Vector2(vp_size.x - 180.0f, 16.0f));
		gold_panel->set_size(Size2(160.0f, 40.0f));
	}

	if (choices_panel) {
		choices_panel->set_anchor(SIDE_LEFT, 0.0f);
		choices_panel->set_anchor(SIDE_TOP, 0.0f);
		choices_panel->set_anchor(SIDE_RIGHT, 0.0f);
		choices_panel->set_anchor(SIDE_BOTTOM, 0.0f);
		float cw = MIN(320.0f, vp_size.x * 0.4f);
		float ch = MIN(220.0f, vp_size.y * 0.4f);
		choices_panel->set_position(Vector2(vp_size.x - cw - margin_x, MAX(16.0f, win_y - ch - 8.0f)));
		choices_panel->set_size(Size2(cw, ch));
	}

	if (number_panel) {
		number_panel->set_anchor(SIDE_LEFT, 0.0f);
		number_panel->set_anchor(SIDE_TOP, 0.0f);
		number_panel->set_anchor(SIDE_RIGHT, 0.0f);
		number_panel->set_anchor(SIDE_BOTTOM, 0.0f);
		float nw = 280.0f;
		float nh = 70.0f;
		number_panel->set_position(Vector2((vp_size.x - nw) * 0.5f, (vp_size.y - nh) * 0.5f));
		number_panel->set_size(Size2(nw, nh));
	}

	if (history_panel) {
		history_panel->set_anchor(SIDE_LEFT, 0.0f);
		history_panel->set_anchor(SIDE_TOP, 0.0f);
		history_panel->set_anchor(SIDE_RIGHT, 0.0f);
		history_panel->set_anchor(SIDE_BOTTOM, 0.0f);
		history_panel->set_position(Vector2(margin_x, 20.0f));
		history_panel->set_size(Size2(win_w, vp_size.y - 40.0f));
	}
}

void ClockGearsMessageWindow::_update_background_style() {
	if (!main_panel) return;
	switch (window_bg) {
		case BG_DIM:
			main_panel->add_theme_style_override("panel", style_dim);
			break;
		case BG_TRANSPARENT:
			main_panel->add_theme_style_override("panel", style_transparent);
			break;
		case BG_WINDOW:
		default:
			main_panel->add_theme_style_override("panel", style_window);
			break;
	}
}

void ClockGearsMessageWindow::set_face(const String &p_path, int p_frame, const String &p_expression) {
	current_face_base_path = p_path;
	current_expression = p_expression;

	int frame = p_frame;
	if (!p_expression.is_empty()) {
		String exp = p_expression.to_lower().strip_edges();
		if (exp.is_valid_int()) {
			frame = exp.to_int();
		} else if (exp == "neutral" || exp == "normal") {
			frame = 0;
		} else if (exp == "feliz" || exp == "happy" || exp == "alegre" || exp == "sonrisa") {
			frame = 1;
		} else if (exp == "triste" || exp == "sad") {
			frame = 2;
		} else if (exp == "enojado" || exp == "angry" || exp == "ira" || exp == "molesto") {
			frame = 3;
		} else if (exp == "sorprendido" || exp == "surprised" || exp == "asombro") {
			frame = 4;
		} else if (exp == "asustado" || exp == "scared" || exp == "miedo") {
			frame = 5;
		} else if (exp == "pensativo" || exp == "thinking") {
			frame = 6;
		} else if (exp.begins_with("gui") || exp == "wink") {
			frame = 7;
		}
	}
	current_face_frame = frame;

	if (p_path.is_empty()) {
		if (face_frame) face_frame->hide();
		return;
	}

	Ref<Texture2D> tex;
	// 1. Probar archivo con sufijo de expresión (ej: Actor1_feliz.png)
	if (!p_expression.is_empty()) {
		String exp_path = p_path.get_basename() + "_" + p_expression + "." + p_path.get_extension();
		if (ResourceLoader::exists(exp_path)) {
			tex = ResourceLoader::load(exp_path);
		} else {
			Ref<Image> img = Image::load_from_file(exp_path);
			if (img.is_valid() && !img->is_empty()) {
				tex = ImageTexture::create_from_image(img);
			}
		}
	}

	// 2. Si no hay archivo específico, cargar archivo base
	if (tex.is_null()) {
		if (ResourceLoader::exists(p_path)) {
			tex = ResourceLoader::load(p_path);
		}
		if (tex.is_null()) {
			Ref<Image> img = Image::load_from_file(p_path);
			if (img.is_valid() && !img->is_empty()) {
				tex = ImageTexture::create_from_image(img);
			}
		}
	}

	if (tex.is_null()) {
		if (face_frame) face_frame->hide();
		return;
	}

	Size2 sz = tex->get_size();
	if (sz.x >= 144 && sz.y >= 144 && frame >= 0 && frame < 8) {
		float fw = sz.x / 4.0f;
		float fh = sz.y / 2.0f;
		int fx = frame % 4;
		int fy = frame / 4;
		Ref<AtlasTexture> atlas;
		atlas.instantiate();
		atlas->set_atlas(tex);
		atlas->set_region(Rect2(fx * fw, fy * fh, fw, fh));
		tex = atlas;
	}

	if (face_rect) {
		face_rect->set_texture(tex);
	}
	if (face_frame) {
		face_frame->show();
	}
}

void ClockGearsMessageWindow::show_message(const String &p_text, const String &p_speaker, const String &p_face_path, int p_pos, int p_bg) {
	set_process(true);
	set_process_unhandled_input(true);
	window_pos = (WindowPosition)CLAMP(p_pos, 0, 2);
	window_bg = (WindowBackground)CLAMP(p_bg, 0, 2);
	_update_window_position_layout();
	_update_background_style();

	waiting_for_release = true;
	input_block_frames = 2; // Ignorar tecla en los primeros 2 frames de apertura
	is_auto_mode = false;
	is_skip_mode = false;
	_update_mode_indicators();

	active_message = ClockGearsTextParser::parse_full(p_text, p_speaker);
	current_token_idx = 0;
	visible_chars_count = 0;
	type_timer = 0.0f;
	token_wait_timer = 0.0f;
	auto_wait_timer = 0.0f;
	instant_text_active = false;

	String speaker = active_message.speaker_name;
	current_speaker_name = speaker;
	if (name_panel && name_label) {
		if (!speaker.is_empty()) {
			name_label->set_text(speaker);
			name_panel->show();
		} else {
			name_panel->hide();
		}
	}

	set_face(p_face_path, active_message.face_index, active_message.face_expression);

	if (gold_panel) {
		if (active_message.show_gold) {
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			int gold = dm ? dm->get_gold() : 0;
			gold_label->set_text(vformat(CG_MW_TR("%d G"), gold));
			gold_panel->show();
		} else {
			gold_panel->hide();
		}
	}

	if (rich_text_label) {
		rich_text_label->set_text(active_message.bbcode_text);
		rich_text_label->set_visible_characters(0);
	}
	if (continue_arrow) {
		continue_arrow->hide();
	}

	add_history_entry(speaker, active_message.plain_text);

	show();
	main_panel->show();

	if (active_message.plain_text.strip_edges().is_empty()) {
		current_state = STATE_WAITING;
		if (continue_arrow) continue_arrow->show();
	} else {
		current_state = STATE_TYPING;
	}
	queue_redraw();
}

void ClockGearsMessageWindow::show_choices(const PackedStringArray &p_choices, int p_cancel_type, int p_default_idx) {
	choice_cancel_type = p_cancel_type;
	for (int i = 0; i < choice_buttons.size(); i++) {
		if (choice_buttons[i]) {
			choice_buttons[i]->queue_free();
		}
	}
	choice_buttons.clear();

	for (int i = 0; i < p_choices.size(); i++) {
		Button *btn = memnew(Button(p_choices[i]));
		btn->set_h_size_flags(SIZE_EXPAND_FILL);
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		btn->set_focus_mode(FOCUS_NONE);
		btn->connect("pressed", callable_mp(this, &ClockGearsMessageWindow::_on_choice_btn_pressed).bind(i));
		choices_vbox->add_child(btn);
		choice_buttons.push_back(btn);
	}

	_update_window_position_layout();
	if (choices_panel) {
		choices_panel->show();
	}
	current_state = STATE_CHOICES;
	show();
}

void ClockGearsMessageWindow::_on_choice_btn_pressed(int p_idx) {
	if (choices_panel) {
		choices_panel->hide();
	}
	close_window();
	emit_signal("choice_selected", p_idx);
}

void ClockGearsMessageWindow::show_number_input(int p_variable_id, int p_digits) {
	number_target_variable = p_variable_id;
	number_digit_count = CLAMP(p_digits, 1, 8);
	number_active_digit_idx = 0;
	number_digits.resize(number_digit_count);
	for (int i = 0; i < number_digit_count; i++) {
		number_digits.write[i] = 0;
	}

	for (int i = 0; i < digit_labels.size(); i++) {
		if (digit_labels[i]) digit_labels[i]->queue_free();
	}
	digit_labels.clear();

	for (int i = 0; i < number_digit_count; i++) {
		Label *lbl = memnew(Label("0"));
		lbl->add_theme_font_size_override("font_size", 24);
		lbl->set_focus_mode(FOCUS_NONE);
		number_hbox->add_child(lbl);
		digit_labels.push_back(lbl);
	}

	_update_number_display();
	_update_window_position_layout();
	if (number_panel) {
		number_panel->show();
	}
	current_state = STATE_NUMBER_INPUT;
	show();
}

void ClockGearsMessageWindow::_update_number_display() {
	for (int i = 0; i < digit_labels.size(); i++) {
		digit_labels[i]->set_text(itos(number_digits[i]));
		if (i == number_active_digit_idx) {
			digit_labels[i]->add_theme_color_override("font_color", Color(0.95f, 0.8f, 0.3f));
		} else {
			digit_labels[i]->add_theme_color_override("font_color", Color(0.8f, 0.8f, 0.8f));
		}
	}
}

void ClockGearsMessageWindow::_confirm_number_input() {
	int total = 0;
	for (int i = 0; i < number_digits.size(); i++) {
		total = total * 10 + number_digits[i];
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && number_target_variable > 0) {
		dm->set_variable(itos(number_target_variable), total);
	}
	if (number_panel) {
		number_panel->hide();
	}
	close_window();
	emit_signal("number_entered", total);
}

void ClockGearsMessageWindow::close_window() {
	close_window_with_reason("cierre general");
}

void ClockGearsMessageWindow::close_window_with_reason(const String &p_reason) {
	if (current_state == STATE_CLOSED) {
		return;
	}
	current_state = STATE_CLOSED;
	hide();
	if (gold_panel) gold_panel->hide();
	if (choices_panel) choices_panel->hide();
	if (number_panel) number_panel->hide();
	emit_signal("message_completed");
}

void ClockGearsMessageWindow::advance_or_close() {
	close_window();
}

void ClockGearsMessageWindow::set_auto_mode(bool p_enable) {
	is_auto_mode = p_enable;
	_update_mode_indicators();
}

void ClockGearsMessageWindow::set_skip_mode(bool p_enable) {
	is_skip_mode = p_enable;
	_update_mode_indicators();
	if (is_skip_mode && current_state == STATE_TYPING) {
		instant_text_active = true;
	}
}

void ClockGearsMessageWindow::open_history() {
	if (!history_panel) return;
	for (int i = 0; i < history_vbox->get_child_count(); i++) {
		history_vbox->get_child(i)->queue_free();
	}

	for (int i = 0; i < dialogue_history.size(); i++) {
		const DialogueHistoryItem &item = dialogue_history[i];
		VBoxContainer *row = memnew(VBoxContainer);
		row->add_theme_constant_override("separation", 2);

		if (!item.speaker.is_empty()) {
			Label *spk = memnew(Label(item.speaker + ":"));
			spk->add_theme_color_override("font_color", Color(0.95f, 0.78f, 0.35f));
			row->add_child(spk);
		}

		Label *txt = memnew(Label(item.text));
		txt->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		txt->add_theme_color_override("font_color", Color(0.9f, 0.9f, 0.9f));
		row->add_child(txt);

		history_vbox->add_child(row);
		history_vbox->add_child(memnew(HSeparator));
	}

	history_panel->show();
}

void ClockGearsMessageWindow::close_history() {
	if (history_panel) {
		history_panel->hide();
	}
}

bool ClockGearsMessageWindow::is_history_open() const {
	return history_panel && history_panel->is_visible();
}

void ClockGearsMessageWindow::add_history_entry(const String &p_speaker, const String &p_text) {
	if (p_text.is_empty()) return;
	DialogueHistoryItem item;
	item.speaker = p_speaker;
	item.text = p_text;
	dialogue_history.push_back(item);
	while (dialogue_history.size() > 100) {
		dialogue_history.remove_at(0);
	}
}

void ClockGearsMessageWindow::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		set_process(true);
		set_process_unhandled_input(true);
		_update_window_position_layout();
	} else if (p_what == NOTIFICATION_RESIZED) {
		_update_window_position_layout();
	} else if (p_what == NOTIFICATION_PROCESS) {
		float delta = get_process_delta_time();

		if (input_block_frames > 0) {
			input_block_frames--;
		}

		if (waiting_for_release) {
			Input *inp = Input::get_singleton();
			if (inp) {
				bool any_action_down = inp->is_key_pressed(Key::ENTER) || inp->is_key_pressed(Key::KP_ENTER) ||
						inp->is_key_pressed(Key::SPACE) || inp->is_key_pressed(Key::Z) ||
						inp->is_mouse_button_pressed(MouseButton::LEFT);
				if (!any_action_down && input_block_frames <= 0) {
					waiting_for_release = false;
				}
			} else {
				if (input_block_frames <= 0) {
					waiting_for_release = false;
				}
			}
		}

		// Parpadeo de la flecha ▼
		if (continue_arrow && continue_arrow->is_visible()) {
			arrow_blink_timer += delta * 4.0f;
			continue_arrow->set_modulate(Color(1, 1, 1, Math::sin(arrow_blink_timer) > 0.0f ? 1.0f : 0.2f));
		}

		if (current_state == STATE_TYPING) {
			if (token_wait_timer > 0.0f) {
				token_wait_timer -= delta;
			} else {
				_advance_typewriter(delta);
			}
		} else if (current_state == STATE_WAITING) {
			if (is_skip_mode) {
				close_window_with_reason("skip");
				return;
			}
			if (is_auto_mode) {
				auto_wait_timer += delta;
				float max_wait = 1.2f + active_message.plain_text.length() * 0.035f;
				if (auto_wait_timer >= max_wait) {
					close_window_with_reason("auto");
					return;
				}
			}
			// En WAITING normal: NO hay temporizador, espera sin límite la tecla de acción.
		}
	}
}

void ClockGearsMessageWindow::_advance_typewriter(float p_delta) {
	if (instant_text_active) {
		if (rich_text_label) {
			rich_text_label->set_visible_characters(-1);
		}
		if (continue_arrow) {
			continue_arrow->show();
		}
		if (active_message.no_input_wait) {
			close_window_with_reason("codigo ^ (no_input_wait)");
		} else {
			current_state = STATE_WAITING;
		}
		return;
	}

	float speed = (float)MAX(1, active_message.typing_speed);
	float char_interval = 1.0f / speed;
	type_timer += p_delta;

	while (type_timer >= char_interval && current_state == STATE_TYPING) {
		type_timer -= char_interval;

		if (current_token_idx < active_message.execution_tokens.size()) {
			const ClockGearsTextParser::Token &tok = active_message.execution_tokens[current_token_idx++];
			switch (tok.type) {
				case ClockGearsTextParser::TOKEN_CHAR:
					visible_chars_count++;
					if (rich_text_label) {
						rich_text_label->set_visible_characters(visible_chars_count);
					}
					break;
				case ClockGearsTextParser::TOKEN_PAUSE_SHORT:
				case ClockGearsTextParser::TOKEN_PAUSE_LONG:
				case ClockGearsTextParser::TOKEN_WAIT_FRAMES:
					token_wait_timer = tok.int_param / 60.0f;
					return;
				case ClockGearsTextParser::TOKEN_WAIT_INPUT:
					current_state = STATE_WAITING;
					if (continue_arrow) continue_arrow->show();
					waiting_for_release = true;
					input_block_frames = 2;
					return;
				case ClockGearsTextParser::TOKEN_INSTANT_ON:
					instant_text_active = true;
					break;
				case ClockGearsTextParser::TOKEN_INSTANT_OFF:
					instant_text_active = false;
					break;
				case ClockGearsTextParser::TOKEN_SPEED:
					active_message.typing_speed = tok.int_param;
					break;
				case ClockGearsTextParser::TOKEN_EXPRESSION:
					set_face(current_face_base_path, active_message.face_index, tok.str_param);
					break;
				case ClockGearsTextParser::TOKEN_FACE_INDEX:
					set_face(current_face_base_path, tok.int_param, active_message.face_expression);
					break;
				case ClockGearsTextParser::TOKEN_NAME:
					if (name_label && name_panel) {
						name_label->set_text(tok.str_param);
						name_panel->set_visible(!tok.str_param.is_empty());
					}
					break;
				default:
					break;
			}
		} else {
			// Todo el texto mostrado
			if (rich_text_label) {
				rich_text_label->set_visible_characters(-1);
			}
			if (continue_arrow) {
				continue_arrow->show();
			}
			if (active_message.no_input_wait) {
				close_window_with_reason("codigo ^ (no_input_wait)");
			} else {
				current_state = STATE_WAITING;
			}
			return;
		}
	}
}

void ClockGearsMessageWindow::unhandled_input(const Ref<InputEvent> &p_event) {
	if (current_state == STATE_CLOSED || !is_visible_in_tree()) {
		return;
	}

	Ref<InputEventKey> k = p_event;
	Ref<InputEventMouseButton> mb = p_event;

	if (k.is_valid() && !k->is_pressed()) {
		if (k->get_keycode() == Key::ENTER || k->get_keycode() == Key::KP_ENTER || k->get_keycode() == Key::SPACE || k->get_keycode() == Key::Z) {
			waiting_for_release = false;
		}
	}
	if (mb.is_valid() && !mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		waiting_for_release = false;
	}

	if (is_history_open()) {
		if (k.is_valid() && k->is_pressed()) {
			if (k->get_keycode() == Key::ESCAPE || k->get_keycode() == Key::L) {
				close_history();
				get_viewport()->set_input_as_handled();
			}
		}
		return;
	}

	// Atajos globales: L/PageUp = Historial, Tab = Auto, Ctrl = Skip
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::L || k->get_keycode() == Key::PAGEUP) {
			open_history();
			get_viewport()->set_input_as_handled();
			return;
		}
		if (k->get_keycode() == Key::TAB) {
			set_auto_mode(!is_auto_mode);
			get_viewport()->set_input_as_handled();
			return;
		}
		if (k->get_keycode() == Key::CTRL) {
			set_skip_mode(!is_skip_mode);
			get_viewport()->set_input_as_handled();
			return;
		}
	}

	if (current_state == STATE_NUMBER_INPUT) {
		if (k.is_valid() && k->is_pressed()) {
			if (k->get_keycode() == Key::LEFT) {
				number_active_digit_idx = (number_active_digit_idx - 1 + number_digit_count) % number_digit_count;
				_update_number_display();
				get_viewport()->set_input_as_handled();
			} else if (k->get_keycode() == Key::RIGHT) {
				number_active_digit_idx = (number_active_digit_idx + 1) % number_digit_count;
				_update_number_display();
				get_viewport()->set_input_as_handled();
			} else if (k->get_keycode() == Key::UP) {
				number_digits.write[number_active_digit_idx] = (number_digits[number_active_digit_idx] + 1) % 10;
				_update_number_display();
				get_viewport()->set_input_as_handled();
			} else if (k->get_keycode() == Key::DOWN) {
				number_digits.write[number_active_digit_idx] = (number_digits[number_active_digit_idx] - 1 + 10) % 10;
				_update_number_display();
				get_viewport()->set_input_as_handled();
			} else if (k->get_keycode() == Key::ENTER || k->get_keycode() == Key::KP_ENTER || k->get_keycode() == Key::SPACE) {
				_confirm_number_input();
				get_viewport()->set_input_as_handled();
			}
		}
		return;
	}

	bool is_advance = false;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::ENTER || k->get_keycode() == Key::KP_ENTER || k->get_keycode() == Key::SPACE || k->get_keycode() == Key::Z) {
			is_advance = true;
		}
	}
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		is_advance = true;
	}

	if (is_advance) {
		get_viewport()->set_input_as_handled(); // CONSUMIR LA TECLA DE ACCIÓN

		if (input_block_frames > 0 || waiting_for_release) {
			return;
		}

		if (current_state == STATE_TYPING) {
			instant_text_active = true;
			if (rich_text_label) {
				rich_text_label->set_visible_characters(-1);
			}
			if (continue_arrow) {
				continue_arrow->show();
			}
			current_state = STATE_WAITING;
			waiting_for_release = true;
			input_block_frames = 2;
			return;
		}

		if (current_state == STATE_WAITING) {
			if (current_token_idx < active_message.execution_tokens.size()) {
				// Pausa intermedia (\!): reanudar escritura
				current_state = STATE_TYPING;
				if (continue_arrow) {
					continue_arrow->hide();
				}
				waiting_for_release = true;
				input_block_frames = 2;
				return;
			}
			close_window_with_reason("tecla de accion del jugador");
			return;
		}
	}
}
