#include "clockgears_battle_window.h"
#include "editor/clockgears_theme.h"
#include "editor/themes/editor_scale.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "scene/resources/style_box_flat.h"

static const ClockGearsSpriteAnimation *s_find_animation_ci(const ClockGearsSpriteSheetData &p_data, const String &p_name, int p_tex_h = 0) {
	const ClockGearsSpriteAnimation *best_match = nullptr;
	String target_lower = p_name.to_lower();
	for (const KeyValue<String, ClockGearsSpriteAnimation> &E : p_data.animations) {
		if (E.key.to_lower() == target_lower) {
			if (!best_match) {
				best_match = &E.value;
			} else {
				// Preferir la animación que coincide con el alto real de la textura o que tiene más frames
				if (p_tex_h > 0 && E.value.frame_h == p_tex_h && best_match->frame_h != p_tex_h) {
					best_match = &E.value;
				} else if (E.value.frames.size() > best_match->frames.size()) {
					best_match = &E.value;
				} else if (E.key == p_name) {
					best_match = &E.value;
				}
			}
		}
	}
	return best_match;
}

Ref<Texture2D> ClockGearsBattleScene::load_texture_with_fallbacks(const String &p_path) {
	if (p_path.strip_edges().is_empty()) return Ref<Texture2D>();

	if (ResourceLoader::exists(p_path)) {
		Ref<Texture2D> t = ResourceLoader::load(p_path);
		if (t.is_valid()) return t;
	}

	Vector<String> try_paths;
	try_paths.push_back(p_path);

	// Localizar ruta absoluta si pertenece al proyecto
	if (ProjectSettings::get_singleton()) {
		String loc_path = ProjectSettings::get_singleton()->localize_path(p_path);
		if (!loc_path.is_empty() && loc_path != p_path) {
			try_paths.push_back(loc_path);
		}
		String gpath = ProjectSettings::get_singleton()->globalize_path(p_path);
		if (!gpath.is_empty() && gpath != p_path) {
			try_paths.push_back(gpath);
		}
	}

	String fname = p_path.get_file();
	if (!fname.is_empty()) {
		try_paths.push_back("res://img/battlers/" + fname);
		try_paths.push_back("res://img/enemies/" + fname);
		try_paths.push_back("res://img/characters/" + fname);
		try_paths.push_back("res://img/" + fname);
		try_paths.push_back("res://" + fname);

		if (ProjectSettings::get_singleton()) {
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/battlers/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/enemies/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/characters/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/" + fname));
		}
	}

	for (int i = 0; i < try_paths.size(); i++) {
		const String &tp = try_paths[i];
		if (ResourceLoader::exists(tp)) {
			Ref<Texture2D> t = ResourceLoader::load(tp);
			if (t.is_valid()) return t;
		}
		if (FileAccess::exists(tp)) {
			Ref<Image> img = Image::load_from_file(tp);
			if (img.is_valid() && !img->is_empty()) {
				Ref<ImageTexture> it = ImageTexture::create_from_image(img);
				if (it.is_valid()) return it;
			}
		}
	}
	return Ref<Texture2D>();
}

void ClockGearsBattleScene::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_battle", "troop_id", "can_escape", "can_lose", "bgm", "msg_window"), &ClockGearsBattleScene::setup_battle, DEFVAL(""), DEFVAL((ClockGearsMessageWindow *)nullptr));
	ClassDB::bind_method(D_METHOD("get_controller"), &ClockGearsBattleScene::get_controller);
	ClassDB::bind_method(D_METHOD("select_action_attack", "target_idx"), &ClockGearsBattleScene::select_action_attack);
	ClassDB::bind_method(D_METHOD("select_action_skill", "skill_id", "target_idx"), &ClockGearsBattleScene::select_action_skill);
	ClassDB::bind_method(D_METHOD("select_action_item", "item_name", "target_idx"), &ClockGearsBattleScene::select_action_item);
	ClassDB::bind_method(D_METHOD("select_action_defend"), &ClockGearsBattleScene::select_action_defend);
	ClassDB::bind_method(D_METHOD("select_action_flee"), &ClockGearsBattleScene::select_action_flee);
	ClassDB::bind_method(D_METHOD("finish_battle_immediate"), &ClockGearsBattleScene::finish_battle_immediate);
	ClassDB::bind_method(D_METHOD("get_battle_result"), &ClockGearsBattleScene::get_battle_result);
	ClassDB::bind_method(D_METHOD("is_battle_finished"), &ClockGearsBattleScene::is_battle_finished);

	ADD_SIGNAL(MethodInfo("battle_finished", PropertyInfo(Variant::INT, "result")));
}

void ClockGearsBattleScene::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		set_process_unhandled_input(true);
		set_process(true);
		_setup_ui();
		_refresh_ui();
	} else if (p_what == NOTIFICATION_PROCESS) {
		_update_battler_visuals(get_process_delta_time());
	}
}

