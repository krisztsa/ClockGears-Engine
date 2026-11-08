#include "clockgears_picture_picker.h"

#include "clockgears_types.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "scene/resources/image_texture.h"

// ============================================================================
// ClockGearsPicturePreviewCanvas
// ============================================================================

ClockGearsPicturePreviewCanvas::ClockGearsPicturePreviewCanvas(ClockGearsPictureVisualPicker *p_picker) {
	picker = p_picker;
	set_custom_minimum_size(Size2(384, 216));
	set_clip_contents(true);
}

Size2 ClockGearsPicturePreviewCanvas::get_minimum_size() const {
	return Size2(384, 216);
}

bool ClockGearsPicturePreviewCanvas::_get_canvas_transforms(Rect2 &r_screen_rect, float &r_scale, Vector2 &r_box_pos) const {
	Size2 csz = get_size();
	if (csz.x < 32 || csz.y < 32) return false;

	float internal_w = 960.0f;
	float internal_h = 540.0f;
	if (ProjectSettings::get_singleton()) {
		internal_w = float(ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960));
		internal_h = float(ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540));
	}
	if (internal_w <= 0.0f) internal_w = 960.0f;
	if (internal_h <= 0.0f) internal_h = 540.0f;

	float avail_w = MAX(10.0f, csz.x - 16.0f);
	float avail_h = MAX(10.0f, csz.y - 16.0f);
	r_scale = MIN(avail_w / internal_w, avail_h / internal_h);
	if (r_scale <= 0.0001f) r_scale = 0.1f;

	Size2 box_size = Size2(internal_w, internal_h) * r_scale;
	r_box_pos = (csz - box_size) * 0.5f;
	r_screen_rect = Rect2(r_box_pos, box_size);
	return true;
}

bool ClockGearsPicturePreviewCanvas::_get_image_screen_rect(Rect2 &r_img_rect, Rect2 &r_h_tl, Rect2 &r_h_tr, Rect2 &r_h_bl, Rect2 &r_h_br, Rect2 &r_h_t, Rect2 &r_h_b, Rect2 &r_h_l, Rect2 &r_h_r, Vector2 &r_anchor_pt) const {
	if (!picker) return false;

	Rect2 srect;
	float sc = 1.0f;
	Vector2 bpos;
	if (!_get_canvas_transforms(srect, sc, bpos)) return false;

	Size2 tex_size = (picker->texture.is_valid() ? picker->texture->get_size() : Size2(96, 96));
	if (tex_size.x <= 0 || tex_size.y <= 0) tex_size = Size2(96, 96);

	Size2 world_size = Size2(tex_size.x * (float(picker->scale_x) / 100.0f), tex_size.y * (float(picker->scale_y) / 100.0f));
	Vector2 world_tl;
	if (picker->origin_mode == ClockGearsPictureVisualPicker::ORIGIN_CENTER) {
		world_tl = Vector2(float(picker->pos_x) - world_size.x * 0.5f, float(picker->pos_y) - world_size.y * 0.5f);
	} else {
		world_tl = Vector2(float(picker->pos_x), float(picker->pos_y));
	}

	Vector2 canvas_img_pos = bpos + world_tl * sc;
	Size2 canvas_img_size = world_size * sc;
	r_img_rect = Rect2(canvas_img_pos, canvas_img_size);

	r_anchor_pt = bpos + Vector2(float(picker->pos_x), float(picker->pos_y)) * sc;

	float hs = 8.0f;
	// 4 Esquinas
	r_h_tl = Rect2(canvas_img_pos - Vector2(hs * 0.5f, hs * 0.5f), Size2(hs, hs));
	r_h_tr = Rect2(Vector2(canvas_img_pos.x + canvas_img_size.x - hs * 0.5f, canvas_img_pos.y - hs * 0.5f), Size2(hs, hs));
	r_h_bl = Rect2(Vector2(canvas_img_pos.x - hs * 0.5f, canvas_img_pos.y + canvas_img_size.y - hs * 0.5f), Size2(hs, hs));
	r_h_br = Rect2(canvas_img_pos + canvas_img_size - Vector2(hs * 0.5f, hs * 0.5f), Size2(hs, hs));

	// 4 Bordes
	r_h_t = Rect2(Vector2(canvas_img_pos.x + canvas_img_size.x * 0.5f - hs * 0.5f, canvas_img_pos.y - hs * 0.5f), Size2(hs, hs));
	r_h_b = Rect2(Vector2(canvas_img_pos.x + canvas_img_size.x * 0.5f - hs * 0.5f, canvas_img_pos.y + canvas_img_size.y - hs * 0.5f), Size2(hs, hs));
	r_h_l = Rect2(Vector2(canvas_img_pos.x - hs * 0.5f, canvas_img_pos.y + canvas_img_size.y * 0.5f - hs * 0.5f), Size2(hs, hs));
	r_h_r = Rect2(Vector2(canvas_img_pos.x + canvas_img_size.x - hs * 0.5f, canvas_img_pos.y + canvas_img_size.y * 0.5f - hs * 0.5f), Size2(hs, hs));

	return true;
}

void ClockGearsPicturePreviewCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Rect2 srect;
		float sc = 1.0f;
		Vector2 bpos;
		if (!_get_canvas_transforms(srect, sc, bpos)) return;

		// Fondo contenedor
		draw_rect(Rect2(Vector2(), get_size()), Color(0.11f, 0.12f, 0.15f, 1.0f));

		// Pantalla virtual de juego con resolución interna real
		draw_rect(srect, Color(0.06f, 0.08f, 0.11f, 1.0f));
		draw_rect(srect, Color(0.28f, 0.38f, 0.52f, 0.9f), false, 2.0f);

		// Guías centrales
		Vector2 center = bpos + srect.size * 0.5f;
		draw_line(Vector2(srect.position.x, center.y), Vector2(srect.position.x + srect.size.x, center.y), Color(0.22f, 0.28f, 0.38f, 0.5f), 1.0f);
		draw_line(Vector2(center.x, srect.position.y), Vector2(center.x, srect.position.y + srect.size.y), Color(0.22f, 0.28f, 0.38f, 0.5f), 1.0f);

		// Imagen y tiradores
		Rect2 img_rect, h_tl, h_tr, h_bl, h_br, h_t, h_b, h_l, h_r;
		Vector2 anchor_pt;
		if (_get_image_screen_rect(img_rect, h_tl, h_tr, h_bl, h_br, h_t, h_b, h_l, h_r, anchor_pt)) {
			if (picker->texture.is_valid()) {
				draw_texture_rect(picker->texture, img_rect, false);
			} else {
				draw_rect(img_rect, Color(0.20f, 0.40f, 0.65f, 0.45f));
				Ref<Font> fnt = get_theme_font("font", "Label");
				if (fnt.is_valid()) {
					draw_string(fnt, img_rect.position + Vector2(6, 18), "Imagen", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.85f, 0.92f, 1.0f));
				}
			}
			// Borde de selección
			draw_rect(img_rect, Color(0.35f, 0.75f, 1.0f, 0.9f), false, 1.5f);

			// 8 Tiradores (esquinas y bordes)
			Color h_bg(1.0f, 1.0f, 1.0f, 1.0f);
			Color h_edge(0.0f, 0.0f, 0.0f, 0.9f);
			draw_rect(h_tl, h_bg); draw_rect(h_tl, h_edge, false, 1.0f);
			draw_rect(h_tr, h_bg); draw_rect(h_tr, h_edge, false, 1.0f);
			draw_rect(h_bl, h_bg); draw_rect(h_bl, h_edge, false, 1.0f);
			draw_rect(h_br, h_bg); draw_rect(h_br, h_edge, false, 1.0f);
			draw_rect(h_t, h_bg); draw_rect(h_t, h_edge, false, 1.0f);
			draw_rect(h_b, h_bg); draw_rect(h_b, h_edge, false, 1.0f);
			draw_rect(h_l, h_bg); draw_rect(h_l, h_edge, false, 1.0f);
			draw_rect(h_r, h_bg); draw_rect(h_r, h_edge, false, 1.0f);

			// Punto de anclaje (cruz dorada)
			draw_line(anchor_pt - Vector2(6, 0), anchor_pt + Vector2(6, 0), Color(1.0f, 0.85f, 0.2f, 1.0f), 2.0f);
			draw_line(anchor_pt - Vector2(0, 6), anchor_pt + Vector2(0, 6), Color(1.0f, 0.85f, 0.2f, 1.0f), 2.0f);
		}

		// Etiqueta de resolución
		Ref<Font> fnt = get_theme_font("font", "Label");
		if (fnt.is_valid()) {
			float iw = 960.0f, ih = 540.0f;
			if (ProjectSettings::get_singleton()) {
				iw = float(ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960));
				ih = float(ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540));
			}
			draw_string(fnt, srect.position + Vector2(6, 16), vformat("%dx%d", int(iw), int(ih)), HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.55f, 0.65f, 0.75f, 0.7f));
		}
	} else if (p_what == NOTIFICATION_MOUSE_EXIT) {
		set_default_cursor_shape(CURSOR_ARROW);
	}
}

