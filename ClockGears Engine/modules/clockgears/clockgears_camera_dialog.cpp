#include "clockgears_camera_dialog.h"
#include "clockgears_map.h"
#include "core/clockgears_data_manager.h"
#include "core/config/project_settings.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/separator.h"
#include "scene/gui/panel_container.h"
#include "scene/resources/style_box_flat.h"

#define CG_CAM_TR(m_str) String::utf8(m_str)

static Ref<StyleBoxFlat> _camera_card_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.10f, 0.11f, 0.13f));
	style->set_corner_radius_all(6);
	style->set_content_margin_all(8);
	style->set_border_width_all(1);
	style->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.08f));
	return style;
}

// ============================================================================
// ClockGearsCameraPreviewCanvas
// ============================================================================

void ClockGearsCameraPreviewCanvas::_bind_methods() {
}

void ClockGearsCameraPreviewCanvas::set_preview_data(const Vector2 &p_map_size_px, const Vector2 &p_start_px, const Rect2 &p_cam_rect, float p_zoom, int p_grid) {
	map_size_px = p_map_size_px.max(Vector2(64, 64));
	player_start_px = p_start_px;
	camera_visible_rect = p_cam_rect;
	camera_zoom = MAX(0.1f, p_zoom);
	grid_res = MAX(16, p_grid);
	queue_redraw();
}

void ClockGearsCameraPreviewCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Size2 sz = get_size();
		if (sz.x <= 10 || sz.y <= 10) return;

		// Fondo oscuro con rejilla
		draw_rect(Rect2(Point2(), sz), Color(0.06f, 0.07f, 0.09f), true);

		// Calcular escala para encajar el mapa en el lienzo dejando un margen de 16 px
		float margin = 20.0f;
		float avail_w = sz.x - margin * 2.0f;
		float avail_h = sz.y - margin * 2.0f;
		float scale = MIN(avail_w / map_size_px.x, avail_h / map_size_px.y);
		if (scale <= 0.001f) scale = 0.1f;

		Vector2 map_draw_sz = map_size_px * scale;
		Vector2 map_offset = Vector2((sz.x - map_draw_sz.x) * 0.5f, (sz.y - map_draw_sz.y) * 0.5f);

		// Área del mapa
		Rect2 map_rect(map_offset, map_draw_sz);
		draw_rect(map_rect, Color(0.12f, 0.14f, 0.18f), true);
		draw_rect(map_rect, Color(0.35f, 0.40f, 0.48f, 0.8f), false, 1.5f);

		// Rejilla sutil del mapa
		float grid_px = float(grid_res) * scale;
		if (grid_px >= 6.0f) {
			Color c_grid(1.0f, 1.0f, 1.0f, 0.06f);
			for (float gx = map_offset.x + grid_px; gx < map_offset.x + map_draw_sz.x - 1.0f; gx += grid_px) {
				draw_line(Vector2(gx, map_offset.y), Vector2(gx, map_offset.y + map_draw_sz.y), c_grid, 1.0f);
			}
			for (float gy = map_offset.y + grid_px; gy < map_offset.y + map_draw_sz.y - 1.0f; gy += grid_px) {
				draw_line(Vector2(map_offset.x, gy), Vector2(map_offset.x + map_draw_sz.x, gy), c_grid, 1.0f);
			}
		}

		// Marco de cámara (Área Visible del Viewport)
		Vector2 cam_tl = map_offset + camera_visible_rect.position * scale;
		Vector2 cam_sz = camera_visible_rect.size * scale;
		Rect2 cam_draw_rect(cam_tl, cam_sz);

		// Relleno semi-transparente y borde cyan brillante
		draw_rect(cam_draw_rect, Color(0.15f, 0.55f, 1.0f, 0.18f), true);
		draw_rect(cam_draw_rect, Color(0.25f, 0.82f, 1.0f, 0.95f), false, 2.0f);

		// Esquinas doradas de encuadre
		float csz = MIN(10.0f, MIN(cam_sz.x * 0.25f, cam_sz.y * 0.25f));
		Color c_gold(1.0f, 0.85f, 0.25f, 1.0f);
		Point2 tl = cam_draw_rect.position;
		Point2 br = cam_draw_rect.position + cam_draw_rect.size;
		draw_line(tl, tl + Vector2(csz, 0), c_gold, 2.5f);
		draw_line(tl, tl + Vector2(0, csz), c_gold, 2.5f);
		draw_line(Vector2(br.x, tl.y), Vector2(br.x - csz, tl.y), c_gold, 2.5f);
		draw_line(Vector2(br.x, tl.y), Vector2(br.x, tl.y + csz), c_gold, 2.5f);
		draw_line(Vector2(tl.x, br.y), Vector2(tl.x + csz, br.y), c_gold, 2.5f);
		draw_line(Vector2(tl.x, br.y), Vector2(tl.x, br.y - csz), c_gold, 2.5f);
		draw_line(br, br - Vector2(csz, 0), c_gold, 2.5f);
		draw_line(br, br - Vector2(0, csz), c_gold, 2.5f);

		// Posición inicial del jugador
		Vector2 p_pos = map_offset + player_start_px * scale;
		draw_circle(p_pos, 4.5f, Color(0.2f, 1.0f, 0.4f, 0.95f));
		draw_arc(p_pos, 7.0f, 0, Math::TAU, 16, Color(1.0f, 1.0f, 1.0f, 0.7f), 1.5f);
	}
}

