#include "clockgears_credits.h"
#include "../core/clockgears_data_manager.h"
#include "clockgears_audio_manager.h"
#include "clockgears_title_screen.h"
#include "clockgears_ui_design_layer.h"

#include "core/config/engine.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/object/callable_mp.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"

static HorizontalAlignment _cg_h_align(const String &p_align) {
	String a = p_align.strip_edges().to_lower();
	if (a == "left" || a == "izquierda") {
		return HORIZONTAL_ALIGNMENT_LEFT;
	}
	if (a == "right" || a == "derecha") {
		return HORIZONTAL_ALIGNMENT_RIGHT;
	}
	return HORIZONTAL_ALIGNMENT_CENTER;
}

static void _cg_style_credit_label(Label *p_lbl, const Dictionary &p_style, const String &p_default_align, const String &p_fallback_col) {
	if (!p_lbl) {
		return;
	}
	int fsz = CLAMP(int(p_style.get("font_size", 16)), 8, 96);
	String col_str = String(p_style.get("color", p_fallback_col)).strip_edges();
	Color col = Color::html_is_valid(col_str) ? Color::html(col_str) : Color::html(p_fallback_col);
	String al = String(p_style.get("align", "")).strip_edges();
	if (al.is_empty()) {
		al = p_default_align;
	}
	p_lbl->set_horizontal_alignment(_cg_h_align(al));
	p_lbl->add_theme_font_size_override("font_size", fsz);
	p_lbl->add_theme_color_override("font_color", col);
}

void ClockGearsCreditsPlayer::_ensure_ui_nodes() {
	if (stage_clip) {
		return;
	}
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_focus_mode(FOCUS_ALL);

	bg_color_rect = memnew(ColorRect);
	bg_color_rect->set_name("CreditsBgColor");
	bg_color_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	bg_color_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(bg_color_rect);

	bg_texture_rect = memnew(TextureRect);
	bg_texture_rect->set_name("CreditsBgTexture");
	bg_texture_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	bg_texture_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg_texture_rect->set_visible(false);
	add_child(bg_texture_rect);

	stage_clip = memnew(Control);
	stage_clip->set_name("CreditsStage");
	stage_clip->set_clip_contents(true);
	stage_clip->set_mouse_filter(MOUSE_FILTER_PASS);
	stage_clip->set_size(Size2(LOGICAL_WIDTH, LOGICAL_HEIGHT));
	add_child(stage_clip);

	scroll_vbox = memnew(VBoxContainer);
	scroll_vbox->set_name("CreditsScrollVBox");
	scroll_vbox->set_position(Vector2(50.0f, LOGICAL_HEIGHT));
	scroll_vbox->set_size(Size2(LOGICAL_WIDTH - 100.0f, 10.0f));
	scroll_vbox->add_theme_constant_override("separation", 10);
	stage_clip->add_child(scroll_vbox);

	custom_ui_layer = memnew(ClockGearsUiDesignLayer);
	custom_ui_layer->set_name("CustomCreditsUiLayer");
	custom_ui_layer->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	custom_ui_layer->set_window_type_fallback("game_menu");
	custom_ui_layer->set_visible(false);
	custom_ui_layer->connect("action_triggered", callable_mp(this, &ClockGearsCreditsPlayer::_on_custom_ui_action));
	add_child(custom_ui_layer);
}

void ClockGearsCreditsPlayer::_clear_scroll_items() {
	if (!scroll_vbox) {
		return;
	}
	for (int i = scroll_vbox->get_child_count() - 1; i >= 0; i--) {
		Node *ch = scroll_vbox->get_child(i);
		scroll_vbox->remove_child(ch);
		memdelete(ch);
	}
}

