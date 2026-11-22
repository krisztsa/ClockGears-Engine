#include "clockgears_game_over.h"

#include "../core/clockgears_data_manager.h"
#include "clockgears_audio_manager.h"
#include "clockgears_title_screen.h"
#include "clockgears_ui_design_layer.h"
#include "clockgears_window_style.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "scene/gui/center_container.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"

ClockGearsGameOverScreen::ClockGearsGameOverScreen() {
	set_name("ClockGearsGameOverScreen");
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_process_mode(PROCESS_MODE_ALWAYS);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_focus_mode(FOCUS_ALL);
	set_visible(false);
}

void ClockGearsGameOverScreen::_ensure_ui_nodes() {
	if (bg_color_rect) {
		return;
	}
	bg_color_rect = memnew(ColorRect);
	bg_color_rect->set_name("BgColor");
	bg_color_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	bg_color_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg_color_rect->set_color(Color::html("#0c070cff"));
	add_child(bg_color_rect);

	bg_texture_rect = memnew(TextureRect);
	bg_texture_rect->set_name("BgTexture");
	bg_texture_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	bg_texture_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	bg_texture_rect->set_visible(false);
	add_child(bg_texture_rect);

	CenterContainer *center = memnew(CenterContainer);
	center->set_name("CenterRoot");
	center->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	center->set_mouse_filter(MOUSE_FILTER_PASS);
	add_child(center);

	stage_root = center;

	main_vbox = memnew(VBoxContainer);
	main_vbox->set_name("GameOverVBox");
	main_vbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	main_vbox->add_theme_constant_override("separation", 22);
	center->add_child(main_vbox);

	title_label = memnew(Label);
	title_label->set_name("GameOverTitle");
	title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	title_label->set_text("GAME OVER");
	title_label->add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.75f));
	title_label->add_theme_constant_override("shadow_offset_x", 2);
	title_label->add_theme_constant_override("shadow_offset_y", 2);
	main_vbox->add_child(title_label);

	panel = memnew(PanelContainer);
	panel->set_name("GameOverPanel");
	panel->set_h_size_flags(SIZE_SHRINK_CENTER);
	main_vbox->add_child(panel);

	buttons_vbox = memnew(VBoxContainer);
	buttons_vbox->set_name("ButtonsVBox");
	buttons_vbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	buttons_vbox->add_theme_constant_override("separation", 10);
	panel->add_child(buttons_vbox);

	retry_btn = memnew(Button);
	retry_btn->set_name("RetryButton");
	retry_btn->set_text("Reintentar");
	retry_btn->connect("pressed", callable_mp(this, &ClockGearsGameOverScreen::_on_retry_pressed));
	buttons_vbox->add_child(retry_btn);

	return_title_btn = memnew(Button);
	return_title_btn->set_name("ReturnTitleButton");
	return_title_btn->set_text(String::utf8("Volver al menú"));
	return_title_btn->connect("pressed", callable_mp(this, &ClockGearsGameOverScreen::_on_return_title_pressed));
	buttons_vbox->add_child(return_title_btn);

	custom_ui_layer = memnew(ClockGearsUiDesignLayer);
	custom_ui_layer->set_name("CustomGameOverUiLayer");
	custom_ui_layer->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	custom_ui_layer->set_window_type_fallback("game_menu");
	custom_ui_layer->set_visible(false);
	custom_ui_layer->connect("action_triggered", callable_mp(this, &ClockGearsGameOverScreen::_on_custom_ui_action));
	add_child(custom_ui_layer);
}

