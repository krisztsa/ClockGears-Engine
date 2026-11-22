#include "clockgears_image_adjust_dialog.h"

#include "../runtime/clockgears_ui_design_layer.h"
#include "../runtime/clockgears_window_style.h"
#include "clockgears_theme.h"
#include "core/input/input_event.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "core/os/keyboard.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/margin_container.h"

static PackedVector2Array s_build_shape_outline(const Rect2 &p_rect, const Dictionary &p_shape_cfg) {
	PackedVector2Array pts;
	String st = String(p_shape_cfg.get("shape_type", "rect")).strip_edges().to_lower();
	if (st == "rect" || st == "square") {
		pts.push_back(p_rect.position);
		pts.push_back(Point2(p_rect.position.x + p_rect.size.x, p_rect.position.y));
		pts.push_back(p_rect.position + p_rect.size);
		pts.push_back(Point2(p_rect.position.x, p_rect.position.y + p_rect.size.y));
		return pts;
	}
	if (st == "circle" || st == "ellipse") {
		Vector2 c = p_rect.get_center();
		float rx = (st == "circle") ? MIN(p_rect.size.x, p_rect.size.y) * 0.5f : p_rect.size.x * 0.5f;
		float ry = (st == "circle") ? rx : p_rect.size.y * 0.5f;
		const int segs = 40;
		for (int i = 0; i < segs; i++) {
			float a = (Math::TAU * float(i)) / float(segs);
			pts.push_back(c + Vector2(Math::cos(a) * rx, Math::sin(a) * ry));
		}
		return pts;
	}
	float r = (st == "pill") ? MIN(p_rect.size.x, p_rect.size.y) * 0.5f : float(p_shape_cfg.get("corner_radius", 8));
	r = CLAMP(r, 0.0f, MIN(p_rect.size.x, p_rect.size.y) * 0.5f);
	if (r <= 1.0f) {
		return s_build_shape_outline(p_rect, Dictionary());
	}
	const int arc_segs = 6;
	Vector2 centers[4] = {
		Vector2(p_rect.position.x + p_rect.size.x - r, p_rect.position.y + r),
		Vector2(p_rect.position.x + p_rect.size.x - r, p_rect.position.y + p_rect.size.y - r),
		Vector2(p_rect.position.x + r, p_rect.position.y + p_rect.size.y - r),
		Vector2(p_rect.position.x + r, p_rect.position.y + r)
	};
	float start_ang[4] = { -Math::PI * 0.5f, 0.0f, Math::PI * 0.5f, Math::PI };
	for (int corner = 0; corner < 4; corner++) {
		for (int s = 0; s <= arc_segs; s++) {
			float a = start_ang[corner] + (Math::PI * 0.5f) * (float(s) / float(arc_segs));
			pts.push_back(centers[corner] + Vector2(Math::cos(a) * r, Math::sin(a) * r));
		}
	}
	return pts;
}

ClockGearsImageAdjustCanvas::ClockGearsImageAdjustCanvas() {
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_clip_contents(true);
}