void ClockGearsPicturePreviewCanvas::gui_input(const Ref<InputEvent> &p_event) {
	if (!picker) return;

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				Rect2 img_rect, h_tl, h_tr, h_bl, h_br, h_t, h_b, h_l, h_r;
				Vector2 anchor_pt;
				if (_get_image_screen_rect(img_rect, h_tl, h_tr, h_bl, h_br, h_t, h_b, h_l, h_r, anchor_pt)) {
					Vector2 mpos = mb->get_position();
					drag_start_mouse = mpos;
					drag_start_pos = Vector2(picker->pos_x, picker->pos_y);
					drag_start_scale = Vector2(picker->scale_x, picker->scale_y);

					if (h_br.has_point(mpos)) {
						current_drag = DRAG_HANDLE_BR;
					} else if (h_tl.has_point(mpos)) {
						current_drag = DRAG_HANDLE_TL;
					} else if (h_tr.has_point(mpos)) {
						current_drag = DRAG_HANDLE_TR;
					} else if (h_bl.has_point(mpos)) {
						current_drag = DRAG_HANDLE_BL;
					} else if (h_t.has_point(mpos)) {
						current_drag = DRAG_HANDLE_T;
					} else if (h_b.has_point(mpos)) {
						current_drag = DRAG_HANDLE_B;
					} else if (h_l.has_point(mpos)) {
						current_drag = DRAG_HANDLE_L;
					} else if (h_r.has_point(mpos)) {
						current_drag = DRAG_HANDLE_R;
					} else if (img_rect.has_point(mpos)) {
						current_drag = DRAG_MOVE;
					} else {
						current_drag = DRAG_NONE;
					}
				}
			} else {
				current_drag = DRAG_NONE;
			}
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Rect2 img_rect, h_tl, h_tr, h_bl, h_br, h_t, h_b, h_l, h_r;
		Vector2 anchor_pt;
		_get_image_screen_rect(img_rect, h_tl, h_tr, h_bl, h_br, h_t, h_b, h_l, h_r, anchor_pt);
		Vector2 mpos = mm->get_position();

		if (current_drag == DRAG_NONE) {
			if (h_tl.has_point(mpos) || h_br.has_point(mpos)) {
				set_default_cursor_shape(CURSOR_FDIAGSIZE);
			} else if (h_tr.has_point(mpos) || h_bl.has_point(mpos)) {
				set_default_cursor_shape(CURSOR_BDIAGSIZE);
			} else if (h_t.has_point(mpos) || h_b.has_point(mpos)) {
				set_default_cursor_shape(CURSOR_VSIZE);
			} else if (h_l.has_point(mpos) || h_r.has_point(mpos)) {
				set_default_cursor_shape(CURSOR_HSIZE);
			} else if (img_rect.has_point(mpos)) {
				set_default_cursor_shape(CURSOR_MOVE);
			} else {
				set_default_cursor_shape(CURSOR_ARROW);
			}
			return;
		}

		Rect2 srect;
		float sc = 1.0f;
		Vector2 bpos;
		if (!_get_canvas_transforms(srect, sc, bpos)) return;

		Size2 tex_size = (picker->texture.is_valid() ? picker->texture->get_size() : Size2(96, 96));
		if (tex_size.x <= 0 || tex_size.y <= 0) tex_size = Size2(96, 96);

		if (current_drag == DRAG_MOVE) {
			Vector2 delta_screen = (mpos - drag_start_mouse) / sc;
			picker->pos_x = Math::round(drag_start_pos.x + delta_screen.x);
			picker->pos_y = Math::round(drag_start_pos.y + delta_screen.y);
			picker->_notify_values_changed();
			queue_redraw();
		} else if (current_drag == DRAG_HANDLE_T || current_drag == DRAG_HANDLE_B) {
			// Escala en un solo eje: vertical (Y)
			float delta_y = (current_drag == DRAG_HANDLE_B) ? ((mpos.y - drag_start_mouse.y) / sc) : ((drag_start_mouse.y - mpos.y) / sc);
			float start_h = tex_size.y * (drag_start_scale.y / 100.0f);
			float new_h = MAX(16.0f, start_h + (picker->origin_mode == ClockGearsPictureVisualPicker::ORIGIN_CENTER ? delta_y * 2.0f : delta_y));
			float new_sy = (new_h / tex_size.y) * 100.0f;
			if (picker->integer_snap_enabled && new_sy >= 100.0f) {
				int snap_y = int(Math::round(new_sy / 100.0f)) * 100;
				if (Math::abs(new_sy - float(snap_y)) <= 25.0f) {
					new_sy = float(snap_y);
				}
			}
			picker->scale_y = CLAMP(int(Math::round(new_sy)), 10, 2000);
			picker->_notify_values_changed();
			queue_redraw();
		} else if (current_drag == DRAG_HANDLE_L || current_drag == DRAG_HANDLE_R) {
			// Escala en un solo eje: horizontal (X)
			float delta_x = (current_drag == DRAG_HANDLE_R) ? ((mpos.x - drag_start_mouse.x) / sc) : ((drag_start_mouse.x - mpos.x) / sc);
			float start_w = tex_size.x * (drag_start_scale.x / 100.0f);
			float new_w = MAX(16.0f, start_w + (picker->origin_mode == ClockGearsPictureVisualPicker::ORIGIN_CENTER ? delta_x * 2.0f : delta_x));
			float new_sx = (new_w / tex_size.x) * 100.0f;
			if (picker->integer_snap_enabled && new_sx >= 100.0f) {
				int snap_x = int(Math::round(new_sx / 100.0f)) * 100;
				if (Math::abs(new_sx - float(snap_x)) <= 25.0f) {
					new_sx = float(snap_x);
				}
			}
			picker->scale_x = CLAMP(int(Math::round(new_sx)), 10, 2000);
			picker->_notify_values_changed();
			queue_redraw();
		} else if (current_drag >= DRAG_HANDLE_TL && current_drag <= DRAG_HANDLE_BR) {
			// Esquinas: escalado proporcional (Shift para libre)
			float delta_x = 0.0f;
			float delta_y = 0.0f;
			if (current_drag == DRAG_HANDLE_BR) {
				delta_x = (mpos.x - drag_start_mouse.x) / sc;
				delta_y = (mpos.y - drag_start_mouse.y) / sc;
			} else if (current_drag == DRAG_HANDLE_TL) {
				delta_x = (drag_start_mouse.x - mpos.x) / sc;
				delta_y = (drag_start_mouse.y - mpos.y) / sc;
			} else if (current_drag == DRAG_HANDLE_TR) {
				delta_x = (mpos.x - drag_start_mouse.x) / sc;
				delta_y = (drag_start_mouse.y - mpos.y) / sc;
			} else if (current_drag == DRAG_HANDLE_BL) {
				delta_x = (drag_start_mouse.x - mpos.x) / sc;
				delta_y = (mpos.y - drag_start_mouse.y) / sc;
			}

			float factor = (picker->origin_mode == ClockGearsPictureVisualPicker::ORIGIN_CENTER) ? 2.0f : 1.0f;
			float start_w = tex_size.x * (drag_start_scale.x / 100.0f);
			float start_h = tex_size.y * (drag_start_scale.y / 100.0f);
			float new_w = MAX(16.0f, start_w + delta_x * factor);
			float new_h = MAX(16.0f, start_h + delta_y * factor);

			float new_sx = (new_w / tex_size.x) * 100.0f;
			float new_sy = (new_h / tex_size.y) * 100.0f;

			bool shift = Input::get_singleton()->is_key_pressed(Key::SHIFT);
			if (!shift) {
				float scale_val = (Math::abs(delta_x) > Math::abs(delta_y)) ? new_sx : new_sy;
				new_sx = scale_val;
				new_sy = scale_val;
			}

			// Snap solo al ampliar (>= 100%), nunca al reducir
			if (picker->integer_snap_enabled) {
				if (new_sx >= 100.0f) {
					int snap_x = int(Math::round(new_sx / 100.0f)) * 100;
					if (Math::abs(new_sx - float(snap_x)) <= 25.0f) {
						new_sx = float(snap_x);
					}
				}
				if (new_sy >= 100.0f) {
					int snap_y = int(Math::round(new_sy / 100.0f)) * 100;
					if (Math::abs(new_sy - float(snap_y)) <= 25.0f) {
						new_sy = float(snap_y);
					}
				}
			}

			picker->scale_x = CLAMP(int(Math::round(new_sx)), 10, 2000);
			picker->scale_y = CLAMP(int(Math::round(new_sy)), 10, 2000);
			picker->_notify_values_changed();
			queue_redraw();
		}
	}
}