void ClockGearsGameOverScreen::_rebuild_from_config(const Dictionary &p_cfg) {
	_ensure_ui_nodes();
	active_config = ClockGearsDataManager::normalize_game_over_config(p_cfg);

	Dictionary bg = active_config.get("background", Dictionary());
	ClockGearsTitleScreen::apply_screen_background(bg_color_rect, bg_texture_rect, bg, "#0c070cff");

	if (title_label) {
		title_label->set_text(String(active_config.get("title_text", "GAME OVER")));
		Color tc = Color::html(String(active_config.get("title_color", "#e84a5fff")));
		title_label->add_theme_color_override("font_color", tc);
	}

	bool show_retry = bool(active_config.get("retry_enabled", true));
	bool show_title = bool(active_config.get("title_enabled", true));
	if (retry_btn) {
		retry_btn->set_visible(show_retry);
		retry_btn->set_text(String(active_config.get("retry_label", "Reintentar")));
	}
	if (return_title_btn) {
		return_title_btn->set_visible(show_title);
		return_title_btn->set_text(String(active_config.get("title_label", String::utf8("Volver al menú"))));
	}
	if (panel) {
		panel->set_visible(show_retry || show_title);
	}

	String wsid = String(active_config.get("window_style_id", "default")).strip_edges().to_lower();
	Dictionary style_overrides;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!wsid.is_empty() && wsid != "default" && dm) {
		Dictionary specific = dm->get_window_style(wsid);
		if (!specific.is_empty()) {
			style_overrides = specific;
		}
	}
	if (panel) {
		ClockGearsWindowStyleManager::apply_window_style(panel, "default", style_overrides);
	}
	if (retry_btn) {
		ClockGearsWindowStyleManager::apply_button_style(retry_btn, "default", style_overrides);
	}
	if (return_title_btn) {
		ClockGearsWindowStyleManager::apply_button_style(return_title_btn, "default", style_overrides);
	}

	if (custom_ui_layer) {
		if (dm && dm->has_custom_ui_design("game_over")) {
			if (stage_root) stage_root->set_visible(false);
			custom_ui_layer->set_window_type_fallback("game_menu");
			custom_ui_layer->set_preview_mode(preview_mode);
			custom_ui_layer->apply_screen_design("game_over");
			custom_ui_layer->set_visible(true);
			if (!preview_mode) {
				custom_ui_layer->play_transition_in();
			}
		} else {
			if (stage_root) stage_root->set_visible(true);
			custom_ui_layer->set_visible(false);
			custom_ui_layer->clear_elements();
		}
	}

	_update_responsive_stage();
}

void ClockGearsGameOverScreen::_update_responsive_stage(const Vector2 &p_override_vp) {
	if (!main_vbox || !title_label) {
		return;
	}
	Vector2 vp_sz = p_override_vp;
	if (vp_sz.x < 64.0f || vp_sz.y < 64.0f) {
		if (is_inside_tree() && get_viewport()) {
			vp_sz = get_viewport()->get_visible_rect().size;
		}
		if (vp_sz.x < 64.0f || vp_sz.y < 64.0f) {
			vp_sz = get_size();
		}
		if (vp_sz.x < 64.0f || vp_sz.y < 64.0f) {
			vp_sz = Vector2(LOGICAL_WIDTH, LOGICAL_HEIGHT);
		}
	}
	float s = MAX(0.35f, MIN(vp_sz.x / LOGICAL_WIDTH, vp_sz.y / LOGICAL_HEIGHT));
	int base_fsz = CLAMP(int(active_config.get("title_font_size", 48)), 16, 128);
	title_label->add_theme_font_size_override("font_size", MAX(16, int(Math::round(float(base_fsz) * s))));
	main_vbox->add_theme_constant_override("separation", MAX(10, int(Math::round(22.0f * s))));
	if (buttons_vbox) {
		buttons_vbox->add_theme_constant_override("separation", MAX(6, int(Math::round(10.0f * s))));
	}
	Size2 btn_sz(Math::round(240.0f * s), Math::round(38.0f * s));
	int btn_fsz = MAX(12, int(Math::round(16.0f * s)));
	if (retry_btn) {
		retry_btn->set_custom_minimum_size(btn_sz);
		retry_btn->add_theme_font_size_override("font_size", btn_fsz);
	}
	if (return_title_btn) {
		return_title_btn->set_custom_minimum_size(btn_sz);
		return_title_btn->add_theme_font_size_override("font_size", btn_fsz);
	}
}

void ClockGearsGameOverScreen::show_game_over(const Dictionary &p_cfg_override) {
	Dictionary cfg = p_cfg_override;
	if (cfg.is_empty()) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		cfg = dm ? dm->get_game_over_config() : ClockGearsDataManager::get_default_game_over_config();
	}
	_rebuild_from_config(cfg);
	open_flag = true;
	last_action = "";
	set_visible(true);

	if (!preview_mode && !(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
		String bgm = String(active_config.get("bgm_path", "")).strip_edges();
		if (!bgm.is_empty()) {
			if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
				am->play_bgm(bgm, 1.0f, 1.0f, 0.35f, float(double(active_config.get("bgm_volume_db", 0.0))));
			}
		}
		if (custom_ui_layer && custom_ui_layer->is_visible()) {
			Vector<Button *> cbtns = custom_ui_layer->get_focus_ordered_buttons();
			if (!cbtns.is_empty() && cbtns[0] && is_inside_tree()) {
				cbtns[0]->grab_focus();
				custom_ui_layer->update_cursor_for_button(cbtns[0]);
			}
		} else if (retry_btn && retry_btn->is_visible() && is_inside_tree()) {
			retry_btn->grab_focus();
		} else if (return_title_btn && return_title_btn->is_visible() && is_inside_tree()) {
			return_title_btn->grab_focus();
		}
	}
	emit_signal("game_over_shown");
}