void ClockGearsImageAdjustCanvas::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW || !owner) {
		return;
	}
	Size2 sz = get_size();
	if (sz.x <= 4.0f || sz.y <= 4.0f) {
		return;
	}
	ClockGearsTheme *ct = ClockGearsTheme::get_singleton();
	bool is_brass = ct && ct->get_active_theme_id() == "brass";
	bool is_light = ct && ct->is_light_mode();
	ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, 1.0f);
	Color bg_col = tk.surface_sunken;
	Color frame_bg = tk.surface_base;
	Color accent = tk.border_copper_bright;
	Color guide_col = tk.accent_neon;

	draw_rect(Rect2(Vector2(), sz), bg_col, true);

	Rect2 frect = owner->_compute_frame_preview_rect();
	if (frect.size.x <= 2.0f || frect.size.y <= 2.0f) {
		return;
	}

	// Draw frame background with subtle checker tint inside the frame bounds
	draw_rect(frect, frame_bg, true);
	Ref<Texture2D> tex;
	if (owner->active_state_idx >= 0 && owner->active_state_idx < owner->state_slots.size()) {
		tex = owner->state_slots[owner->active_state_idx].texture;
	}
	bool is_int = bool(owner->edit_vals.get("manual_integer_scale", false));
	set_texture_filter((is_int || owner->shape_cfg.has("pixel_art") ? bool(owner->shape_cfg.get("pixel_art", true)) : true) ? TEXTURE_FILTER_NEAREST : TEXTURE_FILTER_LINEAR);

	Rect2 img_rect;
	if (tex.is_valid()) {
		img_rect = owner->_compute_image_preview_rect(frect, owner->edit_vals);
		// Full image softly dimmed to reveal out-of-frame area while dragging/zooming
		draw_texture_rect(tex, img_rect, false, Color(1.0f, 1.0f, 1.0f, 0.28f));

		// Clipped region inside exact shape / frame rectangle
		Rect2 clipped = img_rect.intersection(frect);
		if (clipped.size.x > 0.5f && clipped.size.y > 0.5f && img_rect.size.x > 0.5f && img_rect.size.y > 0.5f) {
			Vector2 ts = tex->get_size();
			Vector2 uv0 = (clipped.position - img_rect.position) / img_rect.size;
			Vector2 uv1 = (clipped.position + clipped.size - img_rect.position) / img_rect.size;
			Rect2 src_rect(uv0 * ts, (uv1 - uv0) * ts);
			draw_texture_rect_region(tex, clipped, src_rect, Color(1, 1, 1, 1));
		}
	}

	// Darken outside rect slightly so target frame pops clearly
	Color mask_col(bg_col.r, bg_col.g, bg_col.b, 0.55f);
	if (frect.position.y > 0.0f) {
		draw_rect(Rect2(0, 0, sz.x, frect.position.y), mask_col, true);
	}
	if (frect.position.y + frect.size.y < sz.y) {
		draw_rect(Rect2(0, frect.position.y + frect.size.y, sz.x, sz.y - (frect.position.y + frect.size.y)), mask_col, true);
	}
	if (frect.position.x > 0.0f) {
		draw_rect(Rect2(0, frect.position.y, frect.position.x, frect.size.y), mask_col, true);
	}
	if (frect.position.x + frect.size.x < sz.x) {
		draw_rect(Rect2(frect.position.x + frect.size.x, frect.position.y, sz.x - (frect.position.x + frect.size.x), frect.size.y), mask_col, true);
	}

	// Subtle center crosshair guides inside target frame
	Vector2 fc = frect.get_center();
	draw_line(Point2(fc.x, frect.position.y), Point2(fc.x, frect.position.y + frect.size.y), Color(guide_col.r, guide_col.g, guide_col.b, 0.25f), 1.0f);
	draw_line(Point2(frect.position.x, fc.y), Point2(frect.position.x + frect.size.x, fc.y), Color(guide_col.r, guide_col.g, guide_col.b, 0.25f), 1.0f);

	// Target frame/shape outline
	PackedVector2Array outline = s_build_shape_outline(frect, owner->shape_cfg);
	if (outline.size() >= 3) {
		outline.push_back(outline[0]);
		draw_polyline(outline, accent, 2.0f, true);
	} else {
		draw_rect(frect, accent, false, 2.0f);
	}

	// Image bounds + 4 draggable corner handles
	if (tex.is_valid()) {
		draw_rect(img_rect, Color(guide_col.r, guide_col.g, guide_col.b, 0.8f), false, 1.5f);
		Vector<Point2> corners = ClockGearsImageAdjustDialog::get_corner_handles(img_rect);
		for (int i = 0; i < corners.size(); i++) {
			Rect2 hr(corners[i] - Point2(5.0f, 5.0f), Size2(10.0f, 10.0f));
			draw_rect(hr, accent, true);
			draw_rect(hr, bg_col, false, 1.5f);
		}
	}
}