void ClockGearsBattleScene::_setup_ui() {
	if (main_container) return;

	set_layer(50); // Superior al mapa y tilesets

	// Fondo oscuro semi-transparente
	bg_rect = memnew(ColorRect);
	bg_rect->set_anchors_preset(Control::PRESET_FULL_RECT);
	bg_rect->set_color(Color(0.06f, 0.08f, 0.12f, 0.92f));
	add_child(bg_rect);

	main_container = memnew(PanelContainer);
	main_container->set_anchors_preset(Control::PRESET_FULL_RECT);
	Ref<StyleBoxFlat> empty_sb;
	empty_sb.instantiate();
	empty_sb->set_bg_color(Color(0, 0, 0, 0));
	main_container->add_theme_style_override("panel", empty_sb);
	add_child(main_container);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	main_vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
	main_vbox->add_theme_constant_override("separation", 10);
	main_container->add_child(main_vbox);

	// 1. Barra de Iniciativa (Arriba)
	initiative_panel = memnew(PanelContainer);
	Ref<StyleBoxFlat> init_sb;
	init_sb.instantiate();
	init_sb->set_bg_color(Color(0.12f, 0.14f, 0.18f, 0.85f));
	init_sb->set_border_width_all(1);
	init_sb->set_border_color(Color(0.3f, 0.35f, 0.45f, 0.9f));
	init_sb->set_corner_radius_all(4);
	initiative_panel->add_theme_style_override("panel", init_sb);
	main_vbox->add_child(initiative_panel);

	initiative_bar = memnew(HBoxContainer);
	initiative_bar->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	initiative_bar->set_custom_minimum_size(Size2(0, 36));
	initiative_bar->add_theme_constant_override("separation", 8);
	initiative_panel->add_child(initiative_bar);

	// 2. Área de Enemigos (Centro)
	enemies_container = memnew(HBoxContainer);
	enemies_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	enemies_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemies_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	enemies_container->add_theme_constant_override("separation", 24);
	main_vbox->add_child(enemies_container);

	// Log de Combate (Abajo, compacto y no bloqueante del turno)
	battle_log_panel = memnew(PanelContainer);
	Ref<StyleBoxFlat> log_sb;
	log_sb.instantiate();
	log_sb->set_bg_color(Color(0.08f, 0.1f, 0.14f, 0.9f));
	log_sb->set_border_width_all(1);
	log_sb->set_border_color(Color(0.35f, 0.42f, 0.55f, 0.75f));
	log_sb->set_corner_radius_all(4);
	battle_log_panel->add_theme_style_override("panel", log_sb);
	battle_log_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	battle_log_panel->set_custom_minimum_size(Size2(0, 30));

	battle_log_label = memnew(Label);
	battle_log_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	battle_log_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	battle_log_label->add_theme_color_override("font_color", Color(1.0f, 0.92f, 0.65f));
	battle_log_label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	battle_log_panel->add_child(battle_log_label);
	main_vbox->add_child(battle_log_panel);
	battle_log_panel->hide();

	// 3. Fila inferior: Grupo del Jugador (Izq) + Comandos de Acción (Der)
	HBoxContainer *bottom_row = memnew(HBoxContainer);
	bottom_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bottom_row->set_custom_minimum_size(Size2(0, 140));
	bottom_row->add_theme_constant_override("separation", 16);
	main_vbox->add_child(bottom_row);

	// Área del Grupo
	party_container = memnew(HBoxContainer);
	party_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	party_container->add_theme_constant_override("separation", 12);
	bottom_row->add_child(party_container);

	// Panel de Comandos
	actions_panel = memnew(PanelContainer);
	Ref<StyleBoxFlat> act_sb;
	act_sb.instantiate();
	act_sb->set_bg_color(Color(0.1f, 0.12f, 0.16f, 0.95f));
	act_sb->set_border_width_all(2);
	act_sb->set_border_color(Color(0.35f, 0.5f, 0.75f, 0.9f));
	act_sb->set_corner_radius_all(6);
	actions_panel->add_theme_style_override("panel", act_sb);
	actions_panel->set_custom_minimum_size(Size2(180, 130));
	bottom_row->add_child(actions_panel);

	actions_vbox = memnew(VBoxContainer);
	actions_vbox->add_theme_constant_override("separation", 4);
	actions_panel->add_child(actions_vbox);

	btn_attack = memnew(Button(String::utf8("Atacar")));
	btn_attack->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_attack->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_on_btn_attack_pressed));
	actions_vbox->add_child(btn_attack);

	btn_skill = memnew(Button(String::utf8("Habilidad")));
	btn_skill->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_skill->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_on_btn_skill_pressed));
	actions_vbox->add_child(btn_skill);

	btn_item = memnew(Button(String::utf8("Objeto")));
	btn_item->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_item->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_on_btn_item_pressed));
	actions_vbox->add_child(btn_item);

	btn_defend = memnew(Button(String::utf8("Defender")));
	btn_defend->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_defend->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_on_btn_defend_pressed));
	actions_vbox->add_child(btn_defend);

	btn_flee = memnew(Button(String::utf8("Huir")));
	btn_flee->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn_flee->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_on_btn_flee_pressed));
	actions_vbox->add_child(btn_flee);

	// 4. Submenú flotante para selección de objetivo / habilidad / objeto
	sub_menu_panel = memnew(PanelContainer);
	sub_menu_panel->add_theme_style_override("panel", act_sb);
	sub_menu_panel->set_custom_minimum_size(Size2(220, 160));
	sub_menu_panel->hide();
	add_child(sub_menu_panel);

	sub_menu_vbox = memnew(VBoxContainer);
	sub_menu_vbox->add_theme_constant_override("separation", 4);
	sub_menu_panel->add_child(sub_menu_vbox);
}

void ClockGearsBattleScene::setup_battle(int p_troop_id, bool p_can_escape, bool p_can_lose, const String &p_bgm, ClockGearsMessageWindow *p_msg_window) {
	if (!controller.is_valid()) {
		controller.instantiate();
	}
	controller->setup_battle(p_troop_id, p_can_escape, p_can_lose, p_bgm);

	// Reutilizar la ventana de mensajes de la Fase 7A
	if (p_msg_window) {
		message_window = p_msg_window;
		owns_message_window = false;
	} else if (ClockGearsMessageWindow::get_singleton()) {
		message_window = ClockGearsMessageWindow::get_singleton();
		owns_message_window = false;
	} else {
		message_window = memnew(ClockGearsMessageWindow);
		message_window->set_anchors_preset(Control::PRESET_FULL_RECT);
		add_child(message_window);
		owns_message_window = true;
	}

	if (message_window && !message_window->is_connected("message_completed", callable_mp(this, &ClockGearsBattleScene::_on_message_completed))) {
		message_window->connect("message_completed", callable_mp(this, &ClockGearsBattleScene::_on_message_completed));
	}
	if (controller.is_valid() && !controller->is_connected("hit_executed", callable_mp(this, &ClockGearsBattleScene::_on_hit_executed))) {
		controller->connect("hit_executed", callable_mp(this, &ClockGearsBattleScene::_on_hit_executed));
	}

	_setup_ui();
	_refresh_ui();

	_on_hit_executed(String::utf8("¡Comienza la batalla!"));
}

