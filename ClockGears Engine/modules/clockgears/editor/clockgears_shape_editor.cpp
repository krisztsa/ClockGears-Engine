#include "clockgears_shape_editor.h"

#include "core/input/input_event.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_defs.h"
#include "core/object/callable_mp.h"
#include "scene/resources/font.h"
#ifdef TOOLS_ENABLED
#include "editor/themes/editor_scale.h"
#define CG_SE_EDSCALE EDSCALE
#else
#define CG_SE_EDSCALE 1.0f
#endif

#define CG_SE_TR(m_str) String::utf8(m_str)

static constexpr float CG_SHAPE_GRID_STEP = 32.0f;

// ============================================================================
// ClockGearsShapeCanvas
// ============================================================================

ClockGearsShapeCanvas::ClockGearsShapeCanvas(ClockGearsShapeEditor *p_owner) {
	owner_editor = p_owner;
	set_clip_contents(true);
	set_focus_mode(FOCUS_CLICK);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	set_mouse_filter(MOUSE_FILTER_STOP);
}

void ClockGearsShapeCanvas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_canvas_origin"), &ClockGearsShapeCanvas::get_canvas_origin);
	ClassDB::bind_method(D_METHOD("handle_canvas_event", "event"), &ClockGearsShapeCanvas::handle_canvas_event);
}

Size2 ClockGearsShapeCanvas::get_minimum_size() const {
	return Size2(300.0f * CG_SE_EDSCALE, 260.0f * CG_SE_EDSCALE);
}

Point2 ClockGearsShapeCanvas::get_canvas_origin() const {
	Size2 csz = get_size();
	return (csz * 0.5f).round();
}

Vector2 ClockGearsShapeCanvas::screen_to_local(const Point2 &p_screen_pos) const {
	int z = owner_editor ? MAX(1, owner_editor->get_zoom()) : 1;
	Point2 origin = get_canvas_origin();
	return (p_screen_pos - origin) / float(z);
}

Point2 ClockGearsShapeCanvas::local_to_screen(const Vector2 &p_local_pos) const {
	int z = owner_editor ? MAX(1, owner_editor->get_zoom()) : 1;
	Point2 origin = get_canvas_origin();
	return origin + (p_local_pos * float(z));
}

Ref<StyleBoxFlat> ClockGearsShapeCanvas::_make_pill_style(const Color &p_bg, const Color &p_border, int p_radius) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(true);
	sb->set_bg_color(p_bg);
	sb->set_border_width_all(1);
	sb->set_border_color(p_border);
	sb->set_corner_radius_all(p_radius);
	return sb;
}

void ClockGearsShapeCanvas::_draw_mannequin_silhouette(const Point2 &p_origin, int p_zoom, int p_size) {
	float s = float(MAX(16, p_size));
	float z = float(p_zoom);
	Rect2 box(p_origin - Vector2(s, s) * 0.5f * z, Vector2(s, s) * z);

	Color sil_fill(0.55f, 0.78f, 1.0f, 0.24f);
	Color sil_line(0.65f, 0.85f, 1.0f, 0.45f);

	// Marco sutil de referencia del tamaño del maniquí
	draw_rect(box, Color(0.55f, 0.78f, 1.0f, 0.08f), true);
	draw_rect(box, sil_line, false, 1.0f);

	// Silueta humanoide semitransparente centrada dentro de s x s
	float head_r = s * 0.16f * z;
	Point2 head_c = p_origin + Vector2(0.0f, -s * 0.26f) * z;
	draw_circle(head_c, head_r, sil_fill);
	draw_arc(head_c, head_r, 0.0f, Math::TAU, 24, sil_line, 1.5f);

	// Torso
	Rect2 torso(p_origin + Vector2(-s * 0.17f, -s * 0.09f) * z, Vector2(s * 0.34f, s * 0.30f) * z);
	draw_rect(torso, sil_fill, true);
	draw_rect(torso, sil_line, false, 1.5f);

	// Brazos
	Rect2 left_arm(p_origin + Vector2(-s * 0.28f, -s * 0.08f) * z, Vector2(s * 0.09f, s * 0.26f) * z);
	Rect2 right_arm(p_origin + Vector2(s * 0.19f, -s * 0.08f) * z, Vector2(s * 0.09f, s * 0.26f) * z);
	draw_rect(left_arm, sil_fill, true);
	draw_rect(right_arm, sil_fill, true);

	// Piernas
	Rect2 left_leg(p_origin + Vector2(-s * 0.15f, s * 0.21f) * z, Vector2(s * 0.12f, s * 0.25f) * z);
	Rect2 right_leg(p_origin + Vector2(s * 0.03f, s * 0.21f) * z, Vector2(s * 0.12f, s * 0.25f) * z);
	draw_rect(left_leg, sil_fill, true);
	draw_rect(right_leg, sil_fill, true);

	Ref<Font> font = get_theme_default_font();
	if (font.is_valid()) {
		int fsize = MAX(9, int(10 * CG_SE_EDSCALE));
		String cap = vformat("Maniqui %dx%d", p_size, p_size);
		draw_string(font, box.position + Vector2(3, -4), cap, HORIZONTAL_ALIGNMENT_LEFT, -1, fsize, Color(0.65f, 0.85f, 1.0f, 0.72f));
	}
}

void ClockGearsShapeCanvas::_draw_single_shape(const ClockGearsHitShape &p_shape, const Point2 &p_origin, int p_zoom, const Color &p_fill, const Color &p_border, const String &p_label, bool p_selected) {
	float z = float(p_zoom);
	Transform2D xform(Math::deg_to_rad(p_shape.rotation), p_shape.offset);
	auto to_scr = [&](const Vector2 &p_shape_local) -> Point2 {
		return p_origin + xform.xform(p_shape_local) * z;
	};

	Color draw_border = p_selected ? Color(1.0f, 0.95f, 0.45f, 1.0f) : p_border;
	float line_w = p_selected ? 2.5f : 1.8f;

	Point2 label_anchor = to_scr(Vector2());

	if (p_shape.type == CG_SHAPE_CIRCLE) {
		float r_scr = p_shape.get_radius() * z;
		Point2 c_scr = p_origin + p_shape.offset * z;
		draw_circle(c_scr, r_scr, p_fill);
		draw_arc(c_scr, r_scr, 0.0f, Math::TAU, 36, draw_border, line_w);
		label_anchor = c_scr + Vector2(-r_scr * 0.5f, -r_scr - 6.0f);
	} else if (p_shape.type == CG_SHAPE_RECT) {
		float hw = MAX(0.5f, p_shape.size.x * 0.5f);
		float hh = MAX(0.5f, p_shape.size.y * 0.5f);
		PackedVector2Array pts;
		pts.push_back(to_scr(Vector2(-hw, -hh)));
		pts.push_back(to_scr(Vector2(hw, -hh)));
		pts.push_back(to_scr(Vector2(hw, hh)));
		pts.push_back(to_scr(Vector2(-hw, hh)));
		draw_colored_polygon(pts, p_fill);
		PackedVector2Array closed = pts;
		closed.push_back(pts[0]);
		draw_polyline(closed, draw_border, line_w);
		label_anchor = pts[0] + Vector2(0.0f, -6.0f);
	} else if (p_shape.type == CG_SHAPE_CAPSULE) {
		float r = p_shape.get_radius();
		float h = p_shape.get_capsule_height();
		float half_seg = MAX(0.0f, h * 0.5f - r);
		PackedVector2Array pts;
		const int arc_steps = 12;
		for (int i = 0; i <= arc_steps; i++) {
			float ang = Math::PI + (Math::PI * float(i) / float(arc_steps));
			pts.push_back(to_scr(Vector2(Math::cos(ang) * r, -half_seg + Math::sin(ang) * r)));
		}
		for (int i = 0; i <= arc_steps; i++) {
			float ang = (Math::PI * float(i) / float(arc_steps));
			pts.push_back(to_scr(Vector2(Math::cos(ang) * r, half_seg + Math::sin(ang) * r)));
		}
		draw_colored_polygon(pts, p_fill);
		PackedVector2Array closed = pts;
		closed.push_back(pts[0]);
		draw_polyline(closed, draw_border, line_w);
		label_anchor = to_scr(Vector2(-r, -h * 0.5f)) + Vector2(0.0f, -6.0f);
	} else if (p_shape.type == CG_SHAPE_POLYGON) {
		if (p_shape.vertices.size() >= 3) {
			PackedVector2Array pts;
			float min_y = 1e9f;
			float min_x = 1e9f;
			for (int i = 0; i < p_shape.vertices.size(); i++) {
				Point2 sp = to_scr(p_shape.vertices[i]);
				pts.push_back(sp);
				min_x = MIN(min_x, sp.x);
				min_y = MIN(min_y, sp.y);
			}
			draw_colored_polygon(pts, p_fill);
			PackedVector2Array closed = pts;
			closed.push_back(pts[0]);
			draw_polyline(closed, draw_border, line_w);
			label_anchor = Point2(min_x, min_y - 6.0f);
		}
	}

	// Etiqueta visual (Hitbox / Hurtbox)
	Ref<Font> font = get_theme_default_font();
	if (font.is_valid() && !p_label.is_empty()) {
		int fsize = MAX(9, int(10 * CG_SE_EDSCALE));
		Size2 tsz = font->get_string_size(p_label, HORIZONTAL_ALIGNMENT_LEFT, -1, fsize);
		Rect2 pill(label_anchor + Vector2(-2.0f, -tsz.y), tsz + Vector2(8.0f, 4.0f));
		draw_style_box(_make_pill_style(Color(0.08f, 0.09f, 0.12f, 0.85f), draw_border, 4), pill);
		draw_string(font, pill.position + Vector2(4.0f, tsz.y - 1.0f), p_label, HORIZONTAL_ALIGNMENT_LEFT, -1, fsize, p_border);
	}

	// Tiradores de redimensionado / vértices si está seleccionada
	if (p_selected && owner_editor) {
		Vector<Vector2> handle_locals;
		owner_editor->_get_shape_handle_points(p_shape, handle_locals);
		float h_rad = 4.5f * CG_SE_EDSCALE;
		for (int i = 0; i < handle_locals.size(); i++) {
			Point2 hs = p_origin + handle_locals[i] * z;
			draw_circle(hs, h_rad + 1.2f, Color(0.05f, 0.05f, 0.08f, 0.95f));
			draw_circle(hs, h_rad, Color(1.0f, 0.95f, 0.45f, 1.0f));
		}
	}
}