void ClockGearsImageAdjustCanvas::gui_input(const Ref<InputEvent> &p_ev) {
	if (!owner) {
		return;
	}
	Rect2 frect = owner->_compute_frame_preview_rect();
	if (frect.size.x <= 4.0f || frect.size.y <= 4.0f) {
		return;
	}
	Ref<Texture2D> tex;
	if (owner->active_state_idx >= 0 && owner->active_state_idx < owner->state_slots.size()) {
		tex = owner->state_slots[owner->active_state_idx].texture;
	}

	Ref<InputEventMouseButton> mb = p_ev;
	if (mb.is_valid()) {
		if (mb->is_pressed() && (mb->get_button_index() == MouseButton::WHEEL_UP || mb->get_button_index() == MouseButton::WHEEL_DOWN)) {
			double factor = (mb->get_button_index() == MouseButton::WHEEL_UP) ? 1.1 : (1.0 / 1.1);
			double sx = double(owner->edit_vals.get("manual_scale_x_pct", 100.0));
			double sy = double(owner->edit_vals.get("manual_scale_y_pct", sx));
			bool keep = bool(owner->edit_vals.get("manual_keep_aspect", true));
			bool is_int = bool(owner->edit_vals.get("manual_integer_scale", false));
			if (is_int && tex.is_valid()) {
				float base_s = MIN(owner->logical_frame_size.x / MAX(1.0f, tex->get_size().x), owner->logical_frame_size.y / MAX(1.0f, tex->get_size().y));
				double cur_mult = Math::round((sx / 100.0) * double(base_s));
				cur_mult = (mb->get_button_index() == MouseButton::WHEEL_UP) ? (cur_mult + 1.0) : MAX(1.0, cur_mult - 1.0);
				double ns = CLAMP((cur_mult / MAX(0.001, double(base_s))) * 100.0, 5.0, 1000.0);
				owner->edit_vals["manual_scale_x_pct"] = ns;
				owner->edit_vals["manual_scale_y_pct"] = keep ? ns : owner->_snap_scale_if_integer(sy * factor, tex);
			} else {
				double nsx = CLAMP(sx * factor, 5.0, 1000.0);
				owner->edit_vals["manual_scale_x_pct"] = nsx;
				owner->edit_vals["manual_scale_y_pct"] = keep ? nsx : CLAMP(sy * factor, 5.0, 1000.0);
			}
			owner->_sync_controls_from_vals();
			owner->_redraw_canvas();
			accept_event();
			return;
		}
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				grab_focus();
				drag_start_mouse = mb->get_position();
				drag_start_vals = owner->edit_vals.duplicate(true);
				drag_mode = 1; // move
				if (tex.is_valid()) {
					Rect2 img_rect = owner->_compute_image_preview_rect(frect, owner->edit_vals);
					int hit = ClockGearsImageAdjustDialog::hit_test_corner_handle(mb->get_position(), ClockGearsImageAdjustDialog::get_corner_handles(img_rect), 14.0f);
					if (hit >= 0) {
						drag_mode = 2 + hit;
					}
				}
			} else {
				drag_mode = 0;
			}
			accept_event();
			return;
		}
	}

	Ref<InputEventMouseMotion> mm = p_ev;
	if (mm.is_valid() && drag_mode > 0) {
		if (drag_mode == 1) {
			Vector2 delta = mm->get_position() - drag_start_mouse;
			double dx_pct = (double(delta.x) / double(frect.size.x)) * 100.0;
			double dy_pct = (double(delta.y) / double(frect.size.y)) * 100.0;
			owner->edit_vals["manual_offset_x_pct"] = CLAMP(double(drag_start_vals.get("manual_offset_x_pct", 0.0)) + dx_pct, -400.0, 400.0);
			owner->edit_vals["manual_offset_y_pct"] = CLAMP(double(drag_start_vals.get("manual_offset_y_pct", 0.0)) + dy_pct, -400.0, 400.0);
		} else if (drag_mode >= 2 && drag_mode <= 5 && tex.is_valid()) {
			Rect2 start_r = owner->_compute_image_preview_rect(frect, drag_start_vals);
			Point2 center = start_r.get_center();
			bool keep = bool(owner->edit_vals.get("manual_keep_aspect", true));
			double base_sx = double(drag_start_vals.get("manual_scale_x_pct", 100.0));
			double base_sy = double(drag_start_vals.get("manual_scale_y_pct", base_sx));
			if (keep) {
				float d0 = MAX(10.0f, drag_start_mouse.distance_to(center));
				float d1 = MAX(4.0f, mm->get_position().distance_to(center));
				double ns = CLAMP(base_sx * (double(d1) / double(d0)), 5.0, 1000.0);
				ns = owner->_snap_scale_if_integer(ns, tex);
				owner->edit_vals["manual_scale_x_pct"] = ns;
				owner->edit_vals["manual_scale_y_pct"] = ns;
			} else {
				float dx0 = MAX(8.0f, Math::abs(drag_start_mouse.x - center.x));
				float dy0 = MAX(8.0f, Math::abs(drag_start_mouse.y - center.y));
				float dx1 = MAX(4.0f, Math::abs(mm->get_position().x - center.x));
				float dy1 = MAX(4.0f, Math::abs(mm->get_position().y - center.y));
				double nsx = owner->_snap_scale_if_integer(CLAMP(base_sx * (double(dx1) / double(dx0)), 5.0, 1000.0), tex);
				double nsy = owner->_snap_scale_if_integer(CLAMP(base_sy * (double(dy1) / double(dy0)), 5.0, 1000.0), tex);
				owner->edit_vals["manual_scale_x_pct"] = nsx;
				owner->edit_vals["manual_scale_y_pct"] = nsy;
			}
		}
		owner->_sync_controls_from_vals();
		owner->_redraw_canvas();
		accept_event();
	}
}

void ClockGearsImageAdjustDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_values"), &ClockGearsImageAdjustDialog::get_values);
	ClassDB::bind_method(D_METHOD("set_values", "vals"), &ClockGearsImageAdjustDialog::set_values);
	ClassDB::bind_method(D_METHOD("confirm_adjust"), &ClockGearsImageAdjustDialog::confirm_adjust);
	ClassDB::bind_method(D_METHOD("get_ui_handles"), &ClockGearsImageAdjustDialog::get_ui_handles);
}

Vector<Point2> ClockGearsImageAdjustDialog::get_corner_handles(const Rect2 &p_rect) {
	Vector<Point2> pts;
	pts.push_back(p_rect.position);
	pts.push_back(Point2(p_rect.position.x + p_rect.size.x, p_rect.position.y));
	pts.push_back(p_rect.position + p_rect.size);
	pts.push_back(Point2(p_rect.position.x, p_rect.position.y + p_rect.size.y));
	return pts;
}

int ClockGearsImageAdjustDialog::hit_test_corner_handle(const Point2 &p_mouse_pos, const Vector<Point2> &p_handles, float p_radius) {
	for (int i = 0; i < p_handles.size(); i++) {
		if (p_mouse_pos.distance_to(p_handles[i]) <= p_radius) {
			return i;
		}
	}
	return -1;
}

ClockGearsImageAdjustDialog::ClockGearsImageAdjustDialog() {
	set_name("ClockGearsImageAdjustDialog");
	set_title(String::utf8("Ajustar imagen manualmente"));
	set_exclusive(true);
	set_min_size(Size2i(int(720 * EDSCALE), int(560 * EDSCALE)));
	get_ok_button()->set_text("Aceptar");
	add_cancel_button("Cancelar");
	connect("confirmed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_confirmed));
	connect("window_input", callable_mp(this, &ClockGearsImageAdjustDialog::_on_window_input));
	_build_ui();
}

void ClockGearsImageAdjustDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		_refresh_theme();
	}
}