void ClockGearsPicturePreviewCanvas::_bind_methods() {
}

// ============================================================================
// ClockGearsPictureVisualPicker
// ============================================================================

ClockGearsPictureVisualPicker::ClockGearsPictureVisualPicker() {
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 4);

	// Barra superior: Botones rápidos de anclaje y snap
	HBoxContainer *top_bar = memnew(HBoxContainer);
	top_bar->add_theme_constant_override("separation", 6);
	add_child(top_bar);

	Label *lbl_anchor = memnew(Label(CG_TR("Anclaje rápido:")));
	top_bar->add_child(lbl_anchor);

	btn_anchor_center = memnew(Button(CG_TR("Centro")));
	btn_anchor_center->connect("pressed", callable_mp(this, &ClockGearsPictureVisualPicker::quick_anchor_center));
	top_bar->add_child(btn_anchor_center);

	btn_anchor_top = memnew(Button(CG_TR("Arriba")));
	btn_anchor_top->connect("pressed", callable_mp(this, &ClockGearsPictureVisualPicker::quick_anchor_top));
	top_bar->add_child(btn_anchor_top);

	btn_anchor_bottom = memnew(Button(CG_TR("Abajo")));
	btn_anchor_bottom->connect("pressed", callable_mp(this, &ClockGearsPictureVisualPicker::quick_anchor_bottom));
	top_bar->add_child(btn_anchor_bottom);

	btn_anchor_left = memnew(Button(CG_TR("Izquierda")));
	btn_anchor_left->connect("pressed", callable_mp(this, &ClockGearsPictureVisualPicker::quick_anchor_left));
	top_bar->add_child(btn_anchor_left);

	btn_anchor_right = memnew(Button(CG_TR("Derecha")));
	btn_anchor_right->connect("pressed", callable_mp(this, &ClockGearsPictureVisualPicker::quick_anchor_right));
	top_bar->add_child(btn_anchor_right);

	chk_integer_snap = memnew(CheckBox);
	chk_integer_snap->set_text(CG_TR("Snap Pixel Art (1x, 2x...)"));
	chk_integer_snap->set_pressed(false);
	chk_integer_snap->connect("toggled", callable_mp(this, &ClockGearsPictureVisualPicker::_on_snap_toggled));
	top_bar->add_child(chk_integer_snap);

	// Lienzo de vista previa interactiva
	canvas = memnew(ClockGearsPicturePreviewCanvas(this));
	canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(canvas);
}