void ClockGearsShapeCanvas::_draw_weapon_anchor(const ClockGearsWeaponAnchor &p_anchor, const Point2 &p_origin, int p_zoom, bool p_selected) {
	if (!p_anchor.enabled) {
		return;
	}
	float z = float(p_zoom);
	Point2 ap = p_origin + p_anchor.position * z;
	Color col = p_selected ? Color(1.0f, 1.0f, 0.65f, 1.0f) : Color(1.0f, 0.78f, 0.20f, 0.98f);
	float arm = 10.0f * CG_SE_EDSCALE;
	draw_line(ap + Vector2(-arm, 0), ap + Vector2(arm, 0), col, 2.0f);
	draw_line(ap + Vector2(0, -arm), ap + Vector2(0, arm), col, 2.0f);
	draw_arc(ap, 6.0f * CG_SE_EDSCALE, 0.0f, Math::TAU, 20, col, 2.0f);

	float rad = Math::deg_to_rad(p_anchor.rotation);
	Vector2 dir(Math::cos(rad), Math::sin(rad));
	Point2 tip = ap + dir * (16.0f * CG_SE_EDSCALE);
	draw_line(ap, tip, col, 2.2f);
	draw_circle(tip, 3.0f * CG_SE_EDSCALE, col);

	Ref<Font> font = get_theme_default_font();
	if (font.is_valid()) {
		int fsize = MAX(9, int(10 * CG_SE_EDSCALE));
		draw_string(font, ap + Vector2(8.0f, -8.0f), CG_SE_TR("Ancla"), HORIZONTAL_ALIGNMENT_LEFT, -1, fsize, col);
	}
}

void ClockGearsShapeCanvas::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW || !owner_editor) {
		return;
	}

	Size2 csz = get_size();
	Point2 origin = get_canvas_origin();
	int z = MAX(1, owner_editor->get_zoom());

	// 1. Fondo oscuro del lienzo
	draw_rect(Rect2(Point2(), csz), Color(0.10f, 0.11f, 0.15f, 1.0f), true);

	// 2. Rejilla opcional 32x32 en coordenadas reales del sprite
	if (owner_editor->is_grid_enabled()) {
		float step_px = CG_SHAPE_GRID_STEP * float(z);
		Color grid_col(0.27f, 0.32f, 0.42f, 0.42f);
		Color axis_col(0.45f, 0.56f, 0.74f, 0.55f);

		int nx_left = int(Math::ceil(origin.x / step_px)) + 1;
		int nx_right = int(Math::ceil((csz.x - origin.x) / step_px)) + 1;
		for (int gx = -nx_left; gx <= nx_right; gx++) {
			float sx = origin.x + float(gx) * step_px;
			draw_line(Point2(sx, 0), Point2(sx, csz.y), gx == 0 ? axis_col : grid_col, gx == 0 ? 1.5f : 1.0f);
		}
		int ny_top = int(Math::ceil(origin.y / step_px)) + 1;
		int ny_bot = int(Math::ceil((csz.y - origin.y) / step_px)) + 1;
		for (int gy = -ny_top; gy <= ny_bot; gy++) {
			float sy = origin.y + float(gy) * step_px;
			draw_line(Point2(0, sy), Point2(csz.x, sy), gy == 0 ? axis_col : grid_col, gy == 0 ? 1.5f : 1.0f);
		}
	}

	// 3. Maniquí de referencia (SOLO visual, SIEMPRE debajo del frame)
	if (owner_editor->show_mannequin) {
		_draw_mannequin_silhouette(origin, z, owner_editor->mannequin_size);
	}

	// 4. Frame del sprite
	Size2 frame_sz = Vector2(MAX(1, owner_editor->preview_region.size.x), MAX(1, owner_editor->preview_region.size.y));
	Rect2 frame_dest(origin - frame_sz * 0.5f * float(z), frame_sz * float(z));
	if (owner_editor->preview_texture.is_valid()) {
		Rect2 src_reg(owner_editor->preview_region);
		if (src_reg.size.x <= 0 || src_reg.size.y <= 0) {
			src_reg = Rect2(Point2(), owner_editor->preview_texture->get_size());
		}
		draw_texture_rect_region(owner_editor->preview_texture, frame_dest, src_reg);
	}
	draw_rect(frame_dest, Color(0.65f, 0.72f, 0.85f, 0.38f), false, 1.0f);

	// 5. Hurtbox en azul
	if (owner_editor->allow_hurtbox && owner_editor->hurtbox_data.enabled) {
		bool is_sel = (owner_editor->selected_kind == ClockGearsShapeEditor::SEL_HURTBOX);
		_draw_single_shape(
				owner_editor->hurtbox_data.shape,
				origin,
				z,
				Color(0.20f, 0.58f, 1.0f, 0.22f),
				Color(0.28f, 0.72f, 1.0f, 0.96f),
				CG_SE_TR("Hurtbox"),
				is_sel);
	}

	// 6. Hitboxes en rojo
	if (owner_editor->allow_hitboxes) {
		int hb_count = owner_editor->hitbox_data.shapes.size();
		for (int i = 0; i < hb_count; i++) {
			bool is_sel = (owner_editor->selected_kind == ClockGearsShapeEditor::SEL_HITBOX && owner_editor->selected_hitbox_idx == i);
			String lbl = (hb_count > 1) ? vformat("Hitbox #%d", i + 1) : String("Hitbox");
			_draw_single_shape(
					owner_editor->hitbox_data.shapes[i],
					origin,
					z,
					Color(0.95f, 0.22f, 0.22f, 0.26f),
					Color(1.0f, 0.30f, 0.30f, 0.98f),
					lbl,
					is_sel);
		}
	}

	// 7. Vista previa durante creación de forma o polígono en curso
	if (owner_editor->create_drag_active) {
		bool as_hurt = (owner_editor->current_target == ClockGearsShapeEditor::TARGET_HURTBOX || !owner_editor->allow_hitboxes);
		Color dfill = as_hurt ? Color(0.20f, 0.58f, 1.0f, 0.22f) : Color(0.95f, 0.22f, 0.22f, 0.26f);
		Color dbord = as_hurt ? Color(0.35f, 0.78f, 1.0f, 1.0f) : Color(1.0f, 0.40f, 0.40f, 1.0f);
		_draw_single_shape(owner_editor->create_draft_shape, origin, z, dfill, dbord, "", false);
	}

	if (!owner_editor->poly_draft_vertices.is_empty()) {
		bool as_hurt = (owner_editor->current_target == ClockGearsShapeEditor::TARGET_HURTBOX || !owner_editor->allow_hitboxes);
		Color pcol = as_hurt ? Color(0.35f, 0.78f, 1.0f, 1.0f) : Color(1.0f, 0.40f, 0.40f, 1.0f);
		PackedVector2Array scr_pts;
		for (int i = 0; i < owner_editor->poly_draft_vertices.size(); i++) {
			Point2 sp = origin + owner_editor->poly_draft_vertices[i] * float(z);
			scr_pts.push_back(sp);
			draw_circle(sp, (i == 0 ? 5.5f : 4.0f) * CG_SE_EDSCALE, pcol);
		}
		if (owner_editor->poly_hover_valid) {
			scr_pts.push_back(origin + owner_editor->poly_hover_local * float(z));
		}
		if (scr_pts.size() >= 2) {
			draw_polyline(scr_pts, pcol, 2.0f);
		}
	}

	// 8. Ancla de arma
	if (owner_editor->allow_weapon_anchor && owner_editor->weapon_anchor_data.enabled) {
		bool is_sel = (owner_editor->selected_kind == ClockGearsShapeEditor::SEL_ANCHOR);
		_draw_weapon_anchor(owner_editor->weapon_anchor_data, origin, z, is_sel);
	}
}

void ClockGearsShapeCanvas::gui_input(const Ref<InputEvent> &p_event) {
	handle_canvas_event(p_event);
}