ClockGearsCameraPreviewCanvas::ClockGearsCameraPreviewCanvas() {
	set_custom_minimum_size(Size2(440, 200) * EDSCALE);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
}

// ============================================================================
// ClockGearsCameraDialog
// ============================================================================

void ClockGearsCameraDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_camera_dialog"), &ClockGearsCameraDialog::open_camera_dialog);
}

void ClockGearsCameraDialog::_on_preset_selected(int p_idx) {
	if (updating_ui) return;
	updating_ui = true;
	switch (p_idx) {
		case 0: // 960 x 540
			spin_width->set_value(960);
			spin_height->set_value(540);
			break;
		case 1: // 1280 x 720
			spin_width->set_value(1280);
			spin_height->set_value(720);
			break;
		case 2: // 1920 x 1080
			spin_width->set_value(1920);
			spin_height->set_value(1080);
			break;
		default:
			break;
	}
	updating_ui = false;
	_update_preview();
}

void ClockGearsCameraDialog::_on_dimension_changed(double p_val) {
	if (updating_ui) return;
	int w = int(spin_width->get_value());
	int h = int(spin_height->get_value());
	if (w == 960 && h == 540) opt_res_preset->select(0);
	else if (w == 1280 && h == 720) opt_res_preset->select(1);
	else if (w == 1920 && h == 1080) opt_res_preset->select(2);
	else opt_res_preset->select(3);
	_update_preview();
}

void ClockGearsCameraDialog::_on_zoom_slider_changed(double p_val) {
	if (updating_ui) return;
	updating_ui = true;
	float z = float(p_val);
	if (chk_integer_zoom && chk_integer_zoom->is_pressed()) {
		z = Math::round(z);
		if (z < 1.0f) z = 1.0f;
	}
	spin_zoom->set_value(z);
	updating_ui = false;
	_update_preview();
}

void ClockGearsCameraDialog::_on_zoom_spin_changed(double p_val) {
	if (updating_ui) return;
	updating_ui = true;
	float z = float(p_val);
	if (chk_integer_zoom && chk_integer_zoom->is_pressed()) {
		z = Math::round(z);
		if (z < 1.0f) z = 1.0f;
		spin_zoom->set_value(z);
	}
	slider_zoom->set_value(z);
	updating_ui = false;
	_update_preview();
}

void ClockGearsCameraDialog::_on_integer_snap_toggled(bool p_toggled) {
	if (p_toggled) {
		float cur_z = float(spin_zoom->get_value());
		float rounded_z = Math::round(cur_z);
		if (rounded_z < 1.0f) rounded_z = 1.0f;
		spin_zoom->set_value(rounded_z);
	}
	_update_preview();
}

void ClockGearsCameraDialog::_on_camera_param_changed() {
	if (updating_ui) return;
	if (spin_smooth_speed) {
		spin_smooth_speed->set_editable(chk_smooth ? chk_smooth->is_pressed() : true);
	}
	_update_preview();
}