void ClockGearsPictureVisualPicker::set_image_path(const String &p_path) {
	image_path = p_path;
	texture.unref();
	if (!image_path.is_empty()) {
		if (ResourceLoader::exists(image_path)) {
			texture = ResourceLoader::load(image_path);
		}
		if (texture.is_null() && FileAccess::exists(image_path)) {
			Ref<Image> img = Image::load_from_file(image_path);
			if (img.is_valid() && !img->is_empty()) {
				texture = ImageTexture::create_from_image(img);
			}
		}
	}
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsPictureVisualPicker::set_origin_mode(int p_origin) {
	origin_mode = (OriginMode)CLAMP(p_origin, 0, 1);
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsPictureVisualPicker::set_values(int p_x, int p_y, int p_sx, int p_sy) {
	if (updating_from_external) return;
	pos_x = p_x;
	pos_y = p_y;
	scale_x = p_sx;
	scale_y = p_sy;
	updating_from_external = true;
	if (bound_x_spin) bound_x_spin->set_value(pos_x);
	if (bound_y_spin) bound_y_spin->set_value(pos_y);
	if (bound_scale_x_spin) bound_scale_x_spin->set_value(scale_x);
	if (bound_scale_y_spin) bound_scale_y_spin->set_value(scale_y);
	updating_from_external = false;
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsPictureVisualPicker::set_visual_values(int p_x, int p_y, int p_sx, int p_sy) {
	pos_x = p_x;
	pos_y = p_y;
	scale_x = p_sx;
	scale_y = p_sy;
	_notify_values_changed();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsPictureVisualPicker::set_integer_snap(bool p_snap) {
	integer_snap_enabled = p_snap;
	if (chk_integer_snap) {
		chk_integer_snap->set_pressed(p_snap);
	}
}

void ClockGearsPictureVisualPicker::_on_snap_toggled(bool p_toggled) {
	integer_snap_enabled = p_toggled;
}

void ClockGearsPictureVisualPicker::quick_anchor_center() {
	int internal_w = 960, internal_h = 540;
	if (ProjectSettings::get_singleton()) {
		internal_w = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960);
		internal_h = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540);
	}
	if (internal_w <= 0) internal_w = 960;
	if (internal_h <= 0) internal_h = 540;

	Size2 tex_size = (texture.is_valid() ? texture->get_size() : Size2(96, 96));
	Size2 world_size = Size2(tex_size.x * (float(scale_x) / 100.0f), tex_size.y * (float(scale_y) / 100.0f));
	if (origin_mode == ORIGIN_CENTER) {
		pos_x = internal_w / 2;
		pos_y = internal_h / 2;
	} else {
		pos_x = (internal_w - Math::round(world_size.x)) / 2;
		pos_y = (internal_h - Math::round(world_size.y)) / 2;
	}
	_notify_values_changed();
	if (canvas) canvas->queue_redraw();
}

void ClockGearsPictureVisualPicker::quick_anchor_top() {
	int internal_w = 960, internal_h = 540;
	if (ProjectSettings::get_singleton()) {
		internal_w = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960);
		internal_h = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540);
	}
	if (internal_h <= 0) internal_h = 540;
	Size2 tex_size = (texture.is_valid() ? texture->get_size() : Size2(96, 96));
	Size2 world_size = Size2(tex_size.x * (float(scale_x) / 100.0f), tex_size.y * (float(scale_y) / 100.0f));
	if (origin_mode == ORIGIN_CENTER) {
		pos_y = Math::round(world_size.y * 0.5f);
	} else {
		pos_y = 0;
	}
	_notify_values_changed();
	if (canvas) canvas->queue_redraw();
}