void ClockGearsCreditsPlayer::_rebuild_from_config(const Dictionary &p_cfg) {
	_ensure_ui_nodes();
	_clear_scroll_items();

	active_config = ClockGearsDataManager::normalize_credits_config(p_cfg);
	Dictionary bg = active_config.get("background", Dictionary());
	ClockGearsTitleScreen::apply_screen_background(bg_color_rect, bg_texture_rect, bg, "#090b12ff");

	scroll_speed = CLAMP(float(active_config.get("scroll_speed", 60.0)), 5.0f, 600.0f);
	String def_align = String(active_config.get("alignment", "center"));
	Dictionary styles = active_config.get("styles", Dictionary());
	Dictionary st_sec = styles.get("section", Dictionary());
	Dictionary st_role = styles.get("role", Dictionary());
	Dictionary st_name = styles.get("name", Dictionary());
	Dictionary st_text = styles.get("text", Dictionary());

	Array items = active_config.get("items", Array());
	const float inner_width = LOGICAL_WIDTH - 100.0f;
	float est_height = 0.0f;
	int visible_items = 0;
	const int sep = 10;

	for (int i = 0; i < items.size(); i++) {
		if (items[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary item = items[i];
		String itype = String(item.get("type", "text"));
		String item_align = String(item.get("align", def_align));

		if (itype == "section") {
			String txt = String(item.get("text", ""));
			Label *lbl = memnew(Label);
			lbl->set_text(txt);
			lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
			lbl->set_custom_minimum_size(Size2(inner_width, 0.0f));
			_cg_style_credit_label(lbl, st_sec, item_align, "#ffd97dff");
			scroll_vbox->add_child(lbl);
			int fsz = int(st_sec.get("font_size", 26));
			est_height += MAX(24.0f, float(fsz) * 1.45f) + (visible_items > 0 ? sep : 0);
			visible_items++;
		} else if (itype == "role_name") {
			String role_str = String(item.get("role", ""));
			String name_str = String(item.get("name", ""));
			VBoxContainer *pair = memnew(VBoxContainer);
			pair->add_theme_constant_override("separation", 2);
			pair->set_custom_minimum_size(Size2(inner_width, 0.0f));
			float pair_h = 0.0f;
			if (!role_str.strip_edges().is_empty()) {
				Label *rlbl = memnew(Label);
				rlbl->set_text(role_str);
				rlbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
				rlbl->set_custom_minimum_size(Size2(inner_width, 0.0f));
				_cg_style_credit_label(rlbl, st_role, item_align, "#9fb4d9ff");
				pair->add_child(rlbl);
				pair_h += MAX(18.0f, float(int(st_role.get("font_size", 14))) * 1.35f);
			}
			if (!name_str.strip_edges().is_empty()) {
				Label *nlbl = memnew(Label);
				nlbl->set_text(name_str);
				nlbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
				nlbl->set_custom_minimum_size(Size2(inner_width, 0.0f));
				_cg_style_credit_label(nlbl, st_name, item_align, "#ffffffff");
				pair->add_child(nlbl);
				pair_h += MAX(22.0f, float(int(st_name.get("font_size", 20))) * 1.4f);
			}
			scroll_vbox->add_child(pair);
			est_height += MAX(22.0f, pair_h) + (visible_items > 0 ? sep : 0);
			visible_items++;
		} else if (itype == "text") {
			String txt = String(item.get("text", ""));
			Label *lbl = memnew(Label);
			lbl->set_text(txt);
			lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
			lbl->set_custom_minimum_size(Size2(inner_width, 0.0f));
			_cg_style_credit_label(lbl, st_text, item_align, "#dce4f2ff");
			scroll_vbox->add_child(lbl);
			int fsz = int(st_text.get("font_size", 16));
			int lines = MAX(1, txt.split("\n").size());
			est_height += float(lines) * MAX(20.0f, float(fsz) * 1.4f) + (visible_items > 0 ? sep : 0);
			visible_items++;
		} else if (itype == "image") {
			String ipath = String(item.get("image_path", "")).strip_edges();
			float max_w = CLAMP(float(item.get("max_width", 420.0)), 16.0f, inner_width);
			float max_h = CLAMP(float(item.get("max_height", 220.0)), 16.0f, LOGICAL_HEIGHT);
			Ref<Texture2D> tex = ClockGearsTitleScreen::load_runtime_texture(ipath);
			Size2 disp_sz(MIN(max_w, 160.0f), MIN(max_h, 96.0f));
			if (tex.is_valid()) {
				Size2 ts = tex->get_size();
				if (ts.x > 0.0f && ts.y > 0.0f) {
					float fit_s = MIN(1.0f, MIN(max_w / ts.x, max_h / ts.y));
					disp_sz = Size2(MAX(8.0f, ts.x * fit_s), MAX(8.0f, ts.y * fit_s));
				}
			}
			HBoxContainer *row = memnew(HBoxContainer);
			row->set_custom_minimum_size(Size2(inner_width, disp_sz.y));
			HorizontalAlignment hal = _cg_h_align(item_align);
			if (hal == HORIZONTAL_ALIGNMENT_CENTER || hal == HORIZONTAL_ALIGNMENT_RIGHT) {
				Control *sp_left = memnew(Control);
				sp_left->set_h_size_flags(SIZE_EXPAND_FILL);
				row->add_child(sp_left);
			}
			TextureRect *tr = memnew(TextureRect);
			tr->set_texture(tex);
			tr->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			tr->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			tr->set_custom_minimum_size(disp_sz);
			String flt = String(item.get("filter", "nearest")).strip_edges().to_lower();
			tr->set_texture_filter(flt == "linear" ? TEXTURE_FILTER_LINEAR : TEXTURE_FILTER_NEAREST);
			row->add_child(tr);
			if (hal == HORIZONTAL_ALIGNMENT_CENTER || hal == HORIZONTAL_ALIGNMENT_LEFT) {
				Control *sp_right = memnew(Control);
				sp_right->set_h_size_flags(SIZE_EXPAND_FILL);
				row->add_child(sp_right);
			}
			scroll_vbox->add_child(row);
			est_height += disp_sz.y + (visible_items > 0 ? sep : 0);
			visible_items++;
		} else if (itype == "spacer") {
			float sh = CLAMP(float(item.get("height", 32.0)), 4.0f, 480.0f);
			Control *sp = memnew(Control);
			sp->set_custom_minimum_size(Size2(inner_width, sh));
			scroll_vbox->add_child(sp);
			est_height += sh + (visible_items > 0 ? sep : 0);
			visible_items++;
		}
	}

	item_count_rendered = visible_items;
	if (preview_mode && visible_items == 0) {
		Label *empty_lbl = memnew(Label);
		empty_lbl->set_text(String::utf8("(Sin líneas de créditos configuradas)"));
		empty_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		empty_lbl->set_custom_minimum_size(Size2(inner_width, 0.0f));
		empty_lbl->add_theme_font_size_override("font_size", 16);
		empty_lbl->add_theme_color_override("font_color", Color(0.65f, 0.72f, 0.84f, 0.75f));
		scroll_vbox->add_child(empty_lbl);
		est_height = 28.0f;
	}
	Size2 min_sz = scroll_vbox->get_combined_minimum_size();
	content_height = MAX(est_height, min_sz.y);
	scroll_vbox->set_size(Size2(inner_width, content_height));
	scroll_y = LOGICAL_HEIGHT;
	scroll_vbox->set_position(Vector2(50.0f, scroll_y));

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (custom_ui_layer) {
		if (dm && dm->has_custom_ui_design("credits")) {
			custom_ui_layer->set_window_type_fallback("game_menu");
			custom_ui_layer->set_preview_mode(preview_mode);
			custom_ui_layer->apply_screen_design("credits");
			custom_ui_layer->set_visible(true);
			if (!preview_mode) {
				custom_ui_layer->play_transition_in();
				Vector<Button *> cbtns = custom_ui_layer->get_focus_ordered_buttons();
				if (!cbtns.is_empty() && cbtns[0] && is_inside_tree()) {
					cbtns[0]->grab_focus();
					custom_ui_layer->update_cursor_for_button(cbtns[0]);
				}
			}
		} else {
			custom_ui_layer->set_visible(false);
			custom_ui_layer->clear_elements();
		}
	}
}

void ClockGearsCreditsPlayer::set_preview_mode(bool p_preview) {
	preview_mode = p_preview;
	if (preview_mode) {
		playing = false;
		set_process(false);
		set_process_input(false);
		set_mouse_filter(MOUSE_FILTER_IGNORE);
	} else {
		set_mouse_filter(MOUSE_FILTER_STOP);
	}
}

void ClockGearsCreditsPlayer::apply_credits_config_preview(const Dictionary &p_cfg, float p_scroll_y) {
	set_preview_mode(true);
	_ensure_ui_nodes();
	_rebuild_from_config(p_cfg);
	_update_responsive_stage();

	completed_naturally = false;
	cancelled_exit = false;
	playing = false;
	scroll_y = p_scroll_y;
	if (scroll_vbox) {
		scroll_vbox->set_position(Vector2(50.0f, scroll_y));
	}
	set_visible(true);
}

Dictionary ClockGearsCreditsPlayer::get_preview_snapshot() const {
	Dictionary out;
	Dictionary bg = active_config.get("background", Dictionary());
	out["bg_type"] = String(bg.get("type", "color"));
	out["bg_color"] = String(bg.get("color", "#090b12ff"));
	out["bg_image_path"] = String(bg.get("image_path", ""));
	out["bg_texture_visible"] = bg_texture_rect ? bg_texture_rect->is_visible() : false;
	out["scroll_speed"] = scroll_speed;
	out["alignment"] = String(active_config.get("alignment", "center"));
	out["bgm_path"] = String(active_config.get("bgm_path", ""));
	out["item_count"] = item_count_rendered;
	out["scroll_y"] = scroll_y;
	out["content_height"] = content_height;
	out["styles"] = Dictionary(active_config.get("styles", Dictionary())).duplicate(true);
	return out;
}

void ClockGearsCreditsPlayer::_update_responsive_stage(const Vector2 &p_override_vp_size) {
	if (!stage_clip) {
		return;
	}
	Vector2 vp_size = p_override_vp_size;
	if (vp_size.x <= 1.0f || vp_size.y <= 1.0f) {
		if (preview_mode && get_size().x > 1.0f && get_size().y > 1.0f) {
			vp_size = get_size();
		} else if (is_inside_tree() && get_viewport()) {
			vp_size = get_viewport()->get_visible_rect().size;
		}
	}
	if (vp_size.x <= 1.0f || vp_size.y <= 1.0f) {
		vp_size = get_size();
	}
	if (vp_size.x <= 1.0f || vp_size.y <= 1.0f) {
		vp_size = Vector2(LOGICAL_WIDTH, LOGICAL_HEIGHT);
	}

	float scale = MIN(vp_size.x / LOGICAL_WIDTH, vp_size.y / LOGICAL_HEIGHT);
	scale = MAX(0.1f, scale);
	Size2 scaled_sz(LOGICAL_WIDTH * scale, LOGICAL_HEIGHT * scale);
	Vector2 pos((vp_size.x - scaled_sz.x) * 0.5f, (vp_size.y - scaled_sz.y) * 0.5f);
	stage_clip->set_size(Size2(LOGICAL_WIDTH, LOGICAL_HEIGHT));
	stage_clip->set_scale(Vector2(scale, scale));
	stage_clip->set_position(pos);
}

void ClockGearsCreditsPlayer::start_credits(const Dictionary &p_cfg_override) {
	set_preview_mode(false);
	_ensure_ui_nodes();
	Dictionary cfg = p_cfg_override;
	if (cfg.is_empty()) {
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			cfg = dm->get_credits_config();
		} else {
			cfg = ClockGearsDataManager::get_default_credits_config();
		}
	}

	_rebuild_from_config(cfg);
	_update_responsive_stage();

	completed_naturally = false;
	cancelled_exit = false;
	enter_held = false;
	mouse_held = false;
	script_accelerated = false;
	playing = true;
	scroll_y = LOGICAL_HEIGHT;
	if (scroll_vbox) {
		scroll_vbox->set_position(Vector2(50.0f, scroll_y));
	}

	set_visible(true);
	set_process(true);
	set_process_input(true);
	if (is_inside_tree()) {
		grab_focus();
	}

	audio_saved_prev_bgm = false;
	audio_played_credits_bgm = false;
	String bgm_path = String(active_config.get("bgm_path", "")).strip_edges();
	if (!bgm_path.is_empty() && !(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
		if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
			if (am->is_bgm_playing() && !am->get_current_bgm_path().is_empty()) {
				am->save_bgm();
				audio_saved_prev_bgm = true;
			}
			am->play_bgm(bgm_path, 1.0f, 1.0f, 0.35f, 0.0f);
			audio_played_credits_bgm = true;
		}
	}

	if (content_height <= 0.0f && !(custom_ui_layer && custom_ui_layer->is_visible())) {
		_finish(false);
	}
}

void ClockGearsCreditsPlayer::_finish(bool p_cancelled) {
	if (!playing) {
		return;
	}
	playing = false;
	enter_held = false;
	mouse_held = false;
	script_accelerated = false;
	cancelled_exit = p_cancelled;
	completed_naturally = !p_cancelled;

	set_process(false);
	set_process_input(false);
	set_visible(false);

	if (audio_played_credits_bgm && !(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
		if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
			if (audio_saved_prev_bgm && am->has_saved_bgm()) {
				am->restore_bgm(0.35f);
			} else {
				am->stop_bgm(0.35f);
			}
		}
	}
	audio_saved_prev_bgm = false;
	audio_played_credits_bgm = false;

	emit_signal("credits_finished", p_cancelled);
}

void ClockGearsCreditsPlayer::stop_credits(bool p_cancelled) {
	_finish(p_cancelled);
}

void ClockGearsCreditsPlayer::step_credits(float p_delta) {
	if (!playing) {
		return;
	}
	if (p_delta <= 0.0f) {
		return;
	}
	if (scroll_vbox) {
		float real_h = scroll_vbox->get_combined_minimum_size().y;
		if (real_h > content_height) {
			content_height = real_h;
		}
		if (scroll_vbox->get_size().y > content_height) {
			content_height = scroll_vbox->get_size().y;
		}
	}

	float mult = is_accelerated() ? accel_multiplier : 1.0f;
	scroll_y -= scroll_speed * mult * p_delta;
	if (scroll_vbox) {
		scroll_vbox->set_position(Vector2(50.0f, scroll_y));
	}

	if (scroll_y + content_height <= 0.0f) {
		_finish(false);
	}
}

void ClockGearsCreditsPlayer::_on_custom_ui_action(const String &p_action, const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array steps;
	if (custom_ui_layer && !p_elem_id.is_empty()) {
		steps = custom_ui_layer->get_element_actions(p_elem_id);
	}
	if (steps.is_empty()) {
		String act = p_action.strip_edges().to_lower();
		if (!act.is_empty()) {
			Dictionary d;
			d["type"] = act;
			steps.push_back(d);
		}
	}
	for (int i = 0; i < steps.size(); i++) {
		if (steps[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary st = steps[i];
		String tp = String(st.get("type", "")).strip_edges().to_lower();
		if (tp == "back" || tp == "close" || tp == "close_menu") {
			_finish(true);
			return;
		} else if (tp == "quit") {
			if (!preview_mode && get_tree()) get_tree()->quit();
			return;
		} else if (tp == "set_switch" && dm) {
			String sw = String(st.get("switch_name", st.get("switch", ""))).strip_edges();
			if (!sw.is_empty()) {
				bool val = (String(st.get("value", true)).to_lower() == "toggle") ? !dm->get_switch(sw) : bool(st.get("value", true));
				dm->set_switch(sw, val);
			}
		} else if (tp == "set_variable" && dm) {
			String vn = String(st.get("variable_name", "")).strip_edges();
			if (vn.is_empty()) {
				vn = String(st.get("var_name", st.get("variable", st.get("name", "")))).strip_edges();
			}
			if (!vn.is_empty()) {
				dm->set_variable(vn, int(st.get("value", 0)));
			}
		} else if (tp == "play_sound") {
			String sp = String(st.get("sound_path", st.get("sound", ""))).strip_edges();
			if (!sp.is_empty()) {
				if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
					am->play_se(sp, 1.0f, 1.0f);
				}
			}
		}
	}
	if (custom_ui_layer && custom_ui_layer->is_visible()) {
		custom_ui_layer->refresh_live_values();
	}
}

bool ClockGearsCreditsPlayer::is_custom_ui_active() const {
	return custom_ui_layer && custom_ui_layer->is_visible();
}

bool ClockGearsCreditsPlayer::handle_credits_key(Key p_keycode, bool p_pressed) {
	if (!playing) {
		return false;
	}
	if (p_keycode == Key::ESCAPE) {
		if (p_pressed) {
			_finish(true);
		}
		return true;
	}
	if (p_keycode == Key::ENTER || p_keycode == Key::KP_ENTER || p_keycode == Key::SPACE) {
		enter_held = p_pressed;
		return true;
	}
	return false;
}

void ClockGearsCreditsPlayer::input(const Ref<InputEvent> &p_event) {
	if (!playing || !is_visible_in_tree() || p_event.is_null()) {
		return;
	}
	InputMap *im = InputMap::get_singleton();

	Ref<InputEventKey> kev = p_event;
	if (kev.is_valid()) {
		if (kev->is_pressed() && !kev->is_echo() && kev->get_keycode() == Key::ESCAPE) {
			if (Viewport *vp = get_viewport()) {
				vp->set_input_as_handled();
			}
			_finish(true);
			return;
		}
		bool custom_btn_focused = false;
		if (custom_ui_layer && custom_ui_layer->is_visible() && get_viewport()) {
			Button *fb = Object::cast_to<Button>(get_viewport()->gui_get_focus_owner());
			if (fb && custom_ui_layer->owns_control(fb) && !fb->is_disabled() && fb->is_visible_in_tree()) {
				custom_btn_focused = true;
			}
		}
		bool is_accel_key = (kev->get_keycode() == Key::ENTER || kev->get_keycode() == Key::KP_ENTER);
		if (!is_accel_key && im && im->has_action("ui_accept")) {
			is_accel_key = kev->is_action("ui_accept");
		}
		if (is_accel_key) {
			if (custom_btn_focused) {
				return;
			}
			enter_held = kev->is_pressed();
			if (Viewport *vp = get_viewport()) {
				vp->set_input_as_handled();
			}
			return;
		}
	}

	if (im && im->has_action("ui_cancel") && p_event->is_action_pressed("ui_cancel")) {
		if (Viewport *vp = get_viewport()) {
			vp->set_input_as_handled();
		}
		_finish(true);
		return;
	}

	Ref<InputEventMouseButton> mev = p_event;
	if (mev.is_valid() && mev->get_button_index() == MouseButton::LEFT) {
		if (custom_ui_layer && custom_ui_layer->is_visible() && get_viewport()) {
			Button *hb = Object::cast_to<Button>(get_viewport()->gui_get_hovered_control());
			if (hb && custom_ui_layer->owns_control(hb) && !hb->is_disabled()) {
				return;
			}
		}
		mouse_held = mev->is_pressed();
		if (Viewport *vp = get_viewport()) {
			vp->set_input_as_handled();
		}
	}
}

void ClockGearsCreditsPlayer::gui_input(const Ref<InputEvent> &p_event) {
	input(p_event);
}

Dictionary ClockGearsCreditsPlayer::get_scroll_state() const {
	Dictionary d;
	d["playing"] = playing;
	d["scroll_y"] = scroll_y;
	d["content_height"] = content_height;
	d["scroll_speed"] = scroll_speed;
	d["effective_speed"] = scroll_speed * (is_accelerated() ? accel_multiplier : 1.0f);
	d["accelerated"] = is_accelerated();
	d["completed_naturally"] = completed_naturally;
	d["cancelled"] = cancelled_exit;
	d["item_count"] = item_count_rendered;
	d["remaining_distance"] = MAX(0.0f, scroll_y + content_height);
	return d;
}

Dictionary ClockGearsCreditsPlayer::get_layout_bounds_info(const Vector2 &p_viewport_size) const {
	const_cast<ClockGearsCreditsPlayer *>(this)->_update_responsive_stage(p_viewport_size);
	Dictionary out;
	Vector2 vp = p_viewport_size;
	if (vp.x <= 1.0f || vp.y <= 1.0f) {
		vp = (is_inside_tree() && get_viewport()) ? get_viewport()->get_visible_rect().size : Vector2(LOGICAL_WIDTH, LOGICAL_HEIGHT);
	}
	Rect2 vp_rect(Vector2(), vp);
	Rect2 stage_rect = stage_clip ? Rect2(stage_clip->get_position(), Vector2(LOGICAL_WIDTH, LOGICAL_HEIGHT) * stage_clip->get_scale()) : Rect2();
	out["viewport_size"] = vp;
	out["stage_rect"] = stage_rect;
	out["stage_scale"] = stage_clip ? stage_clip->get_scale() : Vector2(1, 1);
	out["fits_within_viewport"] = vp_rect.grow(1.5f).encloses(stage_rect);
	out["scroll_y"] = scroll_y;
	out["content_height"] = content_height;
	return out;
}

void ClockGearsCreditsPlayer::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY || p_what == NOTIFICATION_RESIZED) {
		_ensure_ui_nodes();
		_update_responsive_stage();
	} else if (p_what == NOTIFICATION_PROCESS && playing) {
		step_credits(float(get_process_delta_time()));
	}
}

void ClockGearsCreditsPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_credits", "cfg_override"), &ClockGearsCreditsPlayer::start_credits, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("stop_credits", "cancelled"), &ClockGearsCreditsPlayer::stop_credits, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("step_credits", "delta"), &ClockGearsCreditsPlayer::step_credits);
	ClassDB::bind_method(D_METHOD("set_preview_mode", "preview"), &ClockGearsCreditsPlayer::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsCreditsPlayer::is_preview_mode);
	ClassDB::bind_method(D_METHOD("apply_credits_config_preview", "cfg", "scroll_y"), &ClockGearsCreditsPlayer::apply_credits_config_preview, DEFVAL(60.0f));
	ClassDB::bind_method(D_METHOD("get_preview_snapshot"), &ClockGearsCreditsPlayer::get_preview_snapshot);
	ClassDB::bind_method(D_METHOD("is_playing"), &ClockGearsCreditsPlayer::is_playing);
	ClassDB::bind_method(D_METHOD("was_cancelled"), &ClockGearsCreditsPlayer::was_cancelled);
	ClassDB::bind_method(D_METHOD("was_completed_naturally"), &ClockGearsCreditsPlayer::was_completed_naturally);
	ClassDB::bind_method(D_METHOD("set_accelerated", "accel"), &ClockGearsCreditsPlayer::set_accelerated);
	ClassDB::bind_method(D_METHOD("is_accelerated"), &ClockGearsCreditsPlayer::is_accelerated);
	ClassDB::bind_method(D_METHOD("handle_credits_key", "keycode", "pressed"), &ClockGearsCreditsPlayer::handle_credits_key, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_scroll_state"), &ClockGearsCreditsPlayer::get_scroll_state);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsCreditsPlayer::get_layout_bounds_info, DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("is_custom_ui_active"), &ClockGearsCreditsPlayer::is_custom_ui_active);
	ClassDB::bind_method(D_METHOD("get_custom_ui_layer"), &ClockGearsCreditsPlayer::get_custom_ui_layer);

	ADD_SIGNAL(MethodInfo("credits_finished", PropertyInfo(Variant::BOOL, "cancelled")));
}

ClockGearsCreditsPlayer::ClockGearsCreditsPlayer() {
	set_visible(false);
}