void ClockGearsCameraDialog::_on_screen_mode_selected(int p_idx) {
	_update_preview();
}

void ClockGearsCameraDialog::_update_preview() {
	int vw = int(spin_width->get_value());
	int vh = int(spin_height->get_value());
	float z = float(spin_zoom->get_value());
	if (z <= 0.05f) z = 1.0f;

	float vis_w = float(vw) / z;
	float vis_h = float(vh) / z;

	// Obtener datos del mapa activo o por defecto
	Vector2 map_sz_px = Vector2(40 * 32, 30 * 32);
	int grid = 32;
	Vector2 player_start = Vector2(5 * 32 + 16, 5 * 32 + 16);

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Dictionary sys = dm->get_system_config();
		int sx = int(sys.get("start_x", 0));
		int sy = int(sys.get("start_y", 0));
		player_start = Vector2((sx + 0.5f) * grid, (sy + 0.5f) * grid);
	}

	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
	if (map) {
		grid = map->get_grid_resolution();
		map_sz_px = Vector2(map->get_map_size().x * grid, map->get_map_size().y * grid);
		if (dm) {
			Dictionary sys = dm->get_system_config();
			int sx = int(sys.get("start_x", 0));
			int sy = int(sys.get("start_y", 0));
			player_start = Vector2((sx + 0.5f) * grid, (sy + 0.5f) * grid);
		}
	}

	// Centrar cámara en el jugador
	Vector2 cam_center = player_start;

	// Si detenerse en los bordes está activo, recortar al mapa
	bool clamp = chk_clamp_map ? chk_clamp_map->is_pressed() : true;
	if (clamp) {
		if (vis_w < map_sz_px.x) {
			cam_center.x = CLAMP(cam_center.x, vis_w * 0.5f, map_sz_px.x - vis_w * 0.5f);
		} else {
			cam_center.x = map_sz_px.x * 0.5f;
		}
		if (vis_h < map_sz_px.y) {
			cam_center.y = CLAMP(cam_center.y, vis_h * 0.5f, map_sz_px.y - vis_h * 0.5f);
		} else {
			cam_center.y = map_sz_px.y * 0.5f;
		}
	}

	Rect2 cam_rect(cam_center - Vector2(vis_w * 0.5f, vis_h * 0.5f), Vector2(vis_w, vis_h));

	if (preview_canvas) {
		preview_canvas->set_preview_data(map_sz_px, player_start, cam_rect, z, grid);
	}

	if (lbl_preview_info) {
		float tiles_w = vis_w / float(grid);
		float tiles_h = vis_h / float(grid);
		lbl_preview_info->set_text(vformat(CG_CAM_TR("Área visible del juego: %d × %d px (%.1f × %.1f tiles) | Zoom: %.1fx"), int(vis_w), int(vis_h), tiles_w, tiles_h, z));
	}

	// Dibujar el marco en tiempo real sobre el lienzo del mapa en el editor
	if (is_visible() && CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_camera_preview(true, cam_rect);
	}
}