void ClockGearsGameOverScreen::hide_game_over() {
	if (!open_flag) {
		return;
	}
	open_flag = false;
	set_visible(false);
	emit_signal("game_over_closed");
}

void ClockGearsGameOverScreen::set_preview_mode(bool p_preview) {
	preview_mode = p_preview;
	set_mouse_filter(p_preview ? MOUSE_FILTER_IGNORE : MOUSE_FILTER_STOP);
	if (p_preview) {
		set_visible(true);
	}
}

void ClockGearsGameOverScreen::apply_preview_config(const Dictionary &p_cfg) {
	_rebuild_from_config(p_cfg);
	set_visible(true);
}

void ClockGearsGameOverScreen::_on_retry_pressed() {
	if (preview_mode) {
		return;
	}
	trigger_retry();
}

void ClockGearsGameOverScreen::_on_return_title_pressed() {
	if (preview_mode) {
		return;
	}
	trigger_return_to_title();
}

Control *ClockGearsGameOverScreen::get_custom_ui_layer() const {
	return custom_ui_layer;
}

void ClockGearsGameOverScreen::_on_custom_ui_action(const String &p_action, const String &p_elem_id) {
	if (preview_mode) {
		return;
	}
	Array steps;
	if (custom_ui_layer && !p_elem_id.is_empty()) {
		steps = custom_ui_layer->get_element_actions(p_elem_id);
	}
	if (steps.is_empty() && !p_action.strip_edges().is_empty()) {
		Dictionary single;
		single["type"] = p_action.strip_edges().to_lower();
		steps.push_back(single);
	}
	for (int i = 0; i < steps.size(); i++) {
		if (steps[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary st = steps[i];
		String tp = String(st.get("type", st.get("action", ""))).strip_edges().to_lower();
		if (tp == "retry" || tp == "load_latest" || tp == "reintentar") {
			trigger_retry();
			return;
		}
		if (tp == "to_title" || tp == "return_to_title" || tp == "title") {
			trigger_return_to_title();
			return;
		}
		if (tp == "close" || tp == "back") {
			hide_game_over();
			return;
		}
		if (tp == "common_event") {
			int ce_id = int(st.get("common_event_id", st.get("id", 0)));
			last_action = "common_event:" + itos(ce_id);
			if (is_inside_tree() && get_tree()) {
				if (Node *root = get_tree()->get_root()) {
					Node *interp = root->find_child("ClockGearsEventInterpreter", true, false);
					if (interp && ce_id > 0) {
						interp->call("execute_common_event", ce_id);
					}
				}
			}
			continue;
		}
		if (ClockGearsUiDesignLayer::execute_common_action_step(st, custom_ui_layer)) {
			last_action = tp;
		}
	}
}

bool ClockGearsGameOverScreen::trigger_retry() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	hide_game_over();
	int latest_slot = dm->get_latest_save_slot();
	if (latest_slot > 0) {
		last_action = "load_latest_save";
		emit_signal("retry_requested", last_action, latest_slot);
		if (is_inside_tree() && get_tree()) {
			return dm->start_game_from_save(latest_slot, this);
		}
		return dm->load_game_slot(latest_slot);
	}

	// Sin partidas guardadas: reiniciar desde el mapa inicial con el HP completo.
	last_action = "restart_initial_map";
	dm->reset_runtime_state();
	Dictionary sys = dm->get_system_config();
	sys["runtime_has_transfer"] = false;
	sys.erase("runtime_has_exact_pos");
	dm->set_system_config(sys);

	Array party = dm->get_party_actor_ids();
	for (int i = 0; i < party.size(); i++) {
		int aid = int(party[i]);
		if (aid > 0) {
			dm->set_actor_runtime_hp(aid, dm->get_actor_runtime_max_hp(aid));
		}
	}

	emit_signal("retry_requested", last_action, 0);
	String start_map = String(sys.get("start_map", "")).strip_edges();
	if (is_inside_tree() && get_tree() && !start_map.is_empty() && ResourceLoader::exists(start_map)) {
		return get_tree()->change_scene_to_file(start_map) == OK;
	}
	return true;
}

bool ClockGearsGameOverScreen::trigger_return_to_title() {
	last_action = "return_to_title";
	hide_game_over();
	emit_signal("return_to_title_requested");
	if (!preview_mode && is_inside_tree() && get_tree()) {
		ClockGearsTitleScreen::ensure_title_screen_scene("res://title_screen.tscn");
		if (ResourceLoader::exists("res://title_screen.tscn")) {
			return get_tree()->change_scene_to_file("res://title_screen.tscn") == OK;
		}
	}
	return true;
}

Dictionary ClockGearsGameOverScreen::get_state_snapshot() const {
	Dictionary out;
	out["open"] = open_flag;
	out["last_action"] = last_action;
	out["title_text"] = title_label ? title_label->get_text() : String();
	out["retry_visible"] = retry_btn ? retry_btn->is_visible() : false;
	out["retry_label"] = retry_btn ? retry_btn->get_text() : String();
	out["title_visible"] = return_title_btn ? return_title_btn->is_visible() : false;
	out["title_label"] = return_title_btn ? return_title_btn->get_text() : String();
	out["bgm_path"] = String(active_config.get("bgm_path", ""));
	out["window_style_id"] = String(active_config.get("window_style_id", "default"));
	return out;
}

void ClockGearsGameOverScreen::_notification(int p_what) {
	if (p_what == NOTIFICATION_RESIZED) {
		_update_responsive_stage();
	}
}

void ClockGearsGameOverScreen::gui_input(const Ref<InputEvent> &p_event) {
	if (preview_mode || !open_flag) {
		return;
	}
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		Key kc = k->get_keycode();
		if (custom_ui_layer && custom_ui_layer->is_visible()) {
			Vector<Button *> btns = custom_ui_layer->get_focus_ordered_buttons();
			Button *fbtn = get_viewport() ? Object::cast_to<Button>(get_viewport()->gui_get_focus_owner()) : nullptr;
			if (!btns.is_empty()) {
				int dir = -1;
				if (kc == Key::UP || kc == Key::W) dir = 0;
				else if (kc == Key::RIGHT || kc == Key::D) dir = 1;
				else if (kc == Key::DOWN || kc == Key::S) dir = 2;
				else if (kc == Key::LEFT || kc == Key::A) dir = 3;
				if (dir >= 0) {
					Button *cur = (fbtn && btns.has(fbtn)) ? fbtn : btns[0];
					Button *nxt = custom_ui_layer->find_next_focus_button(cur, dir);
					if (!nxt) {
						int idx = btns.find(cur);
						int delta = (dir == 1 || dir == 2) ? 1 : -1;
						nxt = btns[(idx + delta + btns.size()) % btns.size()];
					}
					if (nxt) {
						nxt->grab_focus();
						custom_ui_layer->update_cursor_for_button(nxt);
					}
					accept_event();
					return;
				}
				if (kc == Key::ENTER || kc == Key::KP_ENTER || kc == Key::SPACE) {
					Button *act_b = (fbtn && btns.has(fbtn)) ? fbtn : btns[0];
					if (act_b && !act_b->is_disabled()) {
						act_b->emit_signal("pressed");
						accept_event();
						return;
					}
				}
			}
		}
		if (kc == Key::ENTER || kc == Key::KP_ENTER || kc == Key::SPACE) {
			Control *f = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
			if (f == return_title_btn && return_title_btn->is_visible()) {
				trigger_return_to_title();
			} else if (retry_btn && retry_btn->is_visible()) {
				trigger_retry();
			} else if (return_title_btn && return_title_btn->is_visible()) {
				trigger_return_to_title();
			}
			accept_event();
		}
	}
}