void ClockGearsBattleScene::_update_battler_visuals(float p_delta) {
	for (int i = 0; i < battler_visuals.size(); i++) {
		ClockGearsBattlerVisual &vis = battler_visuals.write[i];
		if (vis.base_texture.is_null() || vis.atlas_texture.is_null()) continue;

		if (vis.unit.is_valid() && vis.unit->get_is_dead()) {
			const ClockGearsSpriteAnimation *death_anim = s_find_animation_ci(vis.sprite_data, "death");
			if (death_anim && vis.current_anim != death_anim->name) {
				vis.current_anim = death_anim->name;
				vis.anim_timer = 0.0f;
			}
		} else {
			if (vis.state_duration > 0.0f) {
				vis.state_duration -= p_delta;
				if (vis.state_duration <= 0.0f) {
					const ClockGearsSpriteAnimation *idle_anim = s_find_animation_ci(vis.sprite_data, "idle");
					vis.current_anim = idle_anim ? idle_anim->name : "idle";
					vis.anim_timer = 0.0f;
				}
			}
		}

		vis.anim_timer += p_delta;

		int frame_num = 0;
		Rect2i r;
		const ClockGearsSpriteAnimation *cur_anim = s_find_animation_ci(vis.sprite_data, vis.current_anim);
		if (!cur_anim) {
			cur_anim = s_find_animation_ci(vis.sprite_data, "idle");
		}
		if (cur_anim && cur_anim->get_frame_count() > 0) {
			const ClockGearsSpriteAnimation &an = *cur_anim;
			bool loop = (vis.current_anim.to_lower() == "idle") ? true : an.loop;
			float step_dur = 1.0f / MAX(0.5f, an.fps);
			int total_f = an.get_frame_count();
			int step = int(vis.anim_timer / step_dur);
			if (loop) {
				step = step % total_f;
			} else {
				step = MIN(step, total_f - 1);
			}
			ClockGearsAnimationFrame f = an.get_frame(step, vis.base_texture.is_valid() ? vis.base_texture->get_width() : 0, vis.base_texture.is_valid() ? vis.base_texture->get_height() : 0);
			if (!f.sheet_path.is_empty() && f.sheet_path != vis.current_sheet_path) {
				Ref<Texture2D> new_tex = load_texture_with_fallbacks(f.sheet_path);
				if (new_tex.is_valid()) {
					vis.base_texture = new_tex;
					vis.atlas_texture->set_atlas(vis.base_texture);
					vis.current_sheet_path = f.sheet_path;
				}
			}
			r = f.region;
		} else {
			// Si falta la animación, usa el frame 0 de idle o default sin romperse
			const ClockGearsSpriteAnimation *idle_anim = s_find_animation_ci(vis.sprite_data, "idle");
			if (idle_anim && idle_anim->get_frame_count() > 0) {
				ClockGearsAnimationFrame f0 = idle_anim->get_frame(0, vis.base_texture.is_valid() ? vis.base_texture->get_width() : 0, vis.base_texture.is_valid() ? vis.base_texture->get_height() : 0);
				if (!f0.sheet_path.is_empty() && f0.sheet_path != vis.current_sheet_path) {
					Ref<Texture2D> new_tex = load_texture_with_fallbacks(f0.sheet_path);
					if (new_tex.is_valid()) {
						vis.base_texture = new_tex;
						vis.atlas_texture->set_atlas(vis.base_texture);
						vis.current_sheet_path = f0.sheet_path;
					}
				}
				r = f0.region;
			} else {
				int cols = vis.sprite_data.cols > 0 ? vis.sprite_data.cols : 4;
				int fw = vis.sprite_data.frame_w > 0 ? vis.sprite_data.frame_w : (vis.base_texture.is_valid() ? vis.base_texture->get_width() / cols : 32);
				int fh = vis.sprite_data.frame_h > 0 ? vis.sprite_data.frame_h : (vis.base_texture.is_valid() ? vis.base_texture->get_height() : 32);
				if (vis.base_texture.is_valid() && vis.base_texture->get_width() > vis.base_texture->get_height() && (fw >= vis.base_texture->get_width() || fw <= 0)) {
					fw = vis.base_texture->get_height();
				}
				if (vis.base_texture.is_valid()) {
					if (fw <= 0 || fw > vis.base_texture->get_width()) fw = vis.base_texture->get_width();
					if (fh <= 0 || fh > vis.base_texture->get_height()) fh = vis.base_texture->get_height();
				}
				r = Rect2i(0, 0, fw, fh);
			}
		}

		vis.atlas_texture->set_region(Rect2(r.position.x, r.position.y, r.size.x, r.size.y));
	}
}

void ClockGearsBattleScene::_trigger_visual_anim(const Ref<ClockGearsBattleUnit> &p_unit, const String &p_anim, float p_duration) {
	for (int i = 0; i < battler_visuals.size(); i++) {
		ClockGearsBattlerVisual &vis = battler_visuals.write[i];
		if (vis.unit == p_unit) {
			const ClockGearsSpriteAnimation *found_anim = s_find_animation_ci(vis.sprite_data, p_anim);
			vis.current_anim = found_anim ? found_anim->name : p_anim;
			vis.anim_timer = 0.0f;
			vis.state_duration = p_duration;
			break;
		}
	}
}