void ClockGearsCameraDialog::_load_settings() {
	updating_ui = true;

	ProjectSettings *ps = ProjectSettings::get_singleton();
	int w = ps ? int(ps->get_setting("display/window/size/viewport_width", 960)) : 960;
	int h = ps ? int(ps->get_setting("display/window/size/viewport_height", 540)) : 540;
	spin_width->set_value(w);
	spin_height->set_value(h);

	if (w == 960 && h == 540) opt_res_preset->select(0);
	else if (w == 1280 && h == 720) opt_res_preset->select(1);
	else if (w == 1920 && h == 1080) opt_res_preset->select(2);
	else opt_res_preset->select(3);

	float cam_zoom = 2.0f;
	bool cam_follow = true;
	bool cam_smooth = true;
	float cam_smooth_speed = 8.0f;
	bool cam_clamp = true;
	bool cam_integer = false;
	int screen_mode = 0;

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Dictionary sys = dm->get_system_config();
		cam_zoom = float(sys.get("camera_zoom", 2.0f));
		if (cam_zoom <= 0.05f) cam_zoom = 2.0f;
		cam_follow = bool(sys.get("camera_follow_player", true));
		cam_smooth = bool(sys.get("camera_smooth", true));
		cam_smooth_speed = float(sys.get("camera_smooth_speed", 8.0f));
		cam_clamp = bool(sys.get("camera_clamp", true));
		cam_integer = bool(sys.get("camera_integer_zoom", false));
		screen_mode = int(sys.get("screen_mode", 0));
	}

	slider_zoom->set_value(cam_zoom);
	spin_zoom->set_value(cam_zoom);
	chk_integer_zoom->set_pressed(cam_integer);

	chk_follow_player->set_pressed(cam_follow);
	chk_smooth->set_pressed(cam_smooth);
	spin_smooth_speed->set_value(cam_smooth_speed);
	spin_smooth_speed->set_editable(cam_smooth);
	chk_clamp_map->set_pressed(cam_clamp);

	opt_screen_mode->select(CLAMP(screen_mode, 0, 2));

	updating_ui = false;
	_update_preview();
}

void ClockGearsCameraDialog::_save_settings() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (ps) {
		int vw = int(spin_width->get_value());
		int vh = int(spin_height->get_value());
		ps->set_setting("display/window/size/viewport_width", vw);
		ps->set_setting("display/window/size/viewport_height", vh);

		int sm = opt_screen_mode->get_selected();
		if (sm == 0) {
			// Escala entera con bordes negros (Pixel Perfect)
			ps->set_setting("display/window/stretch/mode", "viewport");
			ps->set_setting("display/window/stretch/aspect", "keep");
			ps->set_setting("display/window/stretch/scale_mode", "integer");
		} else if (sm == 1) {
			// Ampliar área visible hasta llenar sin deformar (Expand)
			ps->set_setting("display/window/stretch/mode", "viewport");
			ps->set_setting("display/window/stretch/aspect", "expand");
			ps->set_setting("display/window/stretch/scale_mode", "fractional");
		} else {
			// Llenar con escala fraccional
			ps->set_setting("display/window/stretch/mode", "viewport");
			ps->set_setting("display/window/stretch/aspect", "keep");
			ps->set_setting("display/window/stretch/scale_mode", "fractional");
		}

		ps->save();
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Dictionary sys = dm->get_system_config();
		sys["camera_zoom"] = float(spin_zoom->get_value());
		sys["camera_follow_player"] = chk_follow_player->is_pressed();
		sys["camera_smooth"] = chk_smooth->is_pressed();
		sys["camera_smooth_speed"] = float(spin_smooth_speed->get_value());
		sys["camera_clamp"] = chk_clamp_map->is_pressed();
		sys["camera_integer_zoom"] = chk_integer_zoom->is_pressed();
		sys["screen_mode"] = opt_screen_mode->get_selected();
		dm->set_system_config(sys);
		dm->save_all("res://clockgears_data");
	}

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_camera_preview(false, Rect2());
	}
}

void ClockGearsCameraDialog::_on_dialog_confirmed() {
	_save_settings();
}

void ClockGearsCameraDialog::_on_dialog_canceled() {
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_camera_preview(false, Rect2());
	}
}

void ClockGearsCameraDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		if (!is_visible()) {
			if (CanvasItemEditor::get_singleton()) {
				CanvasItemEditor::get_singleton()->set_clockgears_camera_preview(false, Rect2());
			}
		} else {
			_update_preview();
		}
	}
}

void ClockGearsCameraDialog::open_camera_dialog() {
	_load_settings();
	popup_centered(Size2(580, 520) * EDSCALE);
	_update_preview();
}