void ClockGearsShapeCanvas::handle_canvas_event(const Ref<InputEvent> &p_event) {
	if (!owner_editor) {
		return;
	}

	Ref<InputEventKey> ke = p_event;
	if (ke.is_valid() && ke->is_pressed() && !ke->is_echo()) {
		Key k = ke->get_keycode();
		if (k == Key::KEY_DELETE || k == Key::BACKSPACE) {
			if (owner_editor->delete_selected_shape()) {
				accept_event();
			}
			return;
		}
		if ((k == Key::ENTER || k == Key::KP_ENTER) && owner_editor->current_tool == ClockGearsShapeEditor::TOOL_POLYGON) {
			if (owner_editor->finish_polygon()) {
				accept_event();
			}
			return;
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		MouseButton btn = mb->get_button_index();
		// REGLA EXPLÍCITA: la rueda del ratón NUNCA hace zoom en el editor de formas
		if (btn == MouseButton::WHEEL_UP || btn == MouseButton::WHEEL_DOWN ||
				btn == MouseButton::WHEEL_LEFT || btn == MouseButton::WHEEL_RIGHT) {
			return;
		}

		Point2 scr = mb->get_position();
		Vector2 raw_local = screen_to_local(scr);
		Vector2 snapped_local = owner_editor->_maybe_snap_local(raw_local);
		Point2 origin = get_canvas_origin();

		if (btn == MouseButton::RIGHT && mb->is_pressed()) {
			if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_POLYGON) {
				if (owner_editor->poly_draft_vertices.size() >= 3) {
					owner_editor->finish_polygon();
				} else {
					owner_editor->poly_draft_vertices.clear();
					queue_redraw();
				}
				accept_event();
				return;
			}
			owner_editor->selected_kind = ClockGearsShapeEditor::SEL_NONE;
			owner_editor->selected_hitbox_idx = -1;
			owner_editor->_sync_selected_shape_controls();
			queue_redraw();
			accept_event();
			return;
		}

		if (btn == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				grab_focus();
				ClockGearsShapeEditor::ToolMode ed_tool = owner_editor->current_tool;

				if (ed_tool == ClockGearsShapeEditor::TOOL_POLYGON) {
					if (owner_editor->poly_draft_vertices.size() >= 3) {
						Point2 first_scr = local_to_screen(owner_editor->poly_draft_vertices[0]);
						if (mb->is_double_click() || scr.distance_to(first_scr) <= 10.0f * CG_SE_EDSCALE) {
							owner_editor->finish_polygon();
							accept_event();
							return;
						}
					}
					owner_editor->poly_draft_vertices.push_back(snapped_local);
					owner_editor->poly_hover_local = snapped_local;
					owner_editor->poly_hover_valid = true;
					owner_editor->_update_toolbar_states();
					queue_redraw();
					accept_event();
					return;
				}

				if (ed_tool == ClockGearsShapeEditor::TOOL_CIRCLE ||
						ed_tool == ClockGearsShapeEditor::TOOL_RECT ||
						ed_tool == ClockGearsShapeEditor::TOOL_CAPSULE) {
					owner_editor->drag_kind = ClockGearsShapeEditor::DRAG_CREATE_SHAPE;
					owner_editor->drag_start_local = snapped_local;
					owner_editor->drag_press_screen = scr;
					owner_editor->create_drag_active = true;
					if (ed_tool == ClockGearsShapeEditor::TOOL_CIRCLE) {
						owner_editor->create_draft_shape = ClockGearsHitShape::make_circle(12.0f, snapped_local);
					} else if (ed_tool == ClockGearsShapeEditor::TOOL_RECT) {
						owner_editor->create_draft_shape = ClockGearsHitShape::make_rect(Vector2(24.0f, 24.0f), snapped_local);
					} else {
						owner_editor->create_draft_shape = ClockGearsHitShape::make_capsule(10.0f, 28.0f, snapped_local);
					}
					queue_redraw();
					accept_event();
					return;
				}

				// TOOL_SELECT: 1) Ancla de arma si está habilitada y se pulsa cerca
				if (owner_editor->allow_weapon_anchor && owner_editor->weapon_anchor_data.enabled) {
					Point2 anc_scr = local_to_screen(owner_editor->weapon_anchor_data.position);
					if (scr.distance_to(anc_scr) <= 12.0f * CG_SE_EDSCALE) {
						owner_editor->selected_kind = ClockGearsShapeEditor::SEL_ANCHOR;
						owner_editor->selected_hitbox_idx = -1;
						owner_editor->drag_kind = ClockGearsShapeEditor::DRAG_MOVE_ANCHOR;
						owner_editor->drag_start_local = raw_local;
						owner_editor->drag_initial_anchor_pos = owner_editor->weapon_anchor_data.position;
						owner_editor->_sync_selected_shape_controls();
						owner_editor->_update_toolbar_states();
						queue_redraw();
						accept_event();
						return;
					}
				}

				// 2) Tiradores de redimensionado o vértices de la forma ya seleccionada
				if (const ClockGearsHitShape *sel_sh = owner_editor->_get_const_selected_shape()) {
					int h_idx = owner_editor->_hit_test_handles(*sel_sh, scr, origin);
					if (h_idx >= 0) {
						owner_editor->drag_kind = (sel_sh->type == CG_SHAPE_POLYGON)
								? ClockGearsShapeEditor::DRAG_MOVE_VERTEX
								: ClockGearsShapeEditor::DRAG_RESIZE_HANDLE;
						owner_editor->drag_handle_id = h_idx;
						owner_editor->drag_start_local = raw_local;
						owner_editor->drag_initial_shape = *sel_sh;
						accept_event();
						return;
					}
				}

				// 3) Hitbox shapes (de arriba abajo)
				if (owner_editor->allow_hitboxes) {
					for (int i = owner_editor->hitbox_data.shapes.size() - 1; i >= 0; i--) {
						if (owner_editor->_hit_test_shape(owner_editor->hitbox_data.shapes[i], raw_local)) {
							owner_editor->selected_kind = ClockGearsShapeEditor::SEL_HITBOX;
							owner_editor->selected_hitbox_idx = i;
							owner_editor->drag_kind = ClockGearsShapeEditor::DRAG_MOVE_SHAPE;
							owner_editor->drag_start_local = raw_local;
							owner_editor->drag_initial_shape = owner_editor->hitbox_data.shapes[i];
							owner_editor->_sync_selected_shape_controls();
							owner_editor->_update_toolbar_states();
							queue_redraw();
							accept_event();
							return;
						}
					}
				}

				// 4) Hurtbox
				if (owner_editor->allow_hurtbox && owner_editor->hurtbox_data.enabled &&
						owner_editor->_hit_test_shape(owner_editor->hurtbox_data.shape, raw_local)) {
					owner_editor->selected_kind = ClockGearsShapeEditor::SEL_HURTBOX;
					owner_editor->selected_hitbox_idx = -1;
					owner_editor->drag_kind = ClockGearsShapeEditor::DRAG_MOVE_SHAPE;
					owner_editor->drag_start_local = raw_local;
					owner_editor->drag_initial_shape = owner_editor->hurtbox_data.shape;
					owner_editor->_sync_selected_shape_controls();
					owner_editor->_update_toolbar_states();
					queue_redraw();
					accept_event();
					return;
				}

				// Nada debajo -> limpiar selección
				owner_editor->selected_kind = ClockGearsShapeEditor::SEL_NONE;
				owner_editor->selected_hitbox_idx = -1;
				owner_editor->_sync_selected_shape_controls();
				owner_editor->_update_toolbar_states();
				queue_redraw();
				accept_event();
				return;
			} else {
				// Mouse release
				if (owner_editor->drag_kind == ClockGearsShapeEditor::DRAG_CREATE_SHAPE) {
					owner_editor->drag_kind = ClockGearsShapeEditor::DRAG_NONE;
					owner_editor->create_drag_active = false;
					ClockGearsHitShape final_sh = owner_editor->create_draft_shape;
					if (scr.distance_to(owner_editor->drag_press_screen) < 3.0f * CG_SE_EDSCALE) {
						if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_CIRCLE) {
							final_sh = ClockGearsHitShape::make_circle(12.0f, owner_editor->drag_start_local);
						} else if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_RECT) {
							final_sh = ClockGearsHitShape::make_rect(Vector2(24.0f, 24.0f), owner_editor->drag_start_local);
						} else if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_CAPSULE) {
							final_sh = ClockGearsHitShape::make_capsule(10.0f, 28.0f, owner_editor->drag_start_local);
						}
					}
					owner_editor->_commit_created_shape(final_sh);
					owner_editor->set_tool(ClockGearsShapeEditor::TOOL_SELECT);
					accept_event();
					return;
				}

				if (owner_editor->drag_kind != ClockGearsShapeEditor::DRAG_NONE) {
					owner_editor->drag_kind = ClockGearsShapeEditor::DRAG_NONE;
					owner_editor->drag_handle_id = -1;
					owner_editor->_sync_selected_shape_controls();
					owner_editor->_emit_shape_changed();
					queue_redraw();
					accept_event();
					return;
				}
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Point2 scr = mm->get_position();
		Vector2 raw_local = screen_to_local(scr);
		Vector2 snapped_local = owner_editor->_maybe_snap_local(raw_local);

		if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_POLYGON && !owner_editor->poly_draft_vertices.is_empty()) {
			owner_editor->poly_hover_local = snapped_local;
			owner_editor->poly_hover_valid = true;
			queue_redraw();
		}

		if (owner_editor->drag_kind == ClockGearsShapeEditor::DRAG_CREATE_SHAPE) {
			Vector2 delta = snapped_local - owner_editor->drag_start_local;
			if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_CIRCLE) {
				float rad = MAX(4.0f, Math::round(delta.length()));
				owner_editor->create_draft_shape = ClockGearsHitShape::make_circle(rad, owner_editor->drag_start_local);
			} else if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_RECT) {
				Vector2 center = (owner_editor->drag_start_local + snapped_local) * 0.5f;
				Vector2 rsz(MAX(4.0f, Math::abs(delta.x)), MAX(4.0f, Math::abs(delta.y)));
				owner_editor->create_draft_shape = ClockGearsHitShape::make_rect(rsz, center);
			} else if (owner_editor->current_tool == ClockGearsShapeEditor::TOOL_CAPSULE) {
				float r = MAX(3.0f, Math::abs(delta.x));
				float h = MAX(r * 2.0f, Math::abs(delta.y) * 2.0f);
				owner_editor->create_draft_shape = ClockGearsHitShape::make_capsule(r, h, owner_editor->drag_start_local);
			}
			queue_redraw();
			accept_event();
			return;
		}

		if (owner_editor->drag_kind == ClockGearsShapeEditor::DRAG_MOVE_ANCHOR) {
			Vector2 delta = raw_local - owner_editor->drag_start_local;
			owner_editor->weapon_anchor_data.position = owner_editor->_maybe_snap_local(owner_editor->drag_initial_anchor_pos + delta);
			owner_editor->_sync_inspector_from_data();
			queue_redraw();
			accept_event();
			return;
		}

		ClockGearsHitShape *mut_sh = owner_editor->_get_mutable_selected_shape();
		if (mut_sh && owner_editor->drag_kind == ClockGearsShapeEditor::DRAG_MOVE_SHAPE) {
			Vector2 delta = raw_local - owner_editor->drag_start_local;
			mut_sh->offset = owner_editor->_maybe_snap_local(owner_editor->drag_initial_shape.offset + delta);
			owner_editor->_sync_selected_shape_controls();
			queue_redraw();
			accept_event();
			return;
		}

		if (mut_sh && (owner_editor->drag_kind == ClockGearsShapeEditor::DRAG_RESIZE_HANDLE ||
							  owner_editor->drag_kind == ClockGearsShapeEditor::DRAG_MOVE_VERTEX)) {
			Vector2 target_pt = owner_editor->snap_enabled ? snapped_local : raw_local;
			Transform2D inv_rot(Math::deg_to_rad(-mut_sh->rotation), Vector2());
			Vector2 in_sh = inv_rot.xform(target_pt - mut_sh->offset);

			if (mut_sh->type == CG_SHAPE_CIRCLE) {
				float new_r = MAX(2.0f, Math::round((target_pt - mut_sh->offset).length()));
				mut_sh->size = Vector2(new_r, new_r);
			} else if (mut_sh->type == CG_SHAPE_RECT) {
				int hid = owner_editor->drag_handle_id;
				if (hid == 4) {
					mut_sh->size.x = MAX(4.0f, Math::abs(in_sh.x) * 2.0f);
				} else if (hid == 5) {
					mut_sh->size.y = MAX(4.0f, Math::abs(in_sh.y) * 2.0f);
				} else {
					mut_sh->size.x = MAX(4.0f, Math::abs(in_sh.x) * 2.0f);
					mut_sh->size.y = MAX(4.0f, Math::abs(in_sh.y) * 2.0f);
				}
			} else if (mut_sh->type == CG_SHAPE_CAPSULE) {
				int hid = owner_editor->drag_handle_id;
				if (hid == 0) {
					float new_r = MAX(2.0f, Math::abs(in_sh.x));
					mut_sh->size.x = new_r;
					mut_sh->size.y = MAX(new_r * 2.0f, mut_sh->size.y);
				} else {
					float r = mut_sh->get_radius();
					mut_sh->size.y = MAX(r * 2.0f, Math::abs(in_sh.y) * 2.0f);
				}
			} else if (mut_sh->type == CG_SHAPE_POLYGON) {
				int vid = owner_editor->drag_handle_id;
				if (vid >= 0 && vid < mut_sh->vertices.size()) {
					mut_sh->vertices.write[vid] = in_sh;
				}
			}
			owner_editor->_sync_selected_shape_controls();
			queue_redraw();
			accept_event();
			return;
		}
	}
}