void ClockGearsImageAdjustDialog::_refresh_theme() {
	if (ClockGearsTheme *ct = ClockGearsTheme::get_singleton()) {
		if (bg_root) {
			ct->apply_to_control(bg_root);
			bg_root->set_theme_type_variation("ClockGearsContainer");
		}
		if (!ct->is_connected("theme_changed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_theme_changed).unbind(1))) {
			ct->connect("theme_changed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_theme_changed).unbind(1));
		}
	}
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_theme_changed() {
	_refresh_theme();
}

void ClockGearsImageAdjustDialog::_build_ui() {
	bg_root = memnew(PanelContainer);
	bg_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(bg_root);

	MarginContainer *mc = memnew(MarginContainer);
	mc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	mc->add_theme_constant_override("margin_left", int(10 * EDSCALE));
	mc->add_theme_constant_override("margin_right", int(10 * EDSCALE));
	mc->add_theme_constant_override("margin_top", int(8 * EDSCALE));
	mc->add_theme_constant_override("margin_bottom", int(8 * EDSCALE));
	bg_root->add_child(mc);

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vb->add_theme_constant_override("separation", int(8 * EDSCALE));
	mc->add_child(vb);

	// Barra de herramientas superior
	HFlowContainer *tools = memnew(HFlowContainer);
	tools->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	tools->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	vb->add_child(tools);

	state_bar = memnew(HFlowContainer);
	state_bar->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	state_bar->hide();
	tools->add_child(state_bar);

	keep_aspect_chk = memnew(CheckBox(String::utf8("Mantener proporción")));
	keep_aspect_chk->set_clip_text(false);
	keep_aspect_chk->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	keep_aspect_chk->connect("toggled", callable_mp(this, &ClockGearsImageAdjustDialog::_on_keep_aspect_toggled));
	tools->add_child(keep_aspect_chk);

	integer_scale_chk = memnew(CheckBox("Escala entera (pixel art)"));
	integer_scale_chk->set_clip_text(false);
	integer_scale_chk->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	integer_scale_chk->connect("toggled", callable_mp(this, &ClockGearsImageAdjustDialog::_on_integer_scale_toggled));
	tools->add_child(integer_scale_chk);

	center_btn = memnew(Button("Centrar"));
	center_btn->set_clip_text(false);
	center_btn->set_custom_minimum_size(Size2(84 * EDSCALE, 32 * EDSCALE));
	center_btn->connect("pressed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_center_pressed));
	tools->add_child(center_btn);

	fit_frame_btn = memnew(Button("Ajustar al marco"));
	fit_frame_btn->set_clip_text(false);
	fit_frame_btn->set_custom_minimum_size(Size2(132 * EDSCALE, 32 * EDSCALE));
	fit_frame_btn->connect("pressed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_fit_frame_pressed));
	tools->add_child(fit_frame_btn);

	reset_btn = memnew(Button("Restablecer"));
	reset_btn->set_clip_text(false);
	reset_btn->set_custom_minimum_size(Size2(104 * EDSCALE, 32 * EDSCALE));
	reset_btn->connect("pressed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_reset_pressed));
	tools->add_child(reset_btn);

	// Lienzo interactivo
	PanelContainer *frame_pc = memnew(PanelContainer);
	frame_pc->set_theme_type_variation("ClockGearsPreviewFrame");
	frame_pc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frame_pc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	frame_pc->set_custom_minimum_size(Size2(640 * EDSCALE, 340 * EDSCALE));
	vb->add_child(frame_pc);

	canvas = memnew(ClockGearsImageAdjustCanvas);
	canvas->set_owner_dialog(this);
	canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	frame_pc->add_child(canvas);

	// Campos numéricos opcionales en % relativo al marco
	HFlowContainer *num_hb = memnew(HFlowContainer);
	num_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	num_hb->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	num_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	vb->add_child(num_hb);

	auto make_num = [&](const String &p_lbl, double p_min, double p_max, double p_step, SpinBox *&r_sp, Label **r_lbl_out = nullptr) {
		HBoxContainer *item = memnew(HBoxContainer);
		item->add_theme_constant_override("separation", int(4 * EDSCALE));
		Label *lb = memnew(Label(p_lbl));
		lb->set_clip_text(false);
		item->add_child(lb);
		if (r_lbl_out) {
			*r_lbl_out = lb;
		}
		r_sp = memnew(SpinBox);
		r_sp->set_min(p_min);
		r_sp->set_max(p_max);
		r_sp->set_step(p_step);
		r_sp->set_suffix("%");
		r_sp->set_custom_minimum_size(Size2(104 * EDSCALE, 32 * EDSCALE));
		r_sp->connect("value_changed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_spin_changed));
		item->add_child(r_sp);
		num_hb->add_child(item);
	};

	make_num("Pos X (%):", -400.0, 400.0, 0.5, ox_spin);
	make_num("Pos Y (%):", -400.0, 400.0, 0.5, oy_spin);
	make_num("Escala X (%):", 5.0, 1000.0, 1.0, sx_spin);
	make_num("Escala Y (%):", 5.0, 1000.0, 1.0, sy_spin, &sy_lbl);

	info_lbl = memnew(Label(String::utf8("Arrastra para mover · Esquinas o rueda para escalar · Flechas 1 px (Mayús 8 px)")));
	info_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	vb->add_child(info_lbl);
}

void ClockGearsImageAdjustDialog::open_adjust(
		const String &p_title,
		const Vector2 &p_logical_frame_sz,
		const Dictionary &p_shape_cfg,
		const Dictionary &p_current_cfg,
		const Vector<Pair<String, String>> &p_state_images,
		bool p_default_pixel_art,
		const Callable &p_on_confirm) {
	set_title(p_title.is_empty() ? String::utf8("Ajustar imagen manualmente") : p_title);
	logical_frame_size = Vector2(MAX(16.0f, p_logical_frame_sz.x), MAX(16.0f, p_logical_frame_sz.y));
	shape_cfg = p_shape_cfg.duplicate(true);
	shape_cfg["pixel_art"] = p_default_pixel_art;
	on_confirm_cb = p_on_confirm;

	// Build state preview slots
	state_slots.clear();
	for (int i = 0; i < state_btns.size(); i++) {
		if (state_btns[i]) {
			state_btns[i]->queue_free();
		}
	}
	state_btns.clear();

	for (int i = 0; i < p_state_images.size(); i++) {
		String p = p_state_images[i].second.strip_edges();
		if (p.is_empty()) {
			continue;
		}
		Ref<Texture2D> tex = ClockGearsWindowStyleManager::load_texture_safe(p);
		if (tex.is_valid()) {
			StateImageSlot slot;
			slot.label = p_state_images[i].first;
			slot.path = p;
			slot.texture = tex;
			state_slots.push_back(slot);
		}
	}
	active_state_idx = 0;
	if (state_bar) {
		state_bar->set_visible(state_slots.size() > 1);
		if (state_slots.size() > 1) {
			for (int i = 0; i < state_slots.size(); i++) {
				Button *sb = memnew(Button(state_slots[i].label));
				sb->set_toggle_mode(true);
				sb->set_pressed(i == 0);
				sb->set_clip_text(false);
				sb->set_custom_minimum_size(Size2(72 * EDSCALE, 30 * EDSCALE));
				sb->connect("pressed", callable_mp(this, &ClockGearsImageAdjustDialog::_on_state_btn_pressed).bind(i));
				state_bar->add_child(sb);
				state_btns.push_back(sb);
			}
		}
	}

	Dictionary vals;
	vals["manual_offset_x_pct"] = CLAMP(double(p_current_cfg.get("manual_offset_x_pct", 0.0)), -400.0, 400.0);
	vals["manual_offset_y_pct"] = CLAMP(double(p_current_cfg.get("manual_offset_y_pct", 0.0)), -400.0, 400.0);
	vals["manual_scale_x_pct"] = CLAMP(double(p_current_cfg.get("manual_scale_x_pct", 100.0)), 5.0, 1000.0);
	vals["manual_scale_y_pct"] = CLAMP(double(p_current_cfg.get("manual_scale_y_pct", vals["manual_scale_x_pct"])), 5.0, 1000.0);
	vals["manual_keep_aspect"] = p_current_cfg.has("manual_keep_aspect") ? bool(p_current_cfg["manual_keep_aspect"]) : true;
	vals["manual_integer_scale"] = p_current_cfg.has("manual_integer_scale") ? bool(p_current_cfg["manual_integer_scale"]) : p_default_pixel_art;
	initial_vals = vals.duplicate(true);
	edit_vals = vals.duplicate(true);

	_sync_controls_from_vals();
	popup_centered(Size2i(int(820 * EDSCALE), int(620 * EDSCALE)));
	_refresh_theme();
	if (canvas) {
		canvas->grab_focus();
	}
}

void ClockGearsImageAdjustDialog::set_values(const Dictionary &p_vals) {
	if (p_vals.has("manual_offset_x_pct")) edit_vals["manual_offset_x_pct"] = CLAMP(double(p_vals["manual_offset_x_pct"]), -400.0, 400.0);
	if (p_vals.has("manual_offset_y_pct")) edit_vals["manual_offset_y_pct"] = CLAMP(double(p_vals["manual_offset_y_pct"]), -400.0, 400.0);
	if (p_vals.has("manual_scale_x_pct")) edit_vals["manual_scale_x_pct"] = CLAMP(double(p_vals["manual_scale_x_pct"]), 5.0, 1000.0);
	if (p_vals.has("manual_scale_y_pct")) edit_vals["manual_scale_y_pct"] = CLAMP(double(p_vals["manual_scale_y_pct"]), 5.0, 1000.0);
	if (p_vals.has("manual_keep_aspect")) edit_vals["manual_keep_aspect"] = bool(p_vals["manual_keep_aspect"]);
	if (p_vals.has("manual_integer_scale")) edit_vals["manual_integer_scale"] = bool(p_vals["manual_integer_scale"]);
	_sync_controls_from_vals();
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::confirm_adjust() {
	_on_confirmed();
	hide();
}

Rect2 ClockGearsImageAdjustDialog::_compute_frame_preview_rect() const {
	if (!canvas) {
		return Rect2(0, 0, 320, 180);
	}
	Size2 csz = canvas->get_size();
	float pad = 34.0f * EDSCALE;
	Size2 avail(MAX(32.0f, csz.x - pad * 2.0f), MAX(32.0f, csz.y - pad * 2.0f));
	float s = MIN(avail.x / MAX(1.0f, logical_frame_size.x), avail.y / MAX(1.0f, logical_frame_size.y));
	Size2 fsz = logical_frame_size * s;
	return Rect2((csz - fsz) * 0.5f, fsz);
}

Rect2 ClockGearsImageAdjustDialog::_compute_image_preview_rect(const Rect2 &p_frame_rect, const Dictionary &p_vals) const {
	if (active_state_idx < 0 || active_state_idx >= state_slots.size() || state_slots[active_state_idx].texture.is_null()) {
		return p_frame_rect;
	}
	return ClockGearsUiDesignLayer::compute_manual_image_rect(p_frame_rect, state_slots[active_state_idx].texture->get_size(), p_vals);
}

double ClockGearsImageAdjustDialog::_snap_scale_if_integer(double p_scale_pct, const Ref<Texture2D> &p_tex) const {
	if (!bool(edit_vals.get("manual_integer_scale", false)) || p_tex.is_null()) {
		return p_scale_pct;
	}
	Vector2 ts = p_tex->get_size();
	if (ts.x <= 0.0f || ts.y <= 0.0f) {
		return p_scale_pct;
	}
	float base_s = MIN(logical_frame_size.x / ts.x, logical_frame_size.y / ts.y);
	if (base_s <= 0.0001f) {
		return p_scale_pct;
	}
	double mult = MAX(1.0, Math::round((p_scale_pct / 100.0) * double(base_s)));
	return CLAMP((mult / double(base_s)) * 100.0, 5.0, 1000.0);
}

void ClockGearsImageAdjustDialog::_sync_controls_from_vals() {
	bool prev = updating_controls;
	updating_controls = true;
	bool keep = bool(edit_vals.get("manual_keep_aspect", true));
	bool is_int = bool(edit_vals.get("manual_integer_scale", false));
	if (keep_aspect_chk) keep_aspect_chk->set_pressed(keep);
	if (integer_scale_chk) integer_scale_chk->set_pressed(is_int);
	if (ox_spin) ox_spin->set_value(double(edit_vals.get("manual_offset_x_pct", 0.0)));
	if (oy_spin) oy_spin->set_value(double(edit_vals.get("manual_offset_y_pct", 0.0)));
	if (sx_spin) sx_spin->set_value(double(edit_vals.get("manual_scale_x_pct", 100.0)));
	if (sy_spin) {
		sy_spin->set_value(keep ? double(edit_vals.get("manual_scale_x_pct", 100.0)) : double(edit_vals.get("manual_scale_y_pct", 100.0)));
		sy_spin->set_editable(!keep);
	}
	if (sy_lbl) {
		sy_lbl->set_modulate(keep ? Color(1, 1, 1, 0.5f) : Color(1, 1, 1, 1));
	}
	updating_controls = prev;
}

void ClockGearsImageAdjustDialog::_redraw_canvas() {
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsImageAdjustDialog::_on_state_btn_pressed(int p_idx) {
	active_state_idx = CLAMP(p_idx, 0, MAX(0, state_slots.size() - 1));
	for (int i = 0; i < state_btns.size(); i++) {
		if (state_btns[i]) state_btns[i]->set_pressed(i == active_state_idx);
	}
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_keep_aspect_toggled(bool p_pressed) {
	if (updating_controls) return;
	edit_vals["manual_keep_aspect"] = p_pressed;
	if (p_pressed) {
		edit_vals["manual_scale_y_pct"] = edit_vals.get("manual_scale_x_pct", 100.0);
	}
	_sync_controls_from_vals();
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_integer_scale_toggled(bool p_pressed) {
	if (updating_controls) return;
	edit_vals["manual_integer_scale"] = p_pressed;
	if (p_pressed && active_state_idx >= 0 && active_state_idx < state_slots.size() && state_slots[active_state_idx].texture.is_valid()) {
		Ref<Texture2D> t = state_slots[active_state_idx].texture;
		double nsx = _snap_scale_if_integer(double(edit_vals.get("manual_scale_x_pct", 100.0)), t);
		edit_vals["manual_scale_x_pct"] = nsx;
		edit_vals["manual_scale_y_pct"] = bool(edit_vals.get("manual_keep_aspect", true)) ? nsx : _snap_scale_if_integer(double(edit_vals.get("manual_scale_y_pct", 100.0)), t);
	}
	_sync_controls_from_vals();
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_center_pressed() {
	edit_vals["manual_offset_x_pct"] = 0.0;
	edit_vals["manual_offset_y_pct"] = 0.0;
	_sync_controls_from_vals();
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_fit_frame_pressed() {
	edit_vals["manual_offset_x_pct"] = 0.0;
	edit_vals["manual_offset_y_pct"] = 0.0;
	edit_vals["manual_scale_x_pct"] = 100.0;
	edit_vals["manual_scale_y_pct"] = 100.0;
	_sync_controls_from_vals();
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_reset_pressed() {
	edit_vals = initial_vals.duplicate(true);
	_sync_controls_from_vals();
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_spin_changed(double /*p_val*/) {
	if (updating_controls) return;
	if (ox_spin) edit_vals["manual_offset_x_pct"] = ox_spin->get_value();
	if (oy_spin) edit_vals["manual_offset_y_pct"] = oy_spin->get_value();
	if (sx_spin) edit_vals["manual_scale_x_pct"] = sx_spin->get_value();
	if (bool(edit_vals.get("manual_keep_aspect", true))) {
		edit_vals["manual_scale_y_pct"] = edit_vals.get("manual_scale_x_pct", 100.0);
		if (sy_spin) {
			bool prev = updating_controls;
			updating_controls = true;
			sy_spin->set_value(double(edit_vals["manual_scale_y_pct"]));
			updating_controls = prev;
		}
	} else if (sy_spin) {
		edit_vals["manual_scale_y_pct"] = sy_spin->get_value();
	}
	_redraw_canvas();
}

void ClockGearsImageAdjustDialog::_on_confirmed() {
	Dictionary out = edit_vals.duplicate(true);
	out["scale_mode"] = "manual";
	if (on_confirm_cb.is_valid()) {
		on_confirm_cb.call(out);
	}
}

void ClockGearsImageAdjustDialog::_on_window_input(const Ref<InputEvent> &p_ev) {
	Ref<InputEventKey> k = p_ev;
	if (!k.is_valid() || !k->is_pressed()) {
		return;
	}
	double step_px = k->is_shift_pressed() ? 8.0 : 1.0;
	double step_x_pct = (step_px / double(MAX(1.0f, logical_frame_size.x))) * 100.0;
	double step_y_pct = (step_px / double(MAX(1.0f, logical_frame_size.y))) * 100.0;
	bool moved = false;
	switch (k->get_keycode()) {
		case Key::LEFT:
			edit_vals["manual_offset_x_pct"] = CLAMP(double(edit_vals.get("manual_offset_x_pct", 0.0)) - step_x_pct, -400.0, 400.0);
			moved = true;
			break;
		case Key::RIGHT:
			edit_vals["manual_offset_x_pct"] = CLAMP(double(edit_vals.get("manual_offset_x_pct", 0.0)) + step_x_pct, -400.0, 400.0);
			moved = true;
			break;
		case Key::UP:
			edit_vals["manual_offset_y_pct"] = CLAMP(double(edit_vals.get("manual_offset_y_pct", 0.0)) - step_y_pct, -400.0, 400.0);
			moved = true;
			break;
		case Key::DOWN:
			edit_vals["manual_offset_y_pct"] = CLAMP(double(edit_vals.get("manual_offset_y_pct", 0.0)) + step_y_pct, -400.0, 400.0);
			moved = true;
			break;
		default:
			break;
	}
	if (moved) {
		_sync_controls_from_vals();
		_redraw_canvas();
		set_input_as_handled();
	}
}

Dictionary ClockGearsImageAdjustDialog::get_ui_handles() const {
	Dictionary out;
	out["canvas"] = canvas;
	out["keep_aspect_chk"] = keep_aspect_chk;
	out["integer_scale_chk"] = integer_scale_chk;
	out["center_btn"] = center_btn;
	out["fit_frame_btn"] = fit_frame_btn;
	out["reset_btn"] = reset_btn;
	out["ox_spin"] = ox_spin;
	out["oy_spin"] = oy_spin;
	out["sx_spin"] = sx_spin;
	out["sy_spin"] = sy_spin;
	return out;
}