void ClockGearsPictureVisualPicker::quick_anchor_bottom() {
	int internal_w = 960, internal_h = 540;
	if (ProjectSettings::get_singleton()) {
		internal_w = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960);
		internal_h = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540);
	}
	if (internal_h <= 0) internal_h = 540;
	Size2 tex_size = (texture.is_valid() ? texture->get_size() : Size2(96, 96));
	Size2 world_size = Size2(tex_size.x * (float(scale_x) / 100.0f), tex_size.y * (float(scale_y) / 100.0f));
	if (origin_mode == ORIGIN_CENTER) {
		pos_y = internal_h - Math::round(world_size.y * 0.5f);
	} else {
		pos_y = internal_h - Math::round(world_size.y);
	}
	_notify_values_changed();
	if (canvas) canvas->queue_redraw();
}

void ClockGearsPictureVisualPicker::quick_anchor_left() {
	int internal_w = 960, internal_h = 540;
	if (ProjectSettings::get_singleton()) {
		internal_w = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960);
		internal_h = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540);
	}
	if (internal_w <= 0) internal_w = 960;
	Size2 tex_size = (texture.is_valid() ? texture->get_size() : Size2(96, 96));
	Size2 world_size = Size2(tex_size.x * (float(scale_x) / 100.0f), tex_size.y * (float(scale_y) / 100.0f));
	if (origin_mode == ORIGIN_CENTER) {
		pos_x = Math::round(world_size.x * 0.5f);
	} else {
		pos_x = 0;
	}
	_notify_values_changed();
	if (canvas) canvas->queue_redraw();
}