// ============================================================================
// ClockGearsShapeEditor
// ============================================================================

Ref<StyleBoxFlat> ClockGearsShapeEditor::_make_rounded_btn_style(const Color &p_bg, const Color &p_border, int p_radius) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(true);
	sb->set_bg_color(p_bg);
	sb->set_border_width_all(1);
	sb->set_border_color(p_border);
	sb->set_corner_radius_all(p_radius);
	sb->set_content_margin(SIDE_LEFT, 8.0f * CG_SE_EDSCALE);
	sb->set_content_margin(SIDE_RIGHT, 8.0f * CG_SE_EDSCALE);
	sb->set_content_margin(SIDE_TOP, 4.0f * CG_SE_EDSCALE);
	sb->set_content_margin(SIDE_BOTTOM, 4.0f * CG_SE_EDSCALE);
	return sb;
}

void ClockGearsShapeEditor::_apply_rounded_button_styles(Button *p_btn, bool p_active, const Color &p_accent) {
	if (!p_btn) {
		return;
	}
	Color bg_norm = p_active ? Color(p_accent.r, p_accent.g, p_accent.b, 0.34f) : Color(0.18f, 0.21f, 0.28f, 0.92f);
	Color bd_norm = p_active ? p_accent : Color(0.32f, 0.37f, 0.48f, 0.85f);
	Color bg_hov = p_active ? Color(p_accent.r, p_accent.g, p_accent.b, 0.48f) : Color(0.24f, 0.28f, 0.36f, 0.95f);
	Color bg_prs = Color(p_accent.r, p_accent.g, p_accent.b, 0.58f);

	p_btn->add_theme_style_override("normal", _make_rounded_btn_style(bg_norm, bd_norm, 8));
	p_btn->add_theme_style_override("hover", _make_rounded_btn_style(bg_hov, p_accent, 8));
	p_btn->add_theme_style_override("pressed", _make_rounded_btn_style(bg_prs, p_accent, 8));
	p_btn->add_theme_style_override("focus", _make_rounded_btn_style(bg_norm, p_accent, 8));
	p_btn->add_theme_color_override("font_color", p_active ? Color(1.0f, 1.0f, 1.0f) : Color(0.88f, 0.90f, 0.95f));
}

void ClockGearsShapeEditor::_apply_spinbox_round_style(SpinBox *p_sb) {
	if (!p_sb) {
		return;
	}
	p_sb->set_custom_minimum_size(Size2(76.0f * CG_SE_EDSCALE, 28.0f * CG_SE_EDSCALE));
	if (LineEdit *le = p_sb->get_line_edit()) {
		Ref<StyleBoxFlat> sb = _make_rounded_btn_style(Color(0.13f, 0.15f, 0.20f, 0.96f), Color(0.32f, 0.37f, 0.48f, 0.85f), 6);
		sb->set_content_margin(SIDE_LEFT, 6.0f * CG_SE_EDSCALE);
		sb->set_content_margin(SIDE_RIGHT, 6.0f * CG_SE_EDSCALE);
		le->add_theme_style_override("normal", sb);
		le->add_theme_style_override("focus", _make_rounded_btn_style(Color(0.14f, 0.16f, 0.22f, 1.0f), Color(0.35f, 0.62f, 0.95f, 1.0f), 6));
	}
}

void ClockGearsShapeEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_preview_texture", "texture", "region"), &ClockGearsShapeEditor::set_preview_texture);
	ClassDB::bind_method(D_METHOD("set_mannequin", "show", "size"), &ClockGearsShapeEditor::set_mannequin);
	ClassDB::bind_method(D_METHOD("is_mannequin_shown"), &ClockGearsShapeEditor::is_mannequin_shown);
	ClassDB::bind_method(D_METHOD("get_mannequin_size"), &ClockGearsShapeEditor::get_mannequin_size);

	ClassDB::bind_method(D_METHOD("set_feature_flags", "allow_hitboxes", "allow_hurtbox", "allow_weapon_anchor"), &ClockGearsShapeEditor::set_feature_flags);
	ClassDB::bind_method(D_METHOD("set_shape_data", "hitbox_dict", "hurtbox_dict", "has_hurtbox_override", "weapon_anchor_dict"), &ClockGearsShapeEditor::set_shape_data);
	ClassDB::bind_method(D_METHOD("get_hitbox_data"), &ClockGearsShapeEditor::get_hitbox_data);
	ClassDB::bind_method(D_METHOD("get_hurtbox_data"), &ClockGearsShapeEditor::get_hurtbox_data);
	ClassDB::bind_method(D_METHOD("has_hurtbox_override_enabled"), &ClockGearsShapeEditor::has_hurtbox_override_enabled);
	ClassDB::bind_method(D_METHOD("get_weapon_anchor_data"), &ClockGearsShapeEditor::get_weapon_anchor_data);

	ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &ClockGearsShapeEditor::set_zoom);
	ClassDB::bind_method(D_METHOD("get_zoom"), &ClockGearsShapeEditor::get_zoom);
	ClassDB::bind_method(D_METHOD("set_grid_enabled", "enabled"), &ClockGearsShapeEditor::set_grid_enabled);
	ClassDB::bind_method(D_METHOD("is_grid_enabled"), &ClockGearsShapeEditor::is_grid_enabled);
	ClassDB::bind_method(D_METHOD("set_snap_enabled", "enabled"), &ClockGearsShapeEditor::set_snap_enabled);
	ClassDB::bind_method(D_METHOD("is_snap_enabled"), &ClockGearsShapeEditor::is_snap_enabled);

	ClassDB::bind_method(D_METHOD("set_edit_target", "target"), &ClockGearsShapeEditor::set_edit_target);
	ClassDB::bind_method(D_METHOD("get_edit_target"), &ClockGearsShapeEditor::get_edit_target);
	ClassDB::bind_method(D_METHOD("set_tool", "tool"), &ClockGearsShapeEditor::set_tool);
	ClassDB::bind_method(D_METHOD("get_tool"), &ClockGearsShapeEditor::get_tool);

	ClassDB::bind_method(D_METHOD("add_hitbox_shape", "shape_dict"), &ClockGearsShapeEditor::add_hitbox_shape);
	ClassDB::bind_method(D_METHOD("set_hurtbox_shape", "shape_dict"), &ClockGearsShapeEditor::set_hurtbox_shape);
	ClassDB::bind_method(D_METHOD("finish_polygon"), &ClockGearsShapeEditor::finish_polygon);
	ClassDB::bind_method(D_METHOD("delete_selected_shape"), &ClockGearsShapeEditor::delete_selected_shape);
	ClassDB::bind_method(D_METHOD("select_shape", "kind", "index"), &ClockGearsShapeEditor::select_shape, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_selected_kind"), &ClockGearsShapeEditor::get_selected_kind);
	ClassDB::bind_method(D_METHOD("get_selected_index"), &ClockGearsShapeEditor::get_selected_index);
	ClassDB::bind_method(D_METHOD("get_shape_canvas"), &ClockGearsShapeEditor::get_shape_canvas);

	ADD_SIGNAL(MethodInfo("shape_data_changed"));
}

void ClockGearsShapeEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		_update_toolbar_states();
		_sync_inspector_from_data();
	}
}