void ClockGearsGameOverScreen::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_game_over", "cfg_override"), &ClockGearsGameOverScreen::show_game_over, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("hide_game_over"), &ClockGearsGameOverScreen::hide_game_over);
	ClassDB::bind_method(D_METHOD("set_preview_mode", "preview"), &ClockGearsGameOverScreen::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsGameOverScreen::is_preview_mode);
	ClassDB::bind_method(D_METHOD("apply_preview_config", "cfg"), &ClockGearsGameOverScreen::apply_preview_config);
	ClassDB::bind_method(D_METHOD("is_open"), &ClockGearsGameOverScreen::is_open);
	ClassDB::bind_method(D_METHOD("trigger_retry"), &ClockGearsGameOverScreen::trigger_retry);
	ClassDB::bind_method(D_METHOD("trigger_return_to_title"), &ClockGearsGameOverScreen::trigger_return_to_title);
	ClassDB::bind_method(D_METHOD("get_state_snapshot"), &ClockGearsGameOverScreen::get_state_snapshot);
	ClassDB::bind_method(D_METHOD("get_last_action"), &ClockGearsGameOverScreen::get_last_action);
	ClassDB::bind_method(D_METHOD("get_custom_ui_layer"), &ClockGearsGameOverScreen::get_custom_ui_layer);

	ADD_SIGNAL(MethodInfo("game_over_shown"));
	ADD_SIGNAL(MethodInfo("game_over_closed"));
	ADD_SIGNAL(MethodInfo("retry_requested", PropertyInfo(Variant::STRING, "mode"), PropertyInfo(Variant::INT, "slot")));
	ADD_SIGNAL(MethodInfo("return_to_title_requested"));
}