void ClockGearsBattleScene::unhandled_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	Ref<InputEventMouseButton> mb = p_event;

	// 0. Confirmación de fin de batalla (teclado o ratón)
	if (waiting_finish_confirmation) {
		bool is_confirm = false;
		if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
			Key kc = k->get_keycode();
			if (kc == Key::ENTER || kc == Key::SPACE || kc == Key::Z || kc == Key::ESCAPE) {
				is_confirm = true;
			}
		}
		if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
			is_confirm = true;
		}
		if (is_confirm) {
			if (message_window && message_window->is_busy()) {
				message_window->advance_or_close();
			} else {
				finish_battle_immediate();
			}
			return;
		}
	}

	if (!k.is_valid() || !k->is_pressed() || k->is_echo()) return;

	Key kc = k->get_keycode();

	// 0. Confirmación de fin de batalla
	if (waiting_finish_confirmation) {
		if (kc == Key::ENTER || kc == Key::SPACE || kc == Key::Z || kc == Key::ESCAPE) {
			if (message_window && message_window->is_busy()) {
				message_window->advance_or_close();
			} else {
				finish_battle_immediate();
			}
			return;
		}
	}

	// 1. Submenú abierto
	if (sub_menu_panel && sub_menu_panel->is_visible()) {
		if (kc == Key::ESCAPE) {
			_close_sub_menu();
			return;
		}
		if (kc >= Key::KEY_1 && kc <= Key::KEY_9) {
			int target_idx = int(kc - Key::KEY_1);
			int count = 0;
			for (int i = 0; i < sub_menu_vbox->get_child_count(); i++) {
				Button *b = Object::cast_to<Button>(sub_menu_vbox->get_child(i));
				if (b && b->is_visible() && !b->is_disabled() && b->get_text() != String::utf8("Cancelar")) {
					if (count == target_idx) {
						b->emit_signal("pressed");
						return;
					}
					count++;
				}
			}
		}
		return;
	}

	// 2. Turno del actor
	Ref<ClockGearsBattleUnit> active = controller.is_valid() ? controller->get_active_unit() : Ref<ClockGearsBattleUnit>();
	bool can_act = active.is_valid() && active->get_is_actor() && !controller->is_finished();
	if (!can_act) return;

	if (kc == Key::KEY_1 || kc == Key::A) {
		if (btn_attack && !btn_attack->is_disabled()) {
			_on_btn_attack_pressed();
		}
	} else if (kc == Key::KEY_2 || kc == Key::S) {
		if (btn_skill && !btn_skill->is_disabled()) {
			_on_btn_skill_pressed();
		}
	} else if (kc == Key::KEY_3 || kc == Key::I) {
		if (btn_item && !btn_item->is_disabled()) {
			_on_btn_item_pressed();
		}
	} else if (kc == Key::KEY_4 || kc == Key::D) {
		if (btn_defend && !btn_defend->is_disabled()) {
			_on_btn_defend_pressed();
		}
	} else if (kc == Key::KEY_5 || kc == Key::F) {
		if (btn_flee && !btn_flee->is_disabled()) {
			_on_btn_flee_pressed();
		}
	}
}