ClockGearsShapeEditor::ClockGearsShapeEditor() {
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", int(8.0f * CG_SE_EDSCALE));

	const float btn_h = 30.0f * CG_SE_EDSCALE;

	// Barra 1: Zoom entero x1..x8 + Rejilla 32x32 + Imán
	top_bar = memnew(HFlowContainer);
	top_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_bar->add_theme_constant_override("h_separation", int(6.0f * CG_SE_EDSCALE));
	top_bar->add_theme_constant_override("v_separation", int(6.0f * CG_SE_EDSCALE));
	add_child(top_bar);

	Label *lbl_zoom = memnew(Label(CG_SE_TR("Zoom:")));
	top_bar->add_child(lbl_zoom);

	for (int i = 0; i < 8; i++) {
		int z_val = i + 1;
		Button *zb = memnew(Button(vformat("x%d", z_val)));
		zb->set_custom_minimum_size(Size2(34.0f * CG_SE_EDSCALE, btn_h));
		zb->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_zoom_btn_pressed).bind(z_val));
		top_bar->add_child(zb);
		zoom_btns[i] = zb;
	}

	btn_grid = memnew(Button(CG_SE_TR("Rejilla 32x32")));
	btn_grid->set_toggle_mode(true);
	btn_grid->set_pressed(grid_enabled);
	btn_grid->set_custom_minimum_size(Size2(104.0f * CG_SE_EDSCALE, btn_h));
	btn_grid->connect("toggled", callable_mp(this, &ClockGearsShapeEditor::_on_grid_toggled));
	top_bar->add_child(btn_grid);

	btn_snap = memnew(Button(CG_SE_TR("Imán")));
	btn_snap->set_toggle_mode(true);
	btn_snap->set_pressed(snap_enabled);
	btn_snap->set_custom_minimum_size(Size2(68.0f * CG_SE_EDSCALE, btn_h));
	btn_snap->connect("toggled", callable_mp(this, &ClockGearsShapeEditor::_on_snap_toggled));
	top_bar->add_child(btn_snap);

	// Barra 2: Destino (Hitbox/Hurtbox) + Herramientas de forma
	tools_bar = memnew(HFlowContainer);
	tools_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools_bar->add_theme_constant_override("h_separation", int(6.0f * CG_SE_EDSCALE));
	tools_bar->add_theme_constant_override("v_separation", int(6.0f * CG_SE_EDSCALE));
	add_child(tools_bar);

	Label *lbl_tgt = memnew(Label(CG_SE_TR("Capa:")));
	tools_bar->add_child(lbl_tgt);

	btn_target_hitbox = memnew(Button(CG_SE_TR("Hitbox (Rojo)")));
	btn_target_hitbox->set_custom_minimum_size(Size2(98.0f * CG_SE_EDSCALE, btn_h));
	btn_target_hitbox->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_target_btn_pressed).bind(TARGET_HITBOX));
	tools_bar->add_child(btn_target_hitbox);

	btn_target_hurtbox = memnew(Button(CG_SE_TR("Hurtbox (Azul)")));
	btn_target_hurtbox->set_custom_minimum_size(Size2(104.0f * CG_SE_EDSCALE, btn_h));
	btn_target_hurtbox->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_target_btn_pressed).bind(TARGET_HURTBOX));
	tools_bar->add_child(btn_target_hurtbox);

	btn_tool_select = memnew(Button(CG_SE_TR("Mover/Editar")));
	btn_tool_select->set_custom_minimum_size(Size2(96.0f * CG_SE_EDSCALE, btn_h));
	btn_tool_select->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_tool_btn_pressed).bind(TOOL_SELECT));
	tools_bar->add_child(btn_tool_select);

	btn_tool_circle = memnew(Button(CG_SE_TR("+ Círculo")));
	btn_tool_circle->set_custom_minimum_size(Size2(78.0f * CG_SE_EDSCALE, btn_h));
	btn_tool_circle->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_tool_btn_pressed).bind(TOOL_CIRCLE));
	tools_bar->add_child(btn_tool_circle);

	btn_tool_rect = memnew(Button(CG_SE_TR("+ Rectángulo")));
	btn_tool_rect->set_custom_minimum_size(Size2(96.0f * CG_SE_EDSCALE, btn_h));
	btn_tool_rect->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_tool_btn_pressed).bind(TOOL_RECT));
	tools_bar->add_child(btn_tool_rect);

	btn_tool_capsule = memnew(Button(CG_SE_TR("+ Cápsula")));
	btn_tool_capsule->set_custom_minimum_size(Size2(84.0f * CG_SE_EDSCALE, btn_h));
	btn_tool_capsule->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_tool_btn_pressed).bind(TOOL_CAPSULE));
	tools_bar->add_child(btn_tool_capsule);

	btn_tool_polygon = memnew(Button(CG_SE_TR("+ Polígono")));
	btn_tool_polygon->set_custom_minimum_size(Size2(86.0f * CG_SE_EDSCALE, btn_h));
	btn_tool_polygon->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_tool_btn_pressed).bind(TOOL_POLYGON));
	tools_bar->add_child(btn_tool_polygon);

	btn_finish_polygon = memnew(Button(CG_SE_TR("Cerrar polígono")));
	btn_finish_polygon->set_custom_minimum_size(Size2(108.0f * CG_SE_EDSCALE, btn_h));
	btn_finish_polygon->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_finish_polygon_pressed));
	tools_bar->add_child(btn_finish_polygon);

	btn_delete_shape = memnew(Button(CG_SE_TR("Borrar")));
	btn_delete_shape->set_custom_minimum_size(Size2(68.0f * CG_SE_EDSCALE, btn_h));
	btn_delete_shape->connect("pressed", callable_mp(this, &ClockGearsShapeEditor::_on_delete_shape_pressed));
	tools_bar->add_child(btn_delete_shape);

	// Workspace: Lienzo a la izquierda + Propiedades a la derecha
	workspace_hb = memnew(HBoxContainer);
	workspace_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	workspace_hb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	workspace_hb->add_theme_constant_override("separation", int(10.0f * CG_SE_EDSCALE));
	add_child(workspace_hb);

	canvas_frame = memnew(PanelContainer);
	canvas_frame->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_frame->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_frame->set_custom_minimum_size(Size2(320.0f * CG_SE_EDSCALE, 280.0f * CG_SE_EDSCALE));
	Ref<StyleBoxFlat> cf_sb = _make_rounded_btn_style(Color(0.09f, 0.10f, 0.14f, 1.0f), Color(0.30f, 0.35f, 0.45f, 0.9f), 8);
	cf_sb->set_content_margin_all(2.0f * CG_SE_EDSCALE);
	canvas_frame->add_theme_style_override("panel", cf_sb);
	workspace_hb->add_child(canvas_frame);

	canvas = memnew(ClockGearsShapeCanvas(this));
	canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_frame->add_child(canvas);

	// Panel Inspector derecho (ancho compacto pero cómodo, sin scroll horizontal)
	inspector_panel = memnew(PanelContainer);
	inspector_panel->set_custom_minimum_size(Size2(290.0f * CG_SE_EDSCALE, 0));
	inspector_panel->set_h_size_flags(Control::SIZE_FILL);
	inspector_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	Ref<StyleBoxFlat> ins_sb = _make_rounded_btn_style(Color(0.13f, 0.15f, 0.20f, 0.95f), Color(0.28f, 0.33f, 0.44f, 0.85f), 8);
	ins_sb->set_content_margin_all(10.0f * CG_SE_EDSCALE);
	inspector_panel->add_theme_style_override("panel", ins_sb);
	workspace_hb->add_child(inspector_panel);

	inspector_vbox = memnew(VBoxContainer);
	inspector_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	inspector_vbox->add_theme_constant_override("separation", int(8.0f * CG_SE_EDSCALE));
	inspector_panel->add_child(inspector_vbox);

	auto make_slider_spin_row = [&](VBoxContainer *p_parent, const String &p_title, double p_smin, double p_smax, double p_spin_max, double p_step, HSlider *&r_sl, SpinBox *&r_sp) {
		VBoxContainer *pair = memnew(VBoxContainer);
		pair->add_theme_constant_override("separation", 2);
		p_parent->add_child(pair);

		HBoxContainer *top = memnew(HBoxContainer);
		top->add_theme_constant_override("separation", int(6.0f * CG_SE_EDSCALE));
		pair->add_child(top);

		Label *lbl = memnew(Label(p_title));
		lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		top->add_child(lbl);

		r_sp = memnew(SpinBox);
		r_sp->set_min(p_smin);
		r_sp->set_max(p_spin_max);
		r_sp->set_step(1.0);
		_apply_spinbox_round_style(r_sp);
		top->add_child(r_sp);

		r_sl = memnew(HSlider);
		r_sl->set_min(p_smin);
		r_sl->set_max(p_smax);
		r_sl->set_step(p_step);
		r_sl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		pair->add_child(r_sl);
	};

	// Sección Propiedades de Hitbox del frame
	hitbox_sec = memnew(VBoxContainer);
	hitbox_sec->add_theme_constant_override("separation", int(6.0f * CG_SE_EDSCALE));
	inspector_vbox->add_child(hitbox_sec);

	Label *lbl_hb_hdr = memnew(Label(CG_SE_TR("Hitbox del frame (Rojo)")));
	lbl_hb_hdr->add_theme_color_override("font_color", Color(1.0f, 0.45f, 0.45f));
	hitbox_sec->add_child(lbl_hb_hdr);

	make_slider_spin_row(hitbox_sec, CG_SE_TR("Daño (%)"), 0, 500, 1000, 5, hb_dmg_slider, hb_dmg_spin);
	hb_dmg_slider->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hb_dmg_slider_changed));
	hb_dmg_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hb_dmg_spin_changed));

	make_slider_spin_row(hitbox_sec, CG_SE_TR("Retroceso (Knockback)"), 0, 400, 2000, 5, hb_kb_slider, hb_kb_spin);
	hb_kb_slider->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hb_kb_slider_changed));
	hb_kb_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hb_kb_spin_changed));

	make_slider_spin_row(hitbox_sec, CG_SE_TR("Hitstop (ms)"), 0, 500, 2000, 5, hb_hitstop_slider, hb_hitstop_spin);
	hb_hitstop_slider->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hb_hitstop_slider_changed));
	hb_hitstop_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hb_hitstop_spin_changed));

	// Sección Hurtbox
	hurtbox_sec = memnew(VBoxContainer);
	hurtbox_sec->add_theme_constant_override("separation", int(6.0f * CG_SE_EDSCALE));
	inspector_vbox->add_child(hurtbox_sec);
	hurtbox_sec->add_child(memnew(HSeparator));

	Label *lbl_hu_hdr = memnew(Label(CG_SE_TR("Hurtbox (Azul)")));
	lbl_hu_hdr->add_theme_color_override("font_color", Color(0.40f, 0.78f, 1.0f));
	hurtbox_sec->add_child(lbl_hu_hdr);

	chk_hurtbox_override = memnew(CheckBox(CG_SE_TR("Anular hurtbox en este frame")));
	chk_hurtbox_override->connect("toggled", callable_mp(this, &ClockGearsShapeEditor::_on_hurtbox_override_toggled));
	hurtbox_sec->add_child(chk_hurtbox_override);

	lbl_hurtbox_mode = memnew(Label(CG_SE_TR("Usando hurtbox compartida del sprite")));
	lbl_hurtbox_mode->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	lbl_hurtbox_mode->add_theme_color_override("font_color", Color(0.68f, 0.74f, 0.85f));
	hurtbox_sec->add_child(lbl_hurtbox_mode);

	chk_hurtbox_enabled = memnew(CheckBox(CG_SE_TR("Hurtbox activa")));
	chk_hurtbox_enabled->set_pressed(true);
	chk_hurtbox_enabled->connect("toggled", callable_mp(this, &ClockGearsShapeEditor::_on_hurtbox_enabled_toggled));
	hurtbox_sec->add_child(chk_hurtbox_enabled);

	make_slider_spin_row(hurtbox_sec, CG_SE_TR("Mult. daño recibido (%)"), 0, 500, 1000, 5, hurt_dmg_slider, hurt_dmg_spin);
	hurt_dmg_slider->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hurt_dmg_slider_changed));
	hurt_dmg_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_hurt_dmg_spin_changed));

	// Sección Forma seleccionada (ajuste numérico fino)
	sel_shape_sec = memnew(VBoxContainer);
	sel_shape_sec->add_theme_constant_override("separation", int(4.0f * CG_SE_EDSCALE));
	inspector_vbox->add_child(sel_shape_sec);
	sel_shape_sec->add_child(memnew(HSeparator));

	lbl_sel_shape_title = memnew(Label(CG_SE_TR("Forma seleccionada")));
	lbl_sel_shape_title->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.40f));
	sel_shape_sec->add_child(lbl_sel_shape_title);

	GridContainer *sel_grid = memnew(GridContainer);
	sel_grid->set_columns(2);
	sel_grid->add_theme_constant_override("h_separation", int(8.0f * CG_SE_EDSCALE));
	sel_grid->add_theme_constant_override("v_separation", int(4.0f * CG_SE_EDSCALE));
	sel_shape_sec->add_child(sel_grid);

	auto make_num_cell = [&](const String &p_txt, double p_min, double p_max, SpinBox *&r_sp, Label *&r_lbl) {
		r_lbl = memnew(Label(p_txt));
		r_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sel_grid->add_child(r_lbl);
		r_sp = memnew(SpinBox);
		r_sp->set_min(p_min);
		r_sp->set_max(p_max);
		r_sp->set_step(1.0);
		_apply_spinbox_round_style(r_sp);
		r_sp->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_sel_shape_prop_changed));
		sel_grid->add_child(r_sp);
	};

	Label *dummy_lbl = nullptr;
	make_num_cell(CG_SE_TR("Centro X:"), -512, 512, sel_off_x_spin, dummy_lbl);
	make_num_cell(CG_SE_TR("Centro Y:"), -512, 512, sel_off_y_spin, dummy_lbl);
	make_num_cell(CG_SE_TR("Ancho / Radio:"), 1, 512, sel_dim1_spin, lbl_sel_dim1);
	make_num_cell(CG_SE_TR("Alto:"), 1, 512, sel_dim2_spin, lbl_sel_dim2);
	make_num_cell(CG_SE_TR("Rotación (°):"), -360, 360, sel_rot_spin, dummy_lbl);

	// Sección Ancla de arma
	anchor_sec = memnew(VBoxContainer);
	anchor_sec->add_theme_constant_override("separation", int(6.0f * CG_SE_EDSCALE));
	inspector_vbox->add_child(anchor_sec);
	anchor_sec->add_child(memnew(HSeparator));

	Label *lbl_anc_hdr = memnew(Label(CG_SE_TR("Ancla de arma")));
	lbl_anc_hdr->add_theme_color_override("font_color", Color(1.0f, 0.78f, 0.25f));
	anchor_sec->add_child(lbl_anc_hdr);

	chk_anchor_enabled = memnew(CheckBox(CG_SE_TR("Activar ancla de arma en este frame")));
	chk_anchor_enabled->connect("toggled", callable_mp(this, &ClockGearsShapeEditor::_on_anchor_enabled_toggled));
	anchor_sec->add_child(chk_anchor_enabled);

	HBoxContainer *anc_xy = memnew(HBoxContainer);
	anc_xy->add_theme_constant_override("separation", int(6.0f * CG_SE_EDSCALE));
	anchor_sec->add_child(anc_xy);

	anc_xy->add_child(memnew(Label("X:")));
	anchor_x_spin = memnew(SpinBox);
	anchor_x_spin->set_min(-256);
	anchor_x_spin->set_max(256);
	_apply_spinbox_round_style(anchor_x_spin);
	anchor_x_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anchor_x_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_anchor_pos_changed));
	anc_xy->add_child(anchor_x_spin);

	anc_xy->add_child(memnew(Label("Y:")));
	anchor_y_spin = memnew(SpinBox);
	anchor_y_spin->set_min(-256);
	anchor_y_spin->set_max(256);
	_apply_spinbox_round_style(anchor_y_spin);
	anchor_y_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anchor_y_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_anchor_pos_changed));
	anc_xy->add_child(anchor_y_spin);

	make_slider_spin_row(anchor_sec, CG_SE_TR("Rotación arma (°)"), -180, 180, 360, 1, anchor_rot_slider, anchor_rot_spin);
	anchor_rot_slider->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_anchor_rot_slider_changed));
	anchor_rot_spin->connect("value_changed", callable_mp(this, &ClockGearsShapeEditor::_on_anchor_rot_spin_changed));

	// Línea de estado / ayuda
	status_label = memnew(Label(CG_SE_TR("Clic o arrastra para mover, tirar de los puntos para redimensionar. La rueda no hace zoom.")));
	status_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	status_label->add_theme_color_override("font_color", Color(0.68f, 0.74f, 0.85f));
	add_child(status_label);

	hurtbox_data = ClockGearsHurtboxData::make_default_for_frame(32, 32);
	_update_toolbar_states();
	_sync_inspector_from_data();
}