void ClockGearsPictureVisualPicker::quick_anchor_right() {
	int internal_w = 960, internal_h = 540;
	if (ProjectSettings::get_singleton()) {
		internal_w = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_width", 960);
		internal_h = ProjectSettings::get_singleton()->get_setting("display/window/size/viewport_height", 540);
	}
	if (internal_w <= 0) internal_w = 960;
	Size2 tex_size = (texture.is_valid() ? texture->get_size() : Size2(96, 96));
	Size2 world_size = Size2(tex_size.x * (float(scale_x) / 100.0f), tex_size.y * (float(scale_y) / 100.0f));
	if (origin_mode == ORIGIN_CENTER) {
		pos_x = internal_w - Math::round(world_size.x * 0.5f);
	} else {
		pos_x = internal_w - Math::round(world_size.x);
	}
	_notify_values_changed();
	if (canvas) canvas->queue_redraw();
}

void ClockGearsPictureVisualPicker::_notify_values_changed() {
	if (updating_from_external) return;
	updating_from_external = true;
	if (bound_x_spin) bound_x_spin->set_value(pos_x);
	if (bound_y_spin) bound_y_spin->set_value(pos_y);
	if (bound_scale_x_spin) bound_scale_x_spin->set_value(scale_x);
	if (bound_scale_y_spin) bound_scale_y_spin->set_value(scale_y);
	updating_from_external = false;
	emit_signal("values_changed", pos_x, pos_y, scale_x, scale_y);
}

void ClockGearsPictureVisualPicker::_on_spinbox_changed(double) {
	if (updating_from_external) return;
	updating_from_external = true;
	if (bound_x_spin) pos_x = int(bound_x_spin->get_value());
	if (bound_y_spin) pos_y = int(bound_y_spin->get_value());
	if (bound_scale_x_spin) scale_x = int(bound_scale_x_spin->get_value());
	if (bound_scale_y_spin) scale_y = int(bound_scale_y_spin->get_value());
	updating_from_external = false;
	if (canvas) canvas->queue_redraw();
	emit_signal("values_changed", pos_x, pos_y, scale_x, scale_y);
}