void ClockGearsBattleScene::_refresh_ui() {
	if (!controller.is_valid() || !enemies_container || !party_container) return;

	_update_initiative_bar();
	battler_visuals.clear();

	// 1. Refrescar cartas de enemigos
	while (enemies_container->get_child_count() > 0) {
		Node *c = enemies_container->get_child(0);
		enemies_container->remove_child(c);
		memdelete(c);
	}
	enemy_cards.clear();

	Array ens = controller->get_enemy_units();
	for (int i = 0; i < ens.size(); i++) {
		Ref<ClockGearsBattleUnit> en = ens[i];
		if (!en.is_valid()) continue;

		PanelContainer *card = memnew(PanelContainer);
		card->set_custom_minimum_size(Size2(140, 160));
		Ref<StyleBoxFlat> csb;
		csb.instantiate();
		csb->set_bg_color(en->get_is_dead() ? Color(0.1f, 0.1f, 0.1f, 0.4f) : Color(0.12f, 0.15f, 0.2f, 0.9f));
		csb->set_border_width_all(1);
		csb->set_border_color(Color(0.4f, 0.45f, 0.55f, 0.8f));
		csb->set_corner_radius_all(6);
		card->add_theme_style_override("panel", csb);

		VBoxContainer *cvb = memnew(VBoxContainer);
		cvb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		cvb->add_theme_constant_override("separation", 6);
		card->add_child(cvb);

		// Gráfico de battler con recorte de frame, centrado y escalado pixel art
		TextureRect *trect = memnew(TextureRect);
		trect->set_custom_minimum_size(Size2(110, 110));
		trect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		trect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		trect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		cvb->add_child(trect);

		ClockGearsSpriteSheetData sp_data = en->get_sprite_data();
		String tex_path = en->get_graphic_path();
		const ClockGearsSpriteAnimation *idle_anim = s_find_animation_ci(sp_data, "idle");
		if (idle_anim && !idle_anim->spritesheet_path.is_empty()) {
			tex_path = idle_anim->spritesheet_path;
		}
		Ref<Texture2D> base_tex = load_texture_with_fallbacks(tex_path);
		Ref<AtlasTexture> atlas;
		atlas.instantiate();

		if (base_tex.is_valid()) {
			idle_anim = s_find_animation_ci(sp_data, "idle", base_tex->get_height());
			atlas->set_atlas(base_tex);
			int init_frame = 0;
			Rect2i r;
			if (idle_anim) {
				if (!idle_anim->frames.is_empty()) {
					init_frame = idle_anim->frames[0];
				}
				r = idle_anim->get_frame_rect(init_frame, base_tex->get_width(), base_tex->get_height());
			} else {
				// Frame 0 por defecto si no hay animación idle
				int cols = sp_data.cols > 0 ? sp_data.cols : 4;
				int fw = sp_data.frame_w > 0 ? sp_data.frame_w : (base_tex->get_width() / cols);
				int fh = sp_data.frame_h > 0 ? sp_data.frame_h : base_tex->get_height();
				if (base_tex->get_width() > base_tex->get_height() && (fw >= base_tex->get_width() || fw <= 0)) {
					fw = base_tex->get_height();
				}
				if (fw <= 0 || fw > base_tex->get_width()) fw = base_tex->get_width();
				if (fh <= 0 || fh > base_tex->get_height()) fh = base_tex->get_height();
				r = Rect2i(0, 0, fw, fh);
			}
			// Si la textura mide 64 de alto pero el frame quedó en 32, ajustar a 64
			if (base_tex->get_height() > 32 && r.size.y < base_tex->get_height()) {
				r.size.y = base_tex->get_height();
				r.size.x = MAX(r.size.x, base_tex->get_height());
			}
			atlas->set_region(Rect2(r.position.x, r.position.y, r.size.x, r.size.y));
			trect->set_texture(atlas);
		}

		ClockGearsBattlerVisual vis;
		vis.unit = en;
		vis.texture_rect = trect;
		vis.base_texture = base_tex;
		vis.atlas_texture = atlas;
		vis.sprite_data = sp_data;
		vis.current_anim = en->get_is_dead() ? "death" : (idle_anim ? idle_anim->name : "idle");
		vis.anim_timer = 0.0f;
		vis.state_duration = 0.0f;
		vis.is_actor = false;
		vis.current_sheet_path = tex_path;
		battler_visuals.push_back(vis);

		Label *lbl_name = memnew(Label(en->get_name()));
		lbl_name->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl_name->add_theme_color_override("font_color", en->get_is_dead() ? Color(0.6f, 0.6f, 0.6f) : Color(1.0f, 0.9f, 0.9f));
		cvb->add_child(lbl_name);

		// Barra de HP
		ProgressBar *hp_bar = memnew(ProgressBar);
		hp_bar->set_min(0);
		hp_bar->set_max(en->get_max_hp());
		hp_bar->set_value(en->get_hp());
		hp_bar->set_custom_minimum_size(Size2(100, 14));
		cvb->add_child(hp_bar);

		enemies_container->add_child(card);
		enemy_cards.push_back(card);
	}

	// 2. Refrescar cartas del grupo
	while (party_container->get_child_count() > 0) {
		Node *c = party_container->get_child(0);
		party_container->remove_child(c);
		memdelete(c);
	}
	party_cards.clear();

	Array actors = controller->get_party_units();
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();

	for (int i = 0; i < actors.size(); i++) {
		Ref<ClockGearsBattleUnit> hero = actors[i];
		if (!hero.is_valid()) continue;

		PanelContainer *pcard = memnew(PanelContainer);
		pcard->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		pcard->set_custom_minimum_size(Size2(130, 110));

		bool is_turn = (active.is_valid() && active == hero);
		Ref<StyleBoxFlat> psb;
		psb.instantiate();
		psb->set_bg_color(is_turn ? Color(0.18f, 0.22f, 0.32f, 0.95f) : Color(0.12f, 0.14f, 0.18f, 0.85f));
		psb->set_border_width_all(is_turn ? 2 : 1);
		psb->set_border_color(is_turn ? Color(1.0f, 0.85f, 0.35f, 1.0f) : Color(0.3f, 0.35f, 0.45f, 0.8f));
		psb->set_corner_radius_all(6);
		pcard->add_theme_style_override("panel", psb);

		VBoxContainer *pvb = memnew(VBoxContainer);
		pvb->add_theme_constant_override("separation", 4);
		pcard->add_child(pvb);

		if (!hero->get_graphic_path().is_empty()) {
			TextureRect *ptrect = memnew(TextureRect);
			ptrect->set_custom_minimum_size(Size2(48, 48));
			ptrect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			ptrect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			ptrect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
			pvb->add_child(ptrect);

			Ref<Texture2D> p_tex = load_texture_with_fallbacks(hero->get_graphic_path());
			Ref<AtlasTexture> patlas;
			patlas.instantiate();
			ClockGearsSpriteSheetData h_sp = hero->get_sprite_data();
			if (p_tex.is_valid()) {
				patlas->set_atlas(p_tex);
				Rect2i r = h_sp.get_frame_rect(0, p_tex->get_width(), p_tex->get_height());
				patlas->set_region(Rect2(r.position.x, r.position.y, r.size.x, r.size.y));
				ptrect->set_texture(patlas);
			}

			ClockGearsBattlerVisual h_vis;
			h_vis.unit = hero;
			h_vis.texture_rect = ptrect;
			h_vis.base_texture = p_tex;
			h_vis.atlas_texture = patlas;
			h_vis.sprite_data = h_sp;
			h_vis.current_anim = "idle";
			h_vis.anim_timer = 0.0f;
			h_vis.state_duration = 0.0f;
			h_vis.is_actor = true;
			h_vis.current_sheet_path = hero->get_graphic_path();
			battler_visuals.push_back(h_vis);
		}

		Label *lbl_hname = memnew(Label(vformat("%s (Nv %d)", hero->get_name(), hero->get_level())));
		lbl_hname->add_theme_color_override("font_color", is_turn ? Color(1.0f, 0.85f, 0.35f) : Color(1.0f, 1.0f, 1.0f));
		pvb->add_child(lbl_hname);

		Label *lbl_hp = memnew(Label(vformat("HP: %d / %d", hero->get_hp(), hero->get_max_hp())));
		pvb->add_child(lbl_hp);

		ProgressBar *p_hp = memnew(ProgressBar);
		p_hp->set_min(0);
		p_hp->set_max(hero->get_max_hp());
		p_hp->set_value(hero->get_hp());
		p_hp->set_custom_minimum_size(Size2(0, 10));
		pvb->add_child(p_hp);

		Label *lbl_mp = memnew(Label(vformat("MP: %d / %d", hero->get_mp(), hero->get_max_mp())));
		pvb->add_child(lbl_mp);

		party_container->add_child(pcard);
		party_cards.push_back(pcard);
	}

	// 3. Activar/Desactivar botones según si es turno del jugador
	bool can_act = active.is_valid() && active->get_is_actor() && !controller->is_finished();
	if (btn_attack) btn_attack->set_disabled(!can_act);
	if (btn_skill) {
		bool has_mp = can_act && (active->get_mp() > 0);
		if (can_act && !has_mp) {
			btn_skill->set_disabled(true);
			btn_skill->set_text(String::utf8("Habilidad (Sin MP)"));
			btn_skill->set_tooltip_text(String::utf8("No tienes MP suficiente para usar habilidades."));
		} else {
			btn_skill->set_disabled(!can_act);
			btn_skill->set_text(String::utf8("Habilidad"));
			btn_skill->set_tooltip_text(String::utf8("Usar una habilidad mágica o técnica"));
		}
	}
	if (btn_item) btn_item->set_disabled(!can_act);
	if (btn_defend) btn_defend->set_disabled(!can_act);
	if (btn_flee) btn_flee->set_disabled(!can_act);
}