Vector2 ClockGearsShapeEditor::_maybe_snap_local(const Vector2 &p_local) const {
	if (!snap_enabled) {
		return Vector2(Math::round(p_local.x), Math::round(p_local.y));
	}
	return Vector2(
			Math::round(p_local.x / CG_SHAPE_GRID_STEP) * CG_SHAPE_GRID_STEP,
			Math::round(p_local.y / CG_SHAPE_GRID_STEP) * CG_SHAPE_GRID_STEP);
}

ClockGearsHitShape *ClockGearsShapeEditor::_get_mutable_selected_shape() {
	if (selected_kind == SEL_HITBOX && allow_hitboxes &&
			selected_hitbox_idx >= 0 && selected_hitbox_idx < hitbox_data.shapes.size()) {
		return &hitbox_data.shapes.write[selected_hitbox_idx];
	}
	if (selected_kind == SEL_HURTBOX && allow_hurtbox && hurtbox_data.enabled) {
		return &hurtbox_data.shape;
	}
	return nullptr;
}

const ClockGearsHitShape *ClockGearsShapeEditor::_get_const_selected_shape() const {
	if (selected_kind == SEL_HITBOX && allow_hitboxes &&
			selected_hitbox_idx >= 0 && selected_hitbox_idx < hitbox_data.shapes.size()) {
		return &hitbox_data.shapes[selected_hitbox_idx];
	}
	if (selected_kind == SEL_HURTBOX && allow_hurtbox && hurtbox_data.enabled) {
		return &hurtbox_data.shape;
	}
	return nullptr;
}

bool ClockGearsShapeEditor::_hit_test_shape(const ClockGearsHitShape &p_shape, const Vector2 &p_local) const {
	Transform2D inv(Math::deg_to_rad(-p_shape.rotation), Vector2());
	Vector2 pt = inv.xform(p_local - p_shape.offset);
	if (p_shape.type == CG_SHAPE_CIRCLE) {
		return pt.length() <= MAX(4.0f, p_shape.get_radius());
	}
	if (p_shape.type == CG_SHAPE_RECT) {
		float hw = MAX(4.0f, p_shape.size.x * 0.5f);
		float hh = MAX(4.0f, p_shape.size.y * 0.5f);
		return Math::abs(pt.x) <= hw && Math::abs(pt.y) <= hh;
	}
	if (p_shape.type == CG_SHAPE_CAPSULE) {
		float r = MAX(3.0f, p_shape.get_radius());
		float h = MAX(r * 2.0f, p_shape.get_capsule_height());
		float half_seg = MAX(0.0f, h * 0.5f - r);
		float clamped_y = CLAMP(pt.y, -half_seg, half_seg);
		return pt.distance_to(Vector2(0.0f, clamped_y)) <= r;
	}
	if (p_shape.type == CG_SHAPE_POLYGON && p_shape.vertices.size() >= 3) {
		return Geometry2D::is_point_in_polygon(pt, p_shape.vertices);
	}
	return false;
}

void ClockGearsShapeEditor::_get_shape_handle_points(const ClockGearsHitShape &p_shape, Vector<Vector2> &r_local_points) const {
	r_local_points.clear();
	Transform2D xform(Math::deg_to_rad(p_shape.rotation), p_shape.offset);
	if (p_shape.type == CG_SHAPE_CIRCLE) {
		r_local_points.push_back(xform.xform(Vector2(p_shape.get_radius(), 0.0f)));
	} else if (p_shape.type == CG_SHAPE_RECT) {
		float hw = p_shape.size.x * 0.5f;
		float hh = p_shape.size.y * 0.5f;
		r_local_points.push_back(xform.xform(Vector2(-hw, -hh)));
		r_local_points.push_back(xform.xform(Vector2(hw, -hh)));
		r_local_points.push_back(xform.xform(Vector2(hw, hh)));
		r_local_points.push_back(xform.xform(Vector2(-hw, hh)));
		r_local_points.push_back(xform.xform(Vector2(hw, 0.0f)));
		r_local_points.push_back(xform.xform(Vector2(0.0f, hh)));
	} else if (p_shape.type == CG_SHAPE_CAPSULE) {
		float r = p_shape.get_radius();
		float hh = p_shape.get_capsule_height() * 0.5f;
		r_local_points.push_back(xform.xform(Vector2(r, 0.0f)));
		r_local_points.push_back(xform.xform(Vector2(0.0f, -hh)));
		r_local_points.push_back(xform.xform(Vector2(0.0f, hh)));
	} else if (p_shape.type == CG_SHAPE_POLYGON) {
		for (int i = 0; i < p_shape.vertices.size(); i++) {
			r_local_points.push_back(xform.xform(p_shape.vertices[i]));
		}
	}
}

int ClockGearsShapeEditor::_hit_test_handles(const ClockGearsHitShape &p_shape, const Point2 &p_screen_pos, const Point2 &p_origin) const {
	Vector<Vector2> locals;
	_get_shape_handle_points(p_shape, locals);
	float thresh = 9.0f * CG_SE_EDSCALE;
	for (int i = 0; i < locals.size(); i++) {
		Point2 hs = p_origin + locals[i] * float(MAX(1, zoom));
		if (p_screen_pos.distance_to(hs) <= thresh) {
			return i;
		}
	}
	return -1;
}