void ClockGearsPictureVisualPicker::bind_spinboxes(SpinBox *p_x, SpinBox *p_y, SpinBox *p_sx, SpinBox *p_sy) {
	bound_x_spin = p_x;
	bound_y_spin = p_y;
	bound_scale_x_spin = p_sx;
	bound_scale_y_spin = p_sy;

	if (bound_x_spin) {
		bound_x_spin->connect("value_changed", callable_mp(this, &ClockGearsPictureVisualPicker::_on_spinbox_changed));
	}
	if (bound_y_spin) {
		bound_y_spin->connect("value_changed", callable_mp(this, &ClockGearsPictureVisualPicker::_on_spinbox_changed));
	}
	if (bound_scale_x_spin) {
		bound_scale_x_spin->connect("value_changed", callable_mp(this, &ClockGearsPictureVisualPicker::_on_spinbox_changed));
	}
	if (bound_scale_y_spin) {
		bound_scale_y_spin->connect("value_changed", callable_mp(this, &ClockGearsPictureVisualPicker::_on_spinbox_changed));
	}
}

void ClockGearsPictureVisualPicker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_image_path", "path"), &ClockGearsPictureVisualPicker::set_image_path);
	ClassDB::bind_method(D_METHOD("get_image_path"), &ClockGearsPictureVisualPicker::get_image_path);
	ClassDB::bind_method(D_METHOD("set_origin_mode", "origin"), &ClockGearsPictureVisualPicker::set_origin_mode);
	ClassDB::bind_method(D_METHOD("get_origin_mode"), &ClockGearsPictureVisualPicker::get_origin_mode);
	ClassDB::bind_method(D_METHOD("set_values", "x", "y", "scale_x", "scale_y"), &ClockGearsPictureVisualPicker::set_values);
	ClassDB::bind_method(D_METHOD("set_visual_values", "x", "y", "scale_x", "scale_y"), &ClockGearsPictureVisualPicker::set_visual_values);
	ClassDB::bind_method(D_METHOD("get_pos_x"), &ClockGearsPictureVisualPicker::get_pos_x);
	ClassDB::bind_method(D_METHOD("get_pos_y"), &ClockGearsPictureVisualPicker::get_pos_y);
	ClassDB::bind_method(D_METHOD("get_scale_x"), &ClockGearsPictureVisualPicker::get_scale_x);
	ClassDB::bind_method(D_METHOD("get_scale_y"), &ClockGearsPictureVisualPicker::get_scale_y);
	ClassDB::bind_method(D_METHOD("set_integer_snap", "snap"), &ClockGearsPictureVisualPicker::set_integer_snap);
	ClassDB::bind_method(D_METHOD("is_integer_snap"), &ClockGearsPictureVisualPicker::is_integer_snap);
	ClassDB::bind_method(D_METHOD("bind_spinboxes", "x", "y", "scale_x", "scale_y"), &ClockGearsPictureVisualPicker::bind_spinboxes);
	ClassDB::bind_method(D_METHOD("_on_spinbox_changed", "val"), &ClockGearsPictureVisualPicker::_on_spinbox_changed);

	ClassDB::bind_method(D_METHOD("quick_anchor_center"), &ClockGearsPictureVisualPicker::quick_anchor_center);
	ClassDB::bind_method(D_METHOD("quick_anchor_top"), &ClockGearsPictureVisualPicker::quick_anchor_top);
	ClassDB::bind_method(D_METHOD("quick_anchor_bottom"), &ClockGearsPictureVisualPicker::quick_anchor_bottom);
	ClassDB::bind_method(D_METHOD("quick_anchor_left"), &ClockGearsPictureVisualPicker::quick_anchor_left);
	ClassDB::bind_method(D_METHOD("quick_anchor_right"), &ClockGearsPictureVisualPicker::quick_anchor_right);

	ADD_SIGNAL(MethodInfo("values_changed",
			PropertyInfo(Variant::INT, "x"),
			PropertyInfo(Variant::INT, "y"),
			PropertyInfo(Variant::INT, "scale_x"),
			PropertyInfo(Variant::INT, "scale_y")));
}