void ClockGearsBattleScene::_update_initiative_bar() {
	if (!initiative_bar || !controller.is_valid()) return;

	while (initiative_bar->get_child_count() > 0) {
		Node *c = initiative_bar->get_child(0);
		initiative_bar->remove_child(c);
		memdelete(c);
	}

	Label *lbl_hdr = memnew(Label(String::utf8("Turnos: ")));
	lbl_hdr->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.85f));
	initiative_bar->add_child(lbl_hdr);

	Array order = controller->get_turn_order();
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();

	for (int i = 0; i < order.size(); i++) {
		Ref<ClockGearsBattleUnit> u = order[i];
		if (!u.is_valid() || u->get_is_dead()) continue;

		bool is_cur = (active.is_valid() && active == u);
		String prefix = is_cur ? String::utf8("▶ ") : "";
		String role = u->get_is_actor() ? "[H] " : "[E] ";
		Label *chip = memnew(Label(vformat("%s%s%s (AGI %d)", prefix, role, u->get_name(), u->get_action_speed())));
		chip->add_theme_color_override("font_color", is_cur ? Color(1.0f, 0.85f, 0.2f) : (u->get_is_actor() ? Color(0.6f, 0.85f, 1.0f) : Color(1.0f, 0.6f, 0.6f)));
		initiative_bar->add_child(chip);
	}
}

void ClockGearsBattleScene::_display_message(const String &p_text) {
	if (message_window) {
		message_window->show_message(p_text, "", "", ClockGearsMessageWindow::POS_BOTTOM, ClockGearsMessageWindow::BG_WINDOW);
	}
}

void ClockGearsBattleScene::_check_battle_state() {
	if (!controller.is_valid()) return;

	if (controller->is_finished()) {
		waiting_finish_confirmation = true;
		if (btn_attack) btn_attack->set_disabled(true);
		if (btn_skill) btn_skill->set_disabled(true);
		if (btn_item) btn_item->set_disabled(true);
		if (btn_defend) btn_defend->set_disabled(true);
		if (btn_flee) btn_flee->set_disabled(true);
		_close_sub_menu();

		int res = controller->get_result();
		if (res == ClockGearsBattleController::RESULT_VICTORY) {
			String vic_msg = vformat(String::utf8("¡Victoria! Has ganado %d EXP y %d de Oro."), controller->get_total_exp(), controller->get_total_gold());
			Array drs = controller->get_dropped_items();
			if (!drs.is_empty()) {
				vic_msg += String::utf8("\n¡Objetos encontrados: ");
				for (int i = 0; i < drs.size(); i++) {
					vic_msg += (i > 0 ? ", " : "") + String(drs[i]);
				}
				vic_msg += "!";
			}
			Array lvs = controller->get_level_up_messages();
			for (int i = 0; i < lvs.size(); i++) {
				vic_msg += "\n" + String(lvs[i]);
			}
			_display_message(vic_msg);
		} else if (res == ClockGearsBattleController::RESULT_ESCAPED) {
			_display_message(String::utf8("¡El grupo ha escapado del combate!"));
		} else if (res == ClockGearsBattleController::RESULT_DEFEAT) {
			_display_message(String::utf8("El grupo ha sido derrotado en combate..."));
		} else if (res == ClockGearsBattleController::RESULT_GAME_OVER) {
			_display_message(String::utf8("GAME OVER: Todo el grupo pereció."));
		}

		if (!message_window || !is_inside_tree()) {
			finish_battle_immediate();
		}
	}
}

void ClockGearsBattleScene::_on_message_completed() {
	if (waiting_finish_confirmation) {
		finish_battle_immediate();
	}
}

void ClockGearsBattleScene::_on_hit_executed(const String &p_log) {
	if (battle_log_label && battle_log_panel) {
		battle_log_label->set_text(p_log);
		battle_log_panel->show();
	}
}

void ClockGearsBattleScene::finish_battle_immediate() {
	waiting_finish_confirmation = false;
	if (controller.is_valid()) {
		emit_signal("battle_finished", controller->get_result());
	}
}