ClockGearsCameraDialog::ClockGearsCameraDialog() {
	set_title(CG_CAM_TR("Ajustes de Cámara y Pantalla - ClockGears Engine"));
	set_min_size(Size2(560, 500) * EDSCALE);
	set_ok_button_text(CG_CAM_TR("Guardar y Aplicar"));
	set_cancel_button_text(CG_CAM_TR("Cancelar"));

	connect("confirmed", callable_mp(this, &ClockGearsCameraDialog::_on_dialog_confirmed));
	connect("canceled", callable_mp(this, &ClockGearsCameraDialog::_on_dialog_canceled));

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 8);
	add_child(main_vb);

	// 1. Tarjeta de Vista Previa del Viewport
	PanelContainer *prev_card = memnew(PanelContainer);
	prev_card->add_theme_style_override("panel", _camera_card_style());
	main_vb->add_child(prev_card);

	VBoxContainer *prev_box = memnew(VBoxContainer);
	prev_box->add_theme_constant_override("separation", 4);
	prev_card->add_child(prev_box);

	preview_canvas = memnew(ClockGearsCameraPreviewCanvas);
	prev_box->add_child(preview_canvas);

	lbl_preview_info = memnew(Label(CG_CAM_TR("Área visible del juego...")));
	lbl_preview_info->add_theme_color_override("font_color", Color(0.3f, 0.85f, 1.0f));
	lbl_preview_info->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	prev_box->add_child(lbl_preview_info);

	// 2. Tarjeta de Controles y Parámetros
	PanelContainer *ctrl_card = memnew(PanelContainer);
	ctrl_card->add_theme_style_override("panel", _camera_card_style());
	main_vb->add_child(ctrl_card);

	VBoxContainer *ctrl_box = memnew(VBoxContainer);
	ctrl_box->add_theme_constant_override("separation", 6);
	ctrl_card->add_child(ctrl_box);

	// Fila: Resolución Interna con Presets
	HBoxContainer *res_row = memnew(HBoxContainer);
	res_row->add_theme_constant_override("separation", 6);
	res_row->add_child(memnew(Label(CG_CAM_TR("Resolución Interna:"))));

	opt_res_preset = memnew(OptionButton);
	opt_res_preset->add_item(CG_CAM_TR("960 × 540 (nHD 16:9 Nativo)"), 0);
	opt_res_preset->add_item(CG_CAM_TR("1280 × 720 (HD 16:9)"), 1);
	opt_res_preset->add_item(CG_CAM_TR("1920 × 1080 (Full HD 16:9)"), 2);
	opt_res_preset->add_item(CG_CAM_TR("Personalizado"), 3);
	opt_res_preset->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_res_preset->connect("item_selected", callable_mp(this, &ClockGearsCameraDialog::_on_preset_selected));
	res_row->add_child(opt_res_preset);

	res_row->add_child(memnew(Label(CG_CAM_TR("Ancho:"))));
	spin_width = memnew(SpinBox);
	spin_width->set_min(160);
	spin_width->set_max(7680);
	spin_width->set_value(960);
	spin_width->connect("value_changed", callable_mp(this, &ClockGearsCameraDialog::_on_dimension_changed));
	res_row->add_child(spin_width);

	res_row->add_child(memnew(Label(CG_CAM_TR("Alto:"))));
	spin_height = memnew(SpinBox);
	spin_height->set_min(120);
	spin_height->set_max(4320);
	spin_height->set_value(540);
	spin_height->connect("value_changed", callable_mp(this, &ClockGearsCameraDialog::_on_dimension_changed));
	res_row->add_child(spin_height);
	ctrl_box->add_child(res_row);

	// Fila: Zoom Decimal con Slider y SpinBox sincronizados
	HBoxContainer *zoom_row = memnew(HBoxContainer);
	zoom_row->add_theme_constant_override("separation", 6);
	zoom_row->add_child(memnew(Label(CG_CAM_TR("Zoom de Cámara:"))));

	slider_zoom = memnew(HSlider);
	slider_zoom->set_min(0.25);
	slider_zoom->set_max(4.0);
	slider_zoom->set_step(0.1);
	slider_zoom->set_value(2.0);
	slider_zoom->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slider_zoom->connect("value_changed", callable_mp(this, &ClockGearsCameraDialog::_on_zoom_slider_changed));
	zoom_row->add_child(slider_zoom);

	spin_zoom = memnew(SpinBox);
	spin_zoom->set_min(0.25);
	spin_zoom->set_max(4.0);
	spin_zoom->set_step(0.1);
	spin_zoom->set_value(2.0);
	spin_zoom->connect("value_changed", callable_mp(this, &ClockGearsCameraDialog::_on_zoom_spin_changed));
	zoom_row->add_child(spin_zoom);
	ctrl_box->add_child(zoom_row);

	// Casilla: Ajustar zoom a entero (pixel perfect)
	chk_integer_zoom = memnew(CheckBox(CG_CAM_TR("Ajustar zoom a entero (pixel perfect: 1x, 2x, 3x, 4x)")));
	chk_integer_zoom->set_pressed(false); // Desactivada por defecto como se solicitó
	chk_integer_zoom->connect("toggled", callable_mp(this, &ClockGearsCameraDialog::_on_integer_snap_toggled));
	ctrl_box->add_child(chk_integer_zoom);

	ctrl_box->add_child(memnew(HSeparator));

	// Comportamiento de Cámara
	HBoxContainer *cam_opts_row = memnew(HBoxContainer);
	cam_opts_row->add_theme_constant_override("separation", 10);

	chk_follow_player = memnew(CheckBox(CG_CAM_TR("Seguir al personaje")));
	chk_follow_player->set_pressed(true);
	chk_follow_player->connect("toggled", callable_mp(this, &ClockGearsCameraDialog::_on_camera_param_changed).unbind(1));
	cam_opts_row->add_child(chk_follow_player);

	chk_smooth = memnew(CheckBox(CG_CAM_TR("Suavizado")));
	chk_smooth->set_pressed(true);
	chk_smooth->connect("toggled", callable_mp(this, &ClockGearsCameraDialog::_on_camera_param_changed).unbind(1));
	cam_opts_row->add_child(chk_smooth);

	cam_opts_row->add_child(memnew(Label(CG_CAM_TR("Velocidad:"))));
	spin_smooth_speed = memnew(SpinBox);
	spin_smooth_speed->set_min(0.5);
	spin_smooth_speed->set_max(60.0);
	spin_smooth_speed->set_step(0.5);
	spin_smooth_speed->set_value(8.0);
	spin_smooth_speed->connect("value_changed", callable_mp(this, &ClockGearsCameraDialog::_on_camera_param_changed).unbind(1));
	cam_opts_row->add_child(spin_smooth_speed);

	chk_clamp_map = memnew(CheckBox(CG_CAM_TR("Detenerse en los bordes del mapa")));
	chk_clamp_map->set_pressed(true);
	chk_clamp_map->connect("toggled", callable_mp(this, &ClockGearsCameraDialog::_on_camera_param_changed).unbind(1));
	cam_opts_row->add_child(chk_clamp_map);

	ctrl_box->add_child(cam_opts_row);

	ctrl_box->add_child(memnew(HSeparator));

	// Modo de Pantalla (Punto 3)
	HBoxContainer *screen_mode_row = memnew(HBoxContainer);
	screen_mode_row->add_theme_constant_override("separation", 6);
	screen_mode_row->add_child(memnew(Label(CG_CAM_TR("Modo de Pantalla:"))));

	opt_screen_mode = memnew(OptionButton);
	opt_screen_mode->add_item(CG_CAM_TR("Escala entera con bordes negros (Pixel Perfect)"), 0);
	opt_screen_mode->add_item(CG_CAM_TR("Ampliar el área visible hasta llenar la ventana sin deformar (Expand)"), 1);
	opt_screen_mode->add_item(CG_CAM_TR("Llenar con escala fraccional (Sin deformar, con bordes si difiere proporción)"), 2);
	opt_screen_mode->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_screen_mode->connect("item_selected", callable_mp(this, &ClockGearsCameraDialog::_on_screen_mode_selected));
	screen_mode_row->add_child(opt_screen_mode);
	ctrl_box->add_child(screen_mode_row);

	Label *lbl_fs_hint = memnew(Label(CG_CAM_TR("💡 Atajo de Pantalla Completa: pulsa F11 o Alt+Enter en cualquier momento durante la partida.")));
	lbl_fs_hint->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.55f));
	ctrl_box->add_child(lbl_fs_hint);
}

ClockGearsCameraDialog::~ClockGearsCameraDialog() {
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_camera_preview(false, Rect2());
	}
}