void ClockGearsShapeEditor::_commit_created_shape(const ClockGearsHitShape &p_shape) {
	if (current_target == TARGET_HITBOX && allow_hitboxes) {
		hitbox_data.shapes.push_back(p_shape);
		selected_kind = SEL_HITBOX;
		selected_hitbox_idx = hitbox_data.shapes.size() - 1;
	} else if (allow_hurtbox) {
		hurtbox_data.enabled = true;
		hurtbox_data.shape = p_shape;
		selected_kind = SEL_HURTBOX;
		selected_hitbox_idx = -1;
	}
	_sync_inspector_from_data();
	_update_toolbar_states();
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_sync_selected_shape_controls() {
	if (!sel_shape_sec) {
		return;
	}
	const ClockGearsHitShape *sh = _get_const_selected_shape();
	if (!sh) {
		sel_shape_sec->set_visible(false);
		return;
	}
	sel_shape_sec->set_visible(true);
	bool prev = updating_controls;
	updating_controls = true;

	String kind_txt = (selected_kind == SEL_HITBOX) ? CG_SE_TR("Hitbox") : CG_SE_TR("Hurtbox");
	if (lbl_sel_shape_title) {
		String tname = CG_SE_TR("Rectángulo");
		if (sh->type == CG_SHAPE_CIRCLE) tname = CG_SE_TR("Círculo");
		else if (sh->type == CG_SHAPE_CAPSULE) tname = CG_SE_TR("Cápsula");
		else if (sh->type == CG_SHAPE_POLYGON) tname = CG_SE_TR("Polígono");
		lbl_sel_shape_title->set_text(vformat("%s (%s)", kind_txt, tname));
	}
	if (sel_off_x_spin) sel_off_x_spin->set_value(sh->offset.x);
	if (sel_off_y_spin) sel_off_y_spin->set_value(sh->offset.y);
	if (sel_rot_spin) sel_rot_spin->set_value(sh->rotation);

	bool show_d1 = (sh->type != CG_SHAPE_POLYGON);
	bool show_d2 = (sh->type == CG_SHAPE_RECT || sh->type == CG_SHAPE_CAPSULE);
	if (lbl_sel_dim1) {
		lbl_sel_dim1->set_visible(show_d1);
		if (sh->type == CG_SHAPE_CIRCLE || sh->type == CG_SHAPE_CAPSULE) {
			lbl_sel_dim1->set_text(CG_SE_TR("Radio:"));
		} else {
			lbl_sel_dim1->set_text(CG_SE_TR("Ancho:"));
		}
	}
	if (sel_dim1_spin) {
		sel_dim1_spin->set_visible(show_d1);
		sel_dim1_spin->set_value(sh->size.x);
	}
	if (lbl_sel_dim2) {
		lbl_sel_dim2->set_visible(show_d2);
		lbl_sel_dim2->set_text(CG_SE_TR("Alto:"));
	}
	if (sel_dim2_spin) {
		sel_dim2_spin->set_visible(show_d2);
		sel_dim2_spin->set_value(sh->size.y);
	}

	updating_controls = prev;
}

void ClockGearsShapeEditor::_sync_inspector_from_data() {
	bool prev = updating_controls;
	updating_controls = true;

	if (hitbox_sec) {
		hitbox_sec->set_visible(allow_hitboxes);
	}
	if (hb_dmg_slider) hb_dmg_slider->set_value(hitbox_data.damage_mult_pct);
	if (hb_dmg_spin) hb_dmg_spin->set_value(hitbox_data.damage_mult_pct);
	if (hb_kb_slider) hb_kb_slider->set_value(hitbox_data.knockback);
	if (hb_kb_spin) hb_kb_spin->set_value(hitbox_data.knockback);
	if (hb_hitstop_slider) hb_hitstop_slider->set_value(hitbox_data.hitstop_ms);
	if (hb_hitstop_spin) hb_hitstop_spin->set_value(hitbox_data.hitstop_ms);

	if (hurtbox_sec) {
		hurtbox_sec->set_visible(allow_hurtbox);
	}
	if (chk_hurtbox_override) chk_hurtbox_override->set_pressed(has_hurtbox_override);
	if (lbl_hurtbox_mode) {
		lbl_hurtbox_mode->set_text(has_hurtbox_override
						? CG_SE_TR("Hurtbox anulada solo en este frame")
						: CG_SE_TR("Usando hurtbox compartida del sprite"));
	}
	if (chk_hurtbox_enabled) chk_hurtbox_enabled->set_pressed(hurtbox_data.enabled);
	if (hurt_dmg_slider) hurt_dmg_slider->set_value(hurtbox_data.damage_mult_pct);
	if (hurt_dmg_spin) hurt_dmg_spin->set_value(hurtbox_data.damage_mult_pct);

	if (anchor_sec) {
		anchor_sec->set_visible(allow_weapon_anchor);
	}
	if (chk_anchor_enabled) chk_anchor_enabled->set_pressed(weapon_anchor_data.enabled);
	if (anchor_x_spin) {
		anchor_x_spin->set_editable(weapon_anchor_data.enabled);
		anchor_x_spin->set_value(weapon_anchor_data.position.x);
	}
	if (anchor_y_spin) {
		anchor_y_spin->set_editable(weapon_anchor_data.enabled);
		anchor_y_spin->set_value(weapon_anchor_data.position.y);
	}
	if (anchor_rot_slider) {
		anchor_rot_slider->set_editable(weapon_anchor_data.enabled);
		anchor_rot_slider->set_value(weapon_anchor_data.rotation);
	}
	if (anchor_rot_spin) {
		anchor_rot_spin->set_editable(weapon_anchor_data.enabled);
		anchor_rot_spin->set_value(weapon_anchor_data.rotation);
	}

	updating_controls = prev;
	_sync_selected_shape_controls();
}

void ClockGearsShapeEditor::_update_toolbar_states() {
	for (int i = 0; i < 8; i++) {
		_apply_rounded_button_styles(zoom_btns[i], zoom == (i + 1), Color(0.28f, 0.56f, 0.92f));
	}
	if (btn_grid) {
		btn_grid->set_pressed_no_signal(grid_enabled);
		_apply_rounded_button_styles(btn_grid, grid_enabled, Color(0.28f, 0.62f, 0.85f));
	}
	if (btn_snap) {
		btn_snap->set_pressed_no_signal(snap_enabled);
		_apply_rounded_button_styles(btn_snap, snap_enabled, Color(0.90f, 0.68f, 0.25f));
	}

	if (!allow_hitboxes && current_target == TARGET_HITBOX) {
		current_target = TARGET_HURTBOX;
	}
	if (btn_target_hitbox) {
		btn_target_hitbox->set_visible(allow_hitboxes);
		_apply_rounded_button_styles(btn_target_hitbox, current_target == TARGET_HITBOX, Color(0.92f, 0.30f, 0.30f));
	}
	if (btn_target_hurtbox) {
		btn_target_hurtbox->set_visible(allow_hurtbox);
		_apply_rounded_button_styles(btn_target_hurtbox, current_target == TARGET_HURTBOX, Color(0.25f, 0.62f, 0.96f));
	}

	Color tool_accent = (current_target == TARGET_HITBOX && allow_hitboxes)
			? Color(0.92f, 0.34f, 0.34f)
			: Color(0.28f, 0.62f, 0.96f);
	_apply_rounded_button_styles(btn_tool_select, current_tool == TOOL_SELECT, Color(0.32f, 0.58f, 0.92f));
	_apply_rounded_button_styles(btn_tool_circle, current_tool == TOOL_CIRCLE, tool_accent);
	_apply_rounded_button_styles(btn_tool_rect, current_tool == TOOL_RECT, tool_accent);
	_apply_rounded_button_styles(btn_tool_capsule, current_tool == TOOL_CAPSULE, tool_accent);
	_apply_rounded_button_styles(btn_tool_polygon, current_tool == TOOL_POLYGON, tool_accent);

	if (btn_finish_polygon) {
		bool show_finish = (current_tool == TOOL_POLYGON && poly_draft_vertices.size() >= 3);
		btn_finish_polygon->set_visible(show_finish);
		_apply_rounded_button_styles(btn_finish_polygon, true, Color(0.26f, 0.72f, 0.42f));
	}
	if (btn_delete_shape) {
		bool can_del = (selected_kind == SEL_HITBOX || selected_kind == SEL_HURTBOX);
		btn_delete_shape->set_disabled(!can_del);
		_apply_rounded_button_styles(btn_delete_shape, false, Color(0.85f, 0.30f, 0.30f));
	}
}

void ClockGearsShapeEditor::_emit_shape_changed() {
	emit_signal("shape_data_changed");
}

void ClockGearsShapeEditor::set_preview_texture(const Ref<Texture2D> &p_tex, const Rect2i &p_region) {
	preview_texture = p_tex;
	preview_region = p_region;
	if (preview_region.size.x <= 0 || preview_region.size.y <= 0) {
		if (preview_texture.is_valid()) {
			preview_region = Rect2i(Point2i(), Vector2i(MAX(1, preview_texture->get_width()), MAX(1, preview_texture->get_height())));
		} else {
			preview_region = Rect2i(0, 0, 32, 32);
		}
	}
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::set_mannequin(bool p_show, int p_size) {
	show_mannequin = p_show;
	mannequin_size = MAX(16, p_size);
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::set_feature_flags(bool p_allow_hitboxes, bool p_allow_hurtbox, bool p_allow_weapon_anchor) {
	allow_hitboxes = p_allow_hitboxes;
	allow_hurtbox = p_allow_hurtbox;
	allow_weapon_anchor = p_allow_weapon_anchor;
	if (!allow_hitboxes && selected_kind == SEL_HITBOX) {
		selected_kind = SEL_NONE;
		selected_hitbox_idx = -1;
	}
	_update_toolbar_states();
	_sync_inspector_from_data();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::set_shape_data(const Dictionary &p_hitbox_dict, const Dictionary &p_hurtbox_dict, bool p_has_hurtbox_override, const Dictionary &p_weapon_anchor_dict) {
	hitbox_data.from_dict(p_hitbox_dict);
	int fw = MAX(1, preview_region.size.x);
	int fh = MAX(1, preview_region.size.y);
	hurtbox_data.from_dict(p_hurtbox_dict, fw, fh);
	has_hurtbox_override = p_has_hurtbox_override;
	weapon_anchor_data.from_dict(p_weapon_anchor_dict);

	poly_draft_vertices.clear();
	create_drag_active = false;
	drag_kind = DRAG_NONE;

	if (selected_kind == SEL_HITBOX && (selected_hitbox_idx < 0 || selected_hitbox_idx >= hitbox_data.shapes.size())) {
		selected_kind = SEL_NONE;
		selected_hitbox_idx = -1;
	}
	_update_toolbar_states();
	_sync_inspector_from_data();
	if (canvas) {
		canvas->queue_redraw();
	}
}

Dictionary ClockGearsShapeEditor::get_hitbox_data() const {
	return hitbox_data.to_dict();
}

Dictionary ClockGearsShapeEditor::get_hurtbox_data() const {
	return hurtbox_data.to_dict();
}

Dictionary ClockGearsShapeEditor::get_weapon_anchor_data() const {
	return weapon_anchor_data.to_dict();
}

void ClockGearsShapeEditor::set_zoom(int p_zoom) {
	zoom = CLAMP(p_zoom, 1, 8);
	_update_toolbar_states();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::set_grid_enabled(bool p_enabled) {
	grid_enabled = p_enabled;
	_update_toolbar_states();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::set_snap_enabled(bool p_enabled) {
	snap_enabled = p_enabled;
	_update_toolbar_states();
}

void ClockGearsShapeEditor::set_edit_target(int p_target) {
	if (p_target == TARGET_HITBOX && allow_hitboxes) {
		current_target = TARGET_HITBOX;
	} else if (p_target == TARGET_ANCHOR && allow_weapon_anchor) {
		current_target = TARGET_ANCHOR;
	} else {
		current_target = TARGET_HURTBOX;
	}
	poly_draft_vertices.clear();
	_update_toolbar_states();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::set_tool(int p_tool) {
	current_tool = (ToolMode)CLAMP(p_tool, (int)TOOL_SELECT, (int)TOOL_POLYGON);
	if (current_tool != TOOL_POLYGON) {
		poly_draft_vertices.clear();
		poly_hover_valid = false;
	}
	_update_toolbar_states();
	if (canvas) {
		canvas->queue_redraw();
	}
}

int ClockGearsShapeEditor::add_hitbox_shape(const Dictionary &p_shape_dict) {
	ClockGearsHitShape sh;
	sh.from_dict(p_shape_dict);
	hitbox_data.shapes.push_back(sh);
	int idx = hitbox_data.shapes.size() - 1;
	selected_kind = SEL_HITBOX;
	selected_hitbox_idx = idx;
	_update_toolbar_states();
	_sync_inspector_from_data();
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
	return idx;
}

void ClockGearsShapeEditor::set_hurtbox_shape(const Dictionary &p_shape_dict) {
	hurtbox_data.enabled = true;
	hurtbox_data.shape.from_dict(p_shape_dict);
	selected_kind = SEL_HURTBOX;
	selected_hitbox_idx = -1;
	_update_toolbar_states();
	_sync_inspector_from_data();
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

bool ClockGearsShapeEditor::finish_polygon() {
	if (poly_draft_vertices.size() < 3) {
		return false;
	}
	Vector2 centroid;
	for (int i = 0; i < poly_draft_vertices.size(); i++) {
		centroid += poly_draft_vertices[i];
	}
	centroid /= float(poly_draft_vertices.size());

	Vector<Vector2> rel_verts;
	rel_verts.resize(poly_draft_vertices.size());
	for (int i = 0; i < poly_draft_vertices.size(); i++) {
		rel_verts.write[i] = poly_draft_vertices[i] - centroid;
	}

	ClockGearsHitShape poly_sh = ClockGearsHitShape::make_polygon(rel_verts, centroid, 0.0f);
	poly_draft_vertices.clear();
	poly_hover_valid = false;
	current_tool = TOOL_SELECT;
	_commit_created_shape(poly_sh);
	return true;
}

bool ClockGearsShapeEditor::delete_selected_shape() {
	if (selected_kind == SEL_HITBOX && selected_hitbox_idx >= 0 && selected_hitbox_idx < hitbox_data.shapes.size()) {
		hitbox_data.shapes.remove_at(selected_hitbox_idx);
		selected_kind = SEL_NONE;
		selected_hitbox_idx = -1;
		_update_toolbar_states();
		_sync_inspector_from_data();
		if (canvas) {
			canvas->queue_redraw();
		}
		_emit_shape_changed();
		return true;
	}
	if (selected_kind == SEL_HURTBOX) {
		int fw = MAX(1, preview_region.size.x);
		int fh = MAX(1, preview_region.size.y);
		hurtbox_data.enabled = false;
		hurtbox_data.shape = ClockGearsHitShape::make_rect(Vector2(float(fw), float(fh)));
		selected_kind = SEL_NONE;
		_update_toolbar_states();
		_sync_inspector_from_data();
		if (canvas) {
			canvas->queue_redraw();
		}
		_emit_shape_changed();
		return true;
	}
	return false;
}

void ClockGearsShapeEditor::select_shape(int p_kind, int p_index) {
	if (p_kind == SEL_HITBOX && p_index >= 0 && p_index < hitbox_data.shapes.size()) {
		selected_kind = SEL_HITBOX;
		selected_hitbox_idx = p_index;
	} else if (p_kind == SEL_HURTBOX && allow_hurtbox) {
		selected_kind = SEL_HURTBOX;
		selected_hitbox_idx = -1;
	} else if (p_kind == SEL_ANCHOR && allow_weapon_anchor) {
		selected_kind = SEL_ANCHOR;
		selected_hitbox_idx = -1;
	} else {
		selected_kind = SEL_NONE;
		selected_hitbox_idx = -1;
	}
	_update_toolbar_states();
	_sync_selected_shape_controls();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsShapeEditor::_on_zoom_btn_pressed(int p_zoom) {
	set_zoom(p_zoom);
}

void ClockGearsShapeEditor::_on_grid_toggled(bool p_pressed) {
	set_grid_enabled(p_pressed);
}

void ClockGearsShapeEditor::_on_snap_toggled(bool p_pressed) {
	set_snap_enabled(p_pressed);
}

void ClockGearsShapeEditor::_on_target_btn_pressed(int p_target) {
	set_edit_target(p_target);
}

void ClockGearsShapeEditor::_on_tool_btn_pressed(int p_tool) {
	set_tool(p_tool);
}

void ClockGearsShapeEditor::_on_finish_polygon_pressed() {
	finish_polygon();
}

void ClockGearsShapeEditor::_on_delete_shape_pressed() {
	delete_selected_shape();
}

void ClockGearsShapeEditor::_on_hb_dmg_slider_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hitbox_data.damage_mult_pct = MAX(0.0f, float(p_val));
	if (hb_dmg_spin) hb_dmg_spin->set_value(hitbox_data.damage_mult_pct);
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hb_dmg_spin_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hitbox_data.damage_mult_pct = MAX(0.0f, float(p_val));
	if (hb_dmg_slider) hb_dmg_slider->set_value(CLAMP(hitbox_data.damage_mult_pct, 0.0f, float(hb_dmg_slider->get_max())));
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hb_kb_slider_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hitbox_data.knockback = MAX(0.0f, float(p_val));
	if (hb_kb_spin) hb_kb_spin->set_value(hitbox_data.knockback);
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hb_kb_spin_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hitbox_data.knockback = MAX(0.0f, float(p_val));
	if (hb_kb_slider) hb_kb_slider->set_value(CLAMP(hitbox_data.knockback, 0.0f, float(hb_kb_slider->get_max())));
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hb_hitstop_slider_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hitbox_data.hitstop_ms = MAX(0, int(Math::round(p_val)));
	if (hb_hitstop_spin) hb_hitstop_spin->set_value(hitbox_data.hitstop_ms);
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hb_hitstop_spin_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hitbox_data.hitstop_ms = MAX(0, int(Math::round(p_val)));
	if (hb_hitstop_slider) hb_hitstop_slider->set_value(CLAMP(double(hitbox_data.hitstop_ms), 0.0, hb_hitstop_slider->get_max()));
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hurtbox_override_toggled(bool p_pressed) {
	if (updating_controls) return;
	has_hurtbox_override = p_pressed;
	_sync_inspector_from_data();
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hurtbox_enabled_toggled(bool p_pressed) {
	if (updating_controls) return;
	hurtbox_data.enabled = p_pressed;
	_sync_selected_shape_controls();
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hurt_dmg_slider_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hurtbox_data.damage_mult_pct = MAX(0.0f, float(p_val));
	if (hurt_dmg_spin) hurt_dmg_spin->set_value(hurtbox_data.damage_mult_pct);
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_hurt_dmg_spin_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	hurtbox_data.damage_mult_pct = MAX(0.0f, float(p_val));
	if (hurt_dmg_slider) hurt_dmg_slider->set_value(CLAMP(hurtbox_data.damage_mult_pct, 0.0f, float(hurt_dmg_slider->get_max())));
	updating_controls = false;
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_sel_shape_prop_changed(double p_unused) {
	if (updating_controls) return;
	ClockGearsHitShape *sh = _get_mutable_selected_shape();
	if (!sh) return;
	if (sel_off_x_spin && sel_off_y_spin) {
		sh->offset = Vector2(float(sel_off_x_spin->get_value()), float(sel_off_y_spin->get_value()));
	}
	if (sel_rot_spin) {
		sh->rotation = float(sel_rot_spin->get_value());
	}
	if (sh->type == CG_SHAPE_CIRCLE && sel_dim1_spin) {
		float r = MAX(0.5f, float(sel_dim1_spin->get_value()));
		sh->size = Vector2(r, r);
	} else if (sh->type == CG_SHAPE_CAPSULE && sel_dim1_spin && sel_dim2_spin) {
		float r = MAX(0.5f, float(sel_dim1_spin->get_value()));
		float h = MAX(r * 2.0f, float(sel_dim2_spin->get_value()));
		sh->size = Vector2(r, h);
	} else if (sh->type == CG_SHAPE_RECT && sel_dim1_spin && sel_dim2_spin) {
		sh->size = Vector2(MAX(0.5f, float(sel_dim1_spin->get_value())), MAX(0.5f, float(sel_dim2_spin->get_value())));
	}
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_anchor_enabled_toggled(bool p_pressed) {
	if (updating_controls) return;
	weapon_anchor_data.enabled = p_pressed;
	_sync_inspector_from_data();
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_anchor_pos_changed(double p_unused) {
	if (updating_controls) return;
	if (anchor_x_spin && anchor_y_spin) {
		weapon_anchor_data.position = Vector2(float(anchor_x_spin->get_value()), float(anchor_y_spin->get_value()));
	}
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_anchor_rot_slider_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	weapon_anchor_data.rotation = float(p_val);
	if (anchor_rot_spin) anchor_rot_spin->set_value(weapon_anchor_data.rotation);
	updating_controls = false;
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}

void ClockGearsShapeEditor::_on_anchor_rot_spin_changed(double p_val) {
	if (updating_controls) return;
	updating_controls = true;
	weapon_anchor_data.rotation = float(p_val);
	if (anchor_rot_slider) anchor_rot_slider->set_value(CLAMP(weapon_anchor_data.rotation, float(anchor_rot_slider->get_min()), float(anchor_rot_slider->get_max())));
	updating_controls = false;
	if (canvas) {
		canvas->queue_redraw();
	}
	_emit_shape_changed();
}