void ClockGearsBattleScene::_show_target_selector(int p_action_type, int p_sub_id, const String &p_sub_str) {
	if (!sub_menu_panel || !sub_menu_vbox || !controller.is_valid()) return;

	while (sub_menu_vbox->get_child_count() > 0) {
		Node *c = sub_menu_vbox->get_child(0);
		sub_menu_vbox->remove_child(c);
		memdelete(c);
	}

	Label *lbl_title = memnew(Label(String::utf8("Seleccionar Objetivo:")));
	lbl_title->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.35f));
	sub_menu_vbox->add_child(lbl_title);

	Array targets = (p_action_type == 3) ? controller->get_party_units() : controller->get_enemy_units();
	for (int i = 0; i < targets.size(); i++) {
		Ref<ClockGearsBattleUnit> u = targets[i];
		if (!u.is_valid() || u->get_is_dead()) continue;

		Button *btn = memnew(Button(vformat("%s (HP: %d/%d)", u->get_name(), u->get_hp(), u->get_max_hp())));
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		btn->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_on_target_selected).bind(p_action_type, i, p_sub_id, p_sub_str));
		sub_menu_vbox->add_child(btn);
	}

	Button *btn_cancel = memnew(Button(String::utf8("Cancelar")));
	btn_cancel->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_close_sub_menu));
	sub_menu_vbox->add_child(btn_cancel);

	Vector2 act_gpos = actions_panel ? actions_panel->get_global_position() : Vector2();
	if (act_gpos.x < 10.0f || act_gpos.y < 10.0f) {
		Vector2 vp_size = get_viewport_size();
		if (vp_size.x <= 0) vp_size = Vector2(960, 540);
		sub_menu_panel->set_position(Vector2(MAX(20.0f, vp_size.x - 420.0f), MAX(20.0f, vp_size.y - 190.0f)));
	} else {
		sub_menu_panel->set_position(Vector2(MAX(20.0f, act_gpos.x - 230.0f), MAX(20.0f, act_gpos.y - 40.0f)));
	}
	sub_menu_panel->show();
	if (sub_menu_vbox->get_child_count() > 1) {
		Button *first_btn = Object::cast_to<Button>(sub_menu_vbox->get_child(1));
		if (first_btn) {
			first_btn->grab_focus();
		}
	}
}

void ClockGearsBattleScene::_show_skill_selector() {
	if (!sub_menu_panel || !sub_menu_vbox || !controller.is_valid()) return;

	while (sub_menu_vbox->get_child_count() > 0) {
		Node *c = sub_menu_vbox->get_child(0);
		sub_menu_vbox->remove_child(c);
		memdelete(c);
	}

	Label *lbl_title = memnew(Label(String::utf8("Seleccionar Habilidad:")));
	lbl_title->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.35f));
	sub_menu_vbox->add_child(lbl_title);

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array skills = dm ? dm->get_category_array(CAT_SKILLS) : Array();
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();

	if (!skills.is_empty()) {
		for (int i = 0; i < skills.size(); i++) {
			Dictionary sk = skills[i];
			int sk_id = int(sk.get("id", i + 1));
			String sk_name = sk.get("name", vformat("Habilidad_%d", sk_id));
			int mp_cost = int(sk.get("mp_cost", 10));

			Button *btn = memnew(Button(vformat("%s (%d MP)", sk_name, mp_cost)));
			btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
			if (active.is_valid() && active->get_mp() < mp_cost) {
				btn->set_disabled(true);
				btn->set_tooltip_text(String::utf8("Sin MP suficiente"));
			} else {
				btn->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_show_target_selector).bind(2, sk_id, ""));
			}
			sub_menu_vbox->add_child(btn);
		}
	} else {
		// Habilidad por defecto si la base de datos está vacía
		Button *btn = memnew(Button(String::utf8("Golpe Fuerte (10 MP)")));
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		if (active.is_valid() && active->get_mp() < 10) {
			btn->set_disabled(true);
			btn->set_tooltip_text(String::utf8("Sin MP suficiente"));
		} else {
			btn->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_show_target_selector).bind(2, 1, ""));
		}
		sub_menu_vbox->add_child(btn);
	}

	Button *btn_cancel = memnew(Button(String::utf8("Cancelar")));
	btn_cancel->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_close_sub_menu));
	sub_menu_vbox->add_child(btn_cancel);

	Vector2 act_gpos = actions_panel ? actions_panel->get_global_position() : Vector2();
	if (act_gpos.x < 10.0f || act_gpos.y < 10.0f) {
		Vector2 vp_size = get_viewport_size();
		if (vp_size.x <= 0) vp_size = Vector2(960, 540);
		sub_menu_panel->set_position(Vector2(MAX(20.0f, vp_size.x - 420.0f), MAX(20.0f, vp_size.y - 190.0f)));
	} else {
		sub_menu_panel->set_position(Vector2(MAX(20.0f, act_gpos.x - 230.0f), MAX(20.0f, act_gpos.y - 40.0f)));
	}
	sub_menu_panel->show();
	if (sub_menu_vbox->get_child_count() > 1) {
		Button *first_btn = Object::cast_to<Button>(sub_menu_vbox->get_child(1));
		if (first_btn) {
			first_btn->grab_focus();
		}
	}
}

void ClockGearsBattleScene::_show_item_selector() {
	if (!sub_menu_panel || !sub_menu_vbox || !controller.is_valid()) return;

	while (sub_menu_vbox->get_child_count() > 0) {
		Node *c = sub_menu_vbox->get_child(0);
		sub_menu_vbox->remove_child(c);
		memdelete(c);
	}

	Label *lbl_title = memnew(Label(String::utf8("Seleccionar Objeto:")));
	lbl_title->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.35f));
	sub_menu_vbox->add_child(lbl_title);

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array items = dm ? dm->get_category_array(CAT_ITEMS) : Array();
	bool found = false;

	if (!items.is_empty() && dm) {
		for (int i = 0; i < items.size(); i++) {
			Dictionary it = items[i];
			String iname = it.get("name", "");
			if (dm->has_item(iname, 1)) {
				found = true;
				Button *btn = memnew(Button(vformat("%s", iname)));
				btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
				btn->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_show_target_selector).bind(3, 0, iname));
				sub_menu_vbox->add_child(btn);
			}
		}
	}

	if (!found) {
		Label *lbl_empty = memnew(Label(String::utf8("(No hay consumibles en inventario)")));
		lbl_empty->add_theme_color_override("font_color", Color(0.7f, 0.7f, 0.7f));
		sub_menu_vbox->add_child(lbl_empty);
	}

	Button *btn_cancel = memnew(Button(String::utf8("Cancelar")));
	btn_cancel->connect("pressed", callable_mp(this, &ClockGearsBattleScene::_close_sub_menu));
	sub_menu_vbox->add_child(btn_cancel);

	Vector2 act_gpos = actions_panel ? actions_panel->get_global_position() : Vector2();
	if (act_gpos.x < 10.0f || act_gpos.y < 10.0f) {
		Vector2 vp_size = get_viewport_size();
		if (vp_size.x <= 0) vp_size = Vector2(960, 540);
		sub_menu_panel->set_position(Vector2(MAX(20.0f, vp_size.x - 420.0f), MAX(20.0f, vp_size.y - 190.0f)));
	} else {
		sub_menu_panel->set_position(Vector2(MAX(20.0f, act_gpos.x - 230.0f), MAX(20.0f, act_gpos.y - 40.0f)));
	}
	sub_menu_panel->show();
	if (sub_menu_vbox->get_child_count() > 1) {
		Button *first_btn = Object::cast_to<Button>(sub_menu_vbox->get_child(1));
		if (first_btn) {
			first_btn->grab_focus();
		}
	}
}

void ClockGearsBattleScene::_close_sub_menu() {
	if (sub_menu_panel) {
		sub_menu_panel->hide();
	}
}

void ClockGearsBattleScene::_on_btn_attack_pressed() {
	_show_target_selector(1); // 1 = Attack
}

void ClockGearsBattleScene::_on_btn_skill_pressed() {
	Ref<ClockGearsBattleUnit> active = controller.is_valid() ? controller->get_active_unit() : Ref<ClockGearsBattleUnit>();
	if (active.is_valid() && active->get_mp() <= 0) {
		_display_message(String::utf8("¡No tienes MP suficiente!"));
		return;
	}
	_show_skill_selector();
}

void ClockGearsBattleScene::_on_btn_item_pressed() {
	_show_item_selector();
}

void ClockGearsBattleScene::_on_btn_defend_pressed() {
	_close_sub_menu();
	if (controller.is_valid()) {
		controller->action_defend();
		_refresh_ui();
		_check_battle_state();
	}
}

void ClockGearsBattleScene::_on_btn_flee_pressed() {
	_close_sub_menu();
	if (controller.is_valid()) {
		controller->action_flee();
		_refresh_ui();
		_check_battle_state();
	}
}

void ClockGearsBattleScene::_on_target_selected(int p_action_type, int p_target_idx, int p_sub_id, const String &p_sub_str) {
	_close_sub_menu();
	if (!controller.is_valid()) return;

	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();
	if (active.is_valid()) {
		_trigger_visual_anim(active, "attack", 0.45f);
	}

	Array targets = (p_action_type == 3) ? controller->get_party_units() : controller->get_enemy_units();
	if (p_target_idx >= 0 && p_target_idx < targets.size()) {
		Ref<ClockGearsBattleUnit> tgt = targets[p_target_idx];
		if (tgt.is_valid()) {
			_trigger_visual_anim(tgt, tgt->get_is_dead() ? "death" : "hurt", 0.45f);
		}
	}

	if (p_action_type == 1) {
		controller->action_attack(p_target_idx);
	} else if (p_action_type == 2) {
		controller->action_skill(p_sub_id, p_target_idx);
	} else if (p_action_type == 3) {
		controller->action_item(p_sub_str, p_target_idx);
	}

	_refresh_ui();
	_check_battle_state();

	if (btn_attack && !btn_attack->is_disabled()) {
		btn_attack->grab_focus();
	}
}

bool ClockGearsBattleScene::select_action_attack(int p_target_idx) {
	if (!controller.is_valid()) return false;
	bool res = controller->action_attack(p_target_idx);
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_skill(int p_skill_id, int p_target_idx) {
	if (!controller.is_valid()) return false;
	bool res = controller->action_skill(p_skill_id, p_target_idx);
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_item(const String &p_item_name, int p_target_idx) {
	if (!controller.is_valid()) return false;
	bool res = controller->action_item(p_item_name, p_target_idx);
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_defend() {
	if (!controller.is_valid()) return false;
	bool res = controller->action_defend();
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_flee() {
	if (!controller.is_valid()) return false;
	bool res = controller->action_flee();
	_refresh_ui();
	_check_battle_state();
	return res;
}

int ClockGearsBattleScene::get_battle_result() const {
	return controller.is_valid() ? controller->get_result() : int(ClockGearsBattleController::RESULT_NONE);
}

bool ClockGearsBattleScene::is_battle_finished() const {
	return controller.is_valid() && controller->is_finished();
}

ClockGearsBattleScene::ClockGearsBattleScene() {
	controller.instantiate();
}

ClockGearsBattleScene::~ClockGearsBattleScene() {
}
