#include "clockgears_battle_window.h"
#include "clockgears_debug_panel.h"
#include "core/clockgears_data_manager.h"
#include "clockgears_fx_manager.h"
#include "runtime/clockgears_ui_design_layer.h"
#include "runtime/clockgears_window_style.h"
#include "editor/clockgears_theme.h"
#include "editor/themes/editor_scale.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/placeholder_textures.h"
#include "scene/resources/style_box_flat.h"

static String s_fix_mojibake(const String &p_str) {
	if (p_str.find(String::utf8("Ã")) == -1) {
		return p_str;
	}
	PackedByteArray bytes;
	for (int i = 0; i < p_str.length(); i++) {
		char32_t c = p_str[i];
		bytes.append((uint8_t)(c & 0xFF));
	}
	String fixed = String::utf8((const char *)bytes.ptr(), bytes.size());
	return fixed.is_empty() ? p_str : fixed;
}

static const ClockGearsSpriteAnimation *s_find_animation_ci(const ClockGearsSpriteSheetData &p_data, const String &p_name, int p_tex_h = 0) {
	const ClockGearsSpriteAnimation *best_match = nullptr;
	String target_lower = p_name.to_lower();
	for (const KeyValue<String, ClockGearsSpriteAnimation> &E : p_data.animations) {
		if (E.key.to_lower() == target_lower) {
			if (!best_match) {
				best_match = &E.value;
			} else if (E.value.get_frame_count() > best_match->get_frame_count()) {
				// Siempre preferir la animación que tenga más frames (ej. Idle de 17 frames vs idle viejo de 2 frames)
				best_match = &E.value;
			}
		}
	}
	return best_match;
}

static Rect2i s_default_unit_frame_rect(const ClockGearsSpriteSheetData &p_sp, const Ref<Texture2D> &p_tex) {
	if (!p_tex.is_valid() || p_tex->get_width() <= 0 || p_tex->get_height() <= 0) {
		return Rect2i(0, 0, 32, 32);
	}
	int tw = p_tex->get_width();
	int th = p_tex->get_height();
	// Si no hay animaciones configuradas y es una imagen única (w <= h), usar tamaño real completo (ej. 48x48).
	if (p_sp.animations.is_empty() && tw <= th) {
		return Rect2i(0, 0, tw, th);
	}
	int cols = p_sp.cols > 0 ? p_sp.cols : 4;
	int fw = p_sp.frame_w > 0 ? p_sp.frame_w : (tw / cols);
	int fh = p_sp.frame_h > 0 ? p_sp.frame_h : th;
	if (tw > th && (fw >= tw || fw <= 0)) {
		fw = th;
	}
	if (fw <= 0 || fw > tw) fw = tw;
	if (fh <= 0 || fh > th) fh = th;
	if (th > 32 && fh < th) {
		fh = th;
		fw = MAX(fw, th);
		if (fw > tw) fw = tw;
	}
	return Rect2i(0, 0, fw, fh);
}

bool ClockGearsBattleScene::is_texture_ready(const Ref<Texture2D> &p_tex) {
	if (!p_tex.is_valid()) return false;
	if (Object::cast_to<PlaceholderTexture2D>(p_tex.ptr()) != nullptr) return false;
	return p_tex->get_width() > 0 && p_tex->get_height() > 0;
}

bool ClockGearsBattleScene::can_safely_resource_load_image(const String &p_path) {
	if (!ResourceLoader::exists(p_path)) return false;
	if (p_path.begins_with("res://")) {
		String imp_path = p_path + ".import";
		if (FileAccess::exists(imp_path)) {
			String imp_txt = FileAccess::get_file_as_string(imp_path);
			if (imp_txt.find("dest_files=") == -1) {
				return false;
			}
		}
	}
	return true;
}

Ref<Texture2D> ClockGearsBattleScene::load_texture_with_fallbacks(const String &p_path) {
	if (p_path.strip_edges().is_empty()) return Ref<Texture2D>();

	String fixed_path = s_fix_mojibake(p_path);

	if (can_safely_resource_load_image(fixed_path)) {
		Ref<Texture2D> t = ResourceLoader::load(fixed_path);
		if (is_texture_ready(t)) return t;
	}
	if (fixed_path != p_path && can_safely_resource_load_image(p_path)) {
		Ref<Texture2D> t = ResourceLoader::load(p_path);
		if (is_texture_ready(t)) return t;
	}

	Vector<String> try_paths;
	try_paths.push_back(fixed_path);
	if (fixed_path != p_path) try_paths.push_back(p_path);

	// Localizar ruta absoluta si pertenece al proyecto
	if (ProjectSettings::get_singleton()) {
		String loc_path = ProjectSettings::get_singleton()->localize_path(fixed_path);
		if (!loc_path.is_empty() && loc_path != fixed_path) {
			try_paths.push_back(loc_path);
		}
		String gpath = ProjectSettings::get_singleton()->globalize_path(fixed_path);
		if (!gpath.is_empty() && gpath != fixed_path) {
			try_paths.push_back(gpath);
		}
	}

	String fname = fixed_path.get_file();
	if (!fname.is_empty()) {
		try_paths.push_back("res://img/battlebacks/" + fname);
		try_paths.push_back("res://img/battlers/" + fname);
		try_paths.push_back("res://img/enemies/" + fname);
		try_paths.push_back("res://img/characters/" + fname);
		try_paths.push_back("res://img/animations/" + fname);
		try_paths.push_back("res://img/" + fname);
		try_paths.push_back("res://" + fname);

		if (ProjectSettings::get_singleton()) {
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/battlebacks/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/battlers/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/enemies/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/characters/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/animations/" + fname));
			try_paths.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/" + fname));
		}
	}

	for (int i = 0; i < try_paths.size(); i++) {
		const String &tp = try_paths[i];
		if (can_safely_resource_load_image(tp)) {
			Ref<Texture2D> t = ResourceLoader::load(tp);
			if (is_texture_ready(t)) return t;
		}
		String file_read_path = tp;
		if (tp.begins_with("res://") && ProjectSettings::get_singleton()) {
			String gp = ProjectSettings::get_singleton()->globalize_path(tp);
			if (!gp.is_empty() && FileAccess::exists(gp)) {
				file_read_path = gp;
			}
		}
		if (FileAccess::exists(file_read_path)) {
			Ref<Image> img = Image::load_from_file(file_read_path);
			if (img.is_valid() && !img->is_empty()) {
				Ref<ImageTexture> it = ImageTexture::create_from_image(img);
				if (is_texture_ready(it)) return it;
			}
		}
	}

	// Fallback por escaneo en subdirectorios img/
	String search_dirs[] = { "res://img/battlebacks", "res://img/battlers", "res://img/enemies", "res://img/characters", "res://img/animations", "res://img" };
	for (size_t d = 0; d < sizeof(search_dirs) / sizeof(search_dirs[0]); d++) {
		Ref<DirAccess> da = DirAccess::open(search_dirs[d]);
		if (da.is_valid()) {
			da->list_dir_begin();
			String file = da->get_next();
			while (!file.is_empty()) {
				if (!da->current_is_dir() && !file.ends_with(".import")) {
					if (file.to_lower() == fname.to_lower() || (fname.get_extension() == file.get_extension() && file.begins_with(fname.substr(0, 3)))) {
						String cand_p = search_dirs[d].path_join(file);
						Ref<Image> img = Image::load_from_file(cand_p);
						if (img.is_valid() && !img->is_empty()) {
							Ref<ImageTexture> it = ImageTexture::create_from_image(img);
							if (it.is_valid()) return it;
						}
					}
				}
				file = da->get_next();
			}
		}
	}

	return Ref<Texture2D>();
}

Ref<Texture2D> ClockGearsBattleScene::create_placeholder_marker_texture() {
	const int W = 64;
	const int H = 64;
	Ref<Image> img = Image::create_empty(W, H, false, Image::FORMAT_RGBA8);
	img->fill(Color(0, 0, 0, 0));

	Color sil_fill(0.14f, 0.16f, 0.22f, 0.92f);
	Color sil_outline(0.45f, 0.52f, 0.68f, 0.95f);

	// Draw silhouette bust (head circle + shoulders arc)
	Vector2 head_c(32.0f, 20.0f);
	float head_r = 10.5f;
	Vector2 body_c(32.0f, 54.0f);
	float body_rx = 20.0f;
	float body_ry = 20.0f;

	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			Vector2 p(x + 0.5f, y + 0.5f);
			float dh = p.distance_to(head_c);
			float dx = (p.x - body_c.x) / body_rx;
			float dy = (p.y - body_c.y) / body_ry;
			float db = Math::sqrt(dx * dx + dy * dy);
			bool in_head = (dh <= head_r);
			bool in_body = (p.y >= 31.0f && p.y <= 58.0f && db <= 1.0f);
			if (in_head || in_body) {
				bool edge = (in_head && dh >= head_r - 1.5f) || (in_body && (db >= 0.92f || p.y >= 56.5f));
				img->set_pixel(x, y, edge ? sil_outline : sil_fill);
			}
		}
	}

	// Draw crisp "?" bitmap in the center of the silhouette
	const char *q_rows[11] = {
		"  #####  ",
		" ####### ",
		"##     ##",
		"      ## ",
		"     ##  ",
		"   ###   ",
		"   ##    ",
		"   ##    ",
		"         ",
		"   ##    ",
		"   ##    "
	};
	Color q_col(0.95f, 0.82f, 0.35f, 1.0f);
	int qx0 = 32 - 9;
	int qy0 = 26;
	for (int ry = 0; ry < 11; ry++) {
		for (int rx = 0; rx < 9; rx++) {
			if (q_rows[ry][rx] == '#') {
				for (int sy = 0; sy < 2; sy++) {
					for (int sx = 0; sx < 2; sx++) {
						int px = qx0 + rx * 2 + sx;
						int py = qy0 + ry * 2 + sy;
						if (px >= 0 && px < W && py >= 0 && py < H) {
							img->set_pixel(px, py, q_col);
						}
					}
				}
			}
		}
	}

	return ImageTexture::create_from_image(img);
}

void ClockGearsBattleScene::resolve_enemy_preview_sprite(const Dictionary &p_enemy_dict, Ref<Texture2D> &r_tex, Rect2 &r_src_region, String &r_source) {
	r_tex = Ref<Texture2D>();
	r_src_region = Rect2();
	r_source = "placeholder";

	Dictionary ed = p_enemy_dict.duplicate(true);
	ClockGearsSpriteSheetData::migrate_enemy_record(ed);

	String main_graphic = String(ed.get("graphic_path", "")).strip_edges();
	Dictionary sp_dict = ed.get("sprite_data", Dictionary());
	if (!main_graphic.is_empty() && !sp_dict.has("graphic_path")) {
		sp_dict["graphic_path"] = main_graphic;
	}
	ClockGearsSpriteSheetData sp_data;
	sp_data.from_dict(sp_dict);

	// 1. Primer frame de la ranura Idle (si tiene frames y textura válida)
	const ClockGearsSpriteAnimation *idle_anim = s_find_animation_ci(sp_data, "idle");
	if (idle_anim && idle_anim->get_frame_count() > 0) {
		String sheet_path = idle_anim->spritesheet_path.strip_edges();
		if (!idle_anim->frames_data.is_empty() && !idle_anim->frames_data[0].sheet_path.strip_edges().is_empty()) {
			sheet_path = idle_anim->frames_data[0].sheet_path.strip_edges();
		}
		if (sheet_path.is_empty()) {
			sheet_path = main_graphic;
		}
		Ref<Texture2D> idle_tex = load_texture_with_fallbacks(sheet_path);
		if (idle_tex.is_valid() && idle_tex->get_width() > 0 && idle_tex->get_height() > 0) {
			ClockGearsAnimationFrame f0 = idle_anim->get_frame(0, idle_tex->get_width(), idle_tex->get_height());
			Rect2 reg(f0.region.position.x, f0.region.position.y, f0.region.size.x, f0.region.size.y);
			if (reg.size.x <= 0 || reg.size.y <= 0 || reg.position.x >= idle_tex->get_width() || reg.position.y >= idle_tex->get_height()) {
				int cols = MAX(1, idle_anim->cols > 0 ? idle_anim->cols : (sp_data.cols > 0 ? sp_data.cols : 4));
				int rows = MAX(1, idle_anim->rows > 0 ? idle_anim->rows : (sp_data.rows > 0 ? sp_data.rows : 1));
				reg = Rect2(0, 0, MAX(1, idle_tex->get_width() / cols), MAX(1, idle_tex->get_height() / rows));
			}
			if (reg.position.x + reg.size.x > idle_tex->get_width()) {
				reg.size.x = MAX(1.0f, float(idle_tex->get_width()) - reg.position.x);
			}
			if (reg.position.y + reg.size.y > idle_tex->get_height()) {
				reg.size.y = MAX(1.0f, float(idle_tex->get_height()) - reg.position.y);
			}
			r_tex = idle_tex;
			r_src_region = reg;
			r_source = "idle";
			return;
		}
	}

	// 2. Si no hay Idle con frames, el gráfico principal del enemigo
	if (!main_graphic.is_empty()) {
		Ref<Texture2D> main_tex = load_texture_with_fallbacks(main_graphic);
		if (main_tex.is_valid() && main_tex->get_width() > 0 && main_tex->get_height() > 0) {
			r_tex = main_tex;
			r_src_region = Rect2(0, 0, main_tex->get_width(), main_tex->get_height());
			r_source = "graphic_path";
			return;
		}
	}

	// 3. Si tampoco hay gráfico principal, icono de marcador (silueta con '?'), nunca un cuadrado blanco
	r_tex = create_placeholder_marker_texture();
	if (r_tex.is_valid()) {
		r_src_region = Rect2(0, 0, r_tex->get_width(), r_tex->get_height());
	}
	r_source = "placeholder";
}

void ClockGearsBattleScene::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_battle", "troop_id", "can_escape", "can_lose", "bgm", "msg_window", "test_mode"), &ClockGearsBattleScene::setup_battle, DEFVAL(""), DEFVAL((ClockGearsMessageWindow *)nullptr), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_controller"), &ClockGearsBattleScene::get_controller);
	ClassDB::bind_method(D_METHOD("advance_time", "delta"), &ClockGearsBattleScene::advance_time);
	ClassDB::bind_method(D_METHOD("select_action_attack", "target_idx"), &ClockGearsBattleScene::select_action_attack);
	ClassDB::bind_method(D_METHOD("select_action_skill", "skill_id", "target_idx"), &ClockGearsBattleScene::select_action_skill);
	ClassDB::bind_method(D_METHOD("select_action_item", "item_name", "target_idx"), &ClockGearsBattleScene::select_action_item);
	ClassDB::bind_method(D_METHOD("select_action_defend"), &ClockGearsBattleScene::select_action_defend);
	ClassDB::bind_method(D_METHOD("select_action_flee"), &ClockGearsBattleScene::select_action_flee);
	ClassDB::bind_method(D_METHOD("finish_battle_immediate"), &ClockGearsBattleScene::finish_battle_immediate);
	ClassDB::bind_method(D_METHOD("refresh_window_styles"), &ClockGearsBattleScene::refresh_window_styles);
	ClassDB::bind_method(D_METHOD("get_battle_result"), &ClockGearsBattleScene::get_battle_result);
	ClassDB::bind_method(D_METHOD("is_battle_finished"), &ClockGearsBattleScene::is_battle_finished);
	ClassDB::bind_method(D_METHOD("is_initiative_bar_visible"), &ClockGearsBattleScene::is_initiative_bar_visible);
	ClassDB::bind_method(D_METHOD("is_formula_log_visible"), &ClockGearsBattleScene::is_formula_log_visible);
	ClassDB::bind_method(D_METHOD("get_enemy_node_pos", "idx"), &ClockGearsBattleScene::get_enemy_node_pos);
	ClassDB::bind_method(D_METHOD("get_enemy_sprite_rect", "idx"), &ClockGearsBattleScene::get_enemy_sprite_rect);
	ClassDB::bind_method(D_METHOD("get_enemy_hud_group_rect", "idx"), &ClockGearsBattleScene::get_enemy_hud_group_rect);
	ClassDB::bind_method(D_METHOD("get_enemy_hp_bar_rect", "idx"), &ClockGearsBattleScene::get_enemy_hp_bar_rect);
	ClassDB::bind_method(D_METHOD("get_battle_field_rect"), &ClockGearsBattleScene::get_battle_field_rect);
	ClassDB::bind_method(D_METHOD("get_enemy_effect_center", "idx"), &ClockGearsBattleScene::get_enemy_effect_center);
	ClassDB::bind_method(D_METHOD("get_enemy_label_text", "idx"), &ClockGearsBattleScene::get_enemy_label_text);
	ClassDB::bind_method(D_METHOD("is_enemy_hp_bar_visible", "idx"), &ClockGearsBattleScene::is_enemy_hp_bar_visible);
	ClassDB::bind_method(D_METHOD("get_enemy_current_visual_anim", "idx"), &ClockGearsBattleScene::get_enemy_current_visual_anim);
	ClassDB::bind_method(D_METHOD("get_drawn_battleback_rect", "slot"), &ClockGearsBattleScene::get_drawn_battleback_rect);
	ClassDB::bind_method(D_METHOD("get_custom_ui_layer"), &ClockGearsBattleScene::get_custom_ui_layer);

	ADD_SIGNAL(MethodInfo("battle_finished", PropertyInfo(Variant::INT, "result")));
}

void ClockGearsBattleScene::advance_time(float p_delta) {
	_update_battler_visuals(p_delta);
}

void ClockGearsBattleScene::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		set_process_unhandled_input(true);
		set_process(true);
		_setup_ui();
		_refresh_ui();
	} else if (p_what == NOTIFICATION_PROCESS) {
		_update_battler_visuals(get_process_delta_time());
		if (controller.is_valid() && controller->is_finished() && !waiting_finish_confirmation && action_seq_state == SEQ_IDLE) {
			_check_battle_state();
		}
	}
}

void ClockGearsBattleScene::_setup_ui() {
	if (main_container) return;

	set_layer(50); // Superior al mapa y tilesets

	// Fondo oscuro plano por defecto (cuando no hay imagen de fondo)
	bg_rect = memnew(ColorRect);
	bg_rect->set_anchors_preset(Control::PRESET_FULL_RECT);
	bg_rect->set_color(Color(0.06f, 0.08f, 0.12f, 1.0f));
	bg_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	add_child(bg_rect);

	// Fondo y suelo de la tropa (mismo dibujado compartido que la vista previa y Ajustar)
	battleback_bg_layer = memnew(Control);
	battleback_bg_layer->set_anchors_preset(Control::PRESET_FULL_RECT);
	battleback_bg_layer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	battleback_bg_layer->connect("draw", callable_mp(this, &ClockGearsBattleScene::_on_battleback_bg_layer_draw));
	battleback_bg_layer->connect("resized", callable_mp((CanvasItem *)battleback_bg_layer, &CanvasItem::queue_redraw));
	add_child(battleback_bg_layer);

	battleback_floor_layer = memnew(Control);
	battleback_floor_layer->set_anchors_preset(Control::PRESET_FULL_RECT);
	battleback_floor_layer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	battleback_floor_layer->connect("draw", callable_mp(this, &ClockGearsBattleScene::_on_battleback_floor_layer_draw));
	battleback_floor_layer->connect("resized", callable_mp((CanvasItem *)battleback_floor_layer, &CanvasItem::queue_redraw));
	add_child(battleback_floor_layer);

	// Campo posicionado libremente para enemigos según la posición guardada en la tropa (dibujo unificado con vista previa)
	enemies_field = memnew(Control);
	enemies_field->set_anchors_preset(Control::PRESET_FULL_RECT);
	enemies_field->set_clip_contents(true);
	enemies_field->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	enemies_field->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	enemies_field->connect("draw", callable_mp(this, &ClockGearsBattleScene::_on_enemies_field_draw));
	enemies_field->connect("resized", callable_mp(this, &ClockGearsBattleScene::_sync_enemy_hit_nodes_layout));
	add_child(enemies_field);

	main_container = memnew(PanelContainer);
	main_container->set_anchors_preset(Control::PRESET_FULL_RECT);
	main_container->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	Ref<StyleBoxFlat> empty_sb;
	empty_sb.instantiate();
	empty_sb->set_bg_color(Color(0, 0, 0, 0));
	empty_sb->set_content_margin(SIDE_LEFT, 16);
	empty_sb->set_content_margin(SIDE_RIGHT, 24);
	empty_sb->set_content_margin(SIDE_TOP, 12);
	empty_sb->set_content_margin(SIDE_BOTTOM, 16);
	main_container->add_theme_style_override("panel", empty_sb);
	main_container->connect("resized", callable_mp(this, &ClockGearsBattleScene::refresh_window_styles));
	add_child(main_container);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	main_vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
	main_vbox->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	main_vbox->add_theme_constant_override("separation", 10);
	main_container->add_child(main_vbox);

	// 1. Barra de Iniciativa (Arriba) - oculta por defecto salvo que F3 "Mostrar turnos" esté activo
	initiative_panel = memnew(PanelContainer);
	initiative_panel->set_visible(ClockGearsDebugOverlay::is_show_battle_turns());
	main_vbox->add_child(initiative_panel);

	initiative_bar = memnew(HBoxContainer);
	initiative_bar->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	initiative_bar->set_custom_minimum_size(Size2(0, 36));
	initiative_bar->add_theme_constant_override("separation", 8);
	initiative_panel->add_child(initiative_bar);

	// Espaciador central transparente para el campo de batalla
	Control *center_spacer = memnew(Control);
	center_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	center_spacer->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	center_spacer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	main_vbox->add_child(center_spacer);

	// Log de Combate (Abajo, compacto y no bloqueante del turno; oculto por defecto salvo F3 "Mostrar fórmulas")
	battle_log_panel = memnew(PanelContainer);
	battle_log_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	battle_log_panel->set_custom_minimum_size(Size2(0, 30));

	battle_log_label = memnew(Label);
	battle_log_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	battle_log_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	battle_log_label->add_theme_color_override("font_color", Color(1.0f, 0.92f, 0.65f));
	battle_log_label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	battle_log_panel->add_child(battle_log_label);
	main_vbox->add_child(battle_log_panel);
	battle_log_panel->set_visible(false);

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
	actions_panel->set_custom_minimum_size(Size2(160, 130));
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

	// Capa UI personalizada (diseños de Batalla + botones añadidos)
	custom_ui_layer = memnew(ClockGearsUiDesignLayer);
	custom_ui_layer->set_name("CustomBattleUiLayer");
	custom_ui_layer->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	custom_ui_layer->set_window_type_fallback("battle_commands");
	custom_ui_layer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	custom_ui_layer->set_visible(false);
	custom_ui_layer->connect("action_triggered", callable_mp(this, &ClockGearsBattleScene::_on_custom_ui_action));
	add_child(custom_ui_layer);

	// 4. Submenú flotante para selección de objetivo / habilidad / objeto
	sub_menu_panel = memnew(PanelContainer);
	sub_menu_panel->set_custom_minimum_size(Size2(220, 160));
	sub_menu_panel->hide();
	add_child(sub_menu_panel);

	sub_menu_vbox = memnew(VBoxContainer);
	sub_menu_vbox->add_theme_constant_override("separation", 4);
	sub_menu_panel->add_child(sub_menu_vbox);

	refresh_window_styles();
}

void ClockGearsBattleScene::refresh_window_styles() {
	ClockGearsWindowStyleManager::apply_window_style(initiative_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_window_style(battle_log_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_window_style(actions_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_window_style(sub_menu_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_buttons_in_container(actions_vbox, "battle_commands");
	ClockGearsWindowStyleManager::apply_buttons_in_container(sub_menu_vbox, "battle_commands");
	if (party_container) {
		for (int i = 0; i < party_container->get_child_count(); i++) {
			Control *card = Object::cast_to<Control>(party_container->get_child(i));
			if (card) {
				Dictionary ov;
				if (bool(card->get_meta("_cg_is_turn", false))) {
					Dictionary st = ClockGearsDataManager::get_singleton() ? ClockGearsDataManager::get_singleton()->resolve_window_style("battle_party") : Dictionary();
					String bt = String(st.get("border_type", "rounded"));
					if (bt != "none" && bt != "image") {
						ov["border_color"] = "#ffd959ff";
						ov["border_width"] = MAX(2, int(st.get("border_width", 2)) + 1);
					}
				}
				ClockGearsWindowStyleManager::apply_window_style(card, "battle_party", ov);
			}
		}
	}
}

void ClockGearsBattleScene::setup_battle(int p_troop_id, bool p_can_escape, bool p_can_lose, const String &p_bgm, ClockGearsMessageWindow *p_msg_window, bool p_test_mode) {
	if (!controller.is_valid()) {
		controller.instantiate();
	}
	controller->set_test_mode(p_test_mode);
	controller->setup_battle(p_troop_id, p_can_escape, p_can_lose, p_bgm);
	waiting_finish_confirmation = false;

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
	if (initiative_panel) {
		initiative_panel->set_visible(ClockGearsDebugOverlay::is_show_battle_turns());
	}
	if (battle_log_panel && battle_log_label) {
		bool show_f = ClockGearsDebugOverlay::is_show_battle_formulas() && !battle_log_label->get_text().is_empty();
		battle_log_panel->set_visible(show_f);
	}

	bool any_reg_size_changed = false;
	for (int i = 0; i < battler_visuals.size(); i++) {
		ClockGearsBattlerVisual &vis = battler_visuals.write[i];
		if (vis.unit.is_valid()) {
			float st = vis.unit->get_anim_slot_timer();
			if (st > 0.0f) {
				st -= p_delta;
				vis.unit->set_anim_slot_timer(MAX(0.0f, st));
			}
			float et = vis.unit->get_active_effect_timer();
			if (et > 0.0f) {
				et -= p_delta;
				if (et <= 0.0f) {
					vis.unit->set_active_effect_timer(0.0f);
					if (vis.effect_rect) vis.effect_rect->hide();
				} else {
					vis.unit->set_active_effect_timer(et);
				}
			}
		}

		// Efecto visual sobre el objetivo (CAT_ANIMATIONS)
		if (vis.effect_rect && vis.unit.is_valid() && vis.unit->get_active_effect_anim_id() > 0 && vis.unit->get_active_effect_timer() > 0.0f) {
			int eff_id = vis.unit->get_active_effect_anim_id();
			if (vis.loaded_effect_id != eff_id) {
				vis.loaded_effect_id = eff_id;
				vis.effect_frame_idx = 0;
				vis.effect_frame_timer = 0.0f;
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				Array anim_list = dm ? dm->get_category_array(CAT_ANIMATIONS) : Array();
				for (int a = 0; a < anim_list.size(); a++) {
					Dictionary ad = anim_list[a];
					if (int(ad.get("id", a + 1)) == eff_id) {
						String eff_path = ad.get("spritesheet", ad.get("graphic", ""));
						vis.effect_sprite_data.from_dict(ad);
						vis.effect_base_texture = load_texture_with_fallbacks(eff_path);
						if (vis.effect_atlas_texture.is_null()) vis.effect_atlas_texture.instantiate();
						if (vis.effect_base_texture.is_valid()) {
							vis.effect_atlas_texture->set_atlas(vis.effect_base_texture);
							vis.effect_rect->set_texture(vis.effect_atlas_texture);
						}
						break;
					}
				}
			}
			if (vis.effect_base_texture.is_valid() && vis.effect_atlas_texture.is_valid()) {
				vis.effect_rect->show();
				int cols = MAX(1, vis.effect_sprite_data.cols > 0 ? vis.effect_sprite_data.cols : 5);
				int rows = MAX(1, vis.effect_sprite_data.rows > 0 ? vis.effect_sprite_data.rows : 1);
				int total_ef = MAX(1, cols * rows);
				vis.effect_frame_timer += p_delta;
				if (vis.effect_frame_timer >= 0.07f) {
					vis.effect_frame_timer -= 0.07f;
					vis.effect_frame_idx = (vis.effect_frame_idx + 1) % total_ef;
				}
				int cw = vis.effect_base_texture->get_width() / cols;
				int ch = vis.effect_base_texture->get_height() / rows;
				int cx = (vis.effect_frame_idx % cols) * cw;
				int cy = (vis.effect_frame_idx / cols) * ch;
				vis.effect_atlas_texture->set_region(Rect2(cx, cy, cw, ch));
			}
		} else if (vis.effect_rect) {
			vis.effect_rect->hide();
		}

		if (vis.base_texture.is_null() || vis.atlas_texture.is_null()) continue;

		if (vis.unit.is_valid() && vis.unit->get_is_dead()) {
			String death_name = vis.unit->resolve_slot_animation_name("death");
			const ClockGearsSpriteAnimation *death_anim = s_find_animation_ci(vis.sprite_data, death_name);
			if (death_anim && vis.current_anim != death_anim->name) {
				vis.current_anim = death_anim->name;
				vis.current_frame_idx = 0;
				vis.frame_timer = 0.0f;
				vis.current_frame_duration = 0.0f;
			}
		} else {
			if (vis.state_duration > 0.0f) {
				vis.state_duration -= p_delta;
				if (vis.state_duration <= 0.0f) {
					String idle_name = vis.unit.is_valid() ? vis.unit->resolve_slot_animation_name("idle") : "Idle";
					const ClockGearsSpriteAnimation *idle_anim = s_find_animation_ci(vis.sprite_data, idle_name);
					vis.current_anim = idle_anim ? idle_anim->name : idle_name;
					if (vis.unit.is_valid()) {
						vis.unit->set_current_anim_slot(vis.current_anim);
					}
					vis.current_frame_idx = 0;
					vis.frame_timer = 0.0f;
					vis.current_frame_duration = 0.0f;
				}
			}
		}

		Rect2i r;
		const ClockGearsSpriteAnimation *cur_anim = nullptr;
		if (vis.sprite_data.animations.has(vis.current_anim)) {
			cur_anim = &vis.sprite_data.animations[vis.current_anim];
		}
		const ClockGearsSpriteAnimation *ci_anim = s_find_animation_ci(vis.sprite_data, vis.current_anim);
		if (ci_anim && (!cur_anim || ci_anim->get_frame_count() > cur_anim->get_frame_count())) {
			cur_anim = ci_anim;
		}
		if (!cur_anim) {
			String idle_name = vis.unit.is_valid() ? vis.unit->resolve_slot_animation_name("idle") : "Idle";
			cur_anim = s_find_animation_ci(vis.sprite_data, idle_name);
		}

		if (cur_anim && cur_anim->get_frame_count() > 0) {
			const ClockGearsSpriteAnimation &an = *cur_anim;
			if (!an.spritesheet_path.is_empty() && an.spritesheet_path != vis.current_sheet_path) {
				Ref<Texture2D> anim_tex = load_texture_with_fallbacks(an.spritesheet_path);
				if (anim_tex.is_valid()) {
					vis.base_texture = anim_tex;
					vis.atlas_texture->set_atlas(vis.base_texture);
					vis.current_sheet_path = an.spritesheet_path;
				}
			}
			int total_f = an.get_frame_count();
			bool loop = (vis.current_anim.to_lower().find("idle") != -1) ? true : an.loop;

			if (vis.current_frame_idx < 0 || vis.current_frame_idx >= total_f) {
				vis.current_frame_idx = 0;
			}
			if (vis.current_frame_duration <= 0.0f) {
				ClockGearsAnimationFrame f0 = an.get_frame(vis.current_frame_idx);
				vis.current_frame_duration = f0.get_duration(an.fps);
			}

			vis.frame_timer += p_delta;
			while (vis.frame_timer >= vis.current_frame_duration && vis.current_frame_duration > 0.0f) {
				vis.frame_timer -= vis.current_frame_duration;
				if (an.random_order) {
					vis.current_frame_idx = (total_f > 1) ? (Math::rand() % total_f) : 0;
				} else {
					vis.current_frame_idx++;
					if (vis.current_frame_idx >= total_f) {
						if (loop) {
							vis.current_frame_idx = 0;
						} else {
							vis.current_frame_idx = total_f - 1;
							break;
						}
					}
				}
				ClockGearsAnimationFrame next_f = an.get_frame(vis.current_frame_idx);
				vis.current_frame_duration = next_f.get_duration(an.fps);
			}

			ClockGearsAnimationFrame f = an.get_frame(vis.current_frame_idx, vis.base_texture.is_valid() ? vis.base_texture->get_width() : 0, vis.base_texture.is_valid() ? vis.base_texture->get_height() : 0);
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
			// Si falta una ranura o está vacía, usa el frame 0 sin dar error
			r = s_default_unit_frame_rect(vis.sprite_data, vis.base_texture);
		}

		Size2 prev_reg_sz = vis.atlas_texture->get_region().size;
		vis.atlas_texture->set_region(Rect2(r.position.x, r.position.y, r.size.x, r.size.y));
		if (!vis.is_actor && prev_reg_sz != Size2(r.size.x, r.size.y)) {
			any_reg_size_changed = true;
		}
	}

	if (any_reg_size_changed) {
		_sync_enemy_hit_nodes_layout();
	} else if (enemies_field) {
		enemies_field->queue_redraw();
	}

	if (action_seq_state != SEQ_IDLE) {
		_step_action_sequence(p_delta);
	}
}

void ClockGearsBattleScene::_trigger_visual_anim(const Ref<ClockGearsBattleUnit> &p_unit, const String &p_anim, float p_duration) {
	for (int i = 0; i < battler_visuals.size(); i++) {
		ClockGearsBattlerVisual &vis = battler_visuals.write[i];
		if (vis.unit == p_unit) {
			const ClockGearsSpriteAnimation *found_anim = s_find_animation_ci(vis.sprite_data, p_anim);
			vis.current_anim = found_anim ? found_anim->name : p_anim;
			vis.current_frame_idx = 0;
			vis.frame_timer = 0.0f;
			vis.current_frame_duration = 0.0f;
			vis.state_duration = p_duration;
			break;
		}
	}
}

void ClockGearsBattleScene::_trigger_unit_trigger(const Ref<ClockGearsBattleUnit> &p_unit, int p_trigger, const String &p_fallback, float p_min_duration) {
	if (!p_unit.is_valid()) return;
	String anim_name = p_unit->get_combat_animation(p_trigger, p_fallback);
	if (anim_name.is_empty()) anim_name = p_fallback;
	_trigger_visual_anim(p_unit, anim_name, p_min_duration);
}

void ClockGearsBattleScene::_start_action_sequence(int p_action_type, int p_target_idx, int p_sub_id, const String &p_sub_str) {
	seq_action_type = p_action_type;
	seq_target_idx = p_target_idx;
	seq_sub_id = p_sub_id;
	seq_sub_str = p_sub_str;

	seq_attacker = controller.is_valid() ? controller->get_active_unit() : Ref<ClockGearsBattleUnit>();
	Array tgts = (p_action_type == 3) ? (controller.is_valid() ? controller->get_party_units() : Array()) : (controller.is_valid() ? controller->get_enemy_units() : Array());
	seq_target = (p_target_idx >= 0 && p_target_idx < tgts.size()) ? Ref<ClockGearsBattleUnit>(tgts[p_target_idx]) : Ref<ClockGearsBattleUnit>();

	// 1. Reproducir Ataque en el atacante
	_trigger_unit_trigger(seq_attacker, ANIM_TRIGGER_ATTACK, "attack", 0.45f);
	action_seq_state = SEQ_ATTACK_ANIM;
	action_seq_timer = 0.45f;

	if (btn_attack) btn_attack->set_disabled(true);
	if (btn_skill) btn_skill->set_disabled(true);
	if (btn_item) btn_item->set_disabled(true);
	if (btn_defend) btn_defend->set_disabled(true);
	if (btn_flee) btn_flee->set_disabled(true);
}

void ClockGearsBattleScene::_step_action_sequence(float p_delta) {
	action_seq_timer -= p_delta;
	if (action_seq_timer <= 0.0f) {
		if (action_seq_state == SEQ_ATTACK_ANIM) {
			// 2. Ejecutar acción de daño y reproducir Daño/Muerte en el objetivo
			int tgt_hp_before = seq_target.is_valid() ? seq_target->get_hp() : 0;
			if (controller.is_valid()) {
				if (seq_action_type == 1) {
					controller->action_attack(seq_target_idx);
				} else if (seq_action_type == 2) {
					controller->action_skill(seq_sub_id, seq_target_idx);
				} else if (seq_action_type == 3) {
					controller->action_item(seq_sub_str, seq_target_idx);
				}
			}
			if (seq_target.is_valid()) {
				if (seq_action_type != 3) {
					int delta = tgt_hp_before - seq_target->get_hp();
					_spawn_enemy_hit_fx(seq_target_idx, Math::abs(delta), delta < 0);
				}
				if (seq_target->get_is_dead()) {
					_trigger_unit_trigger(seq_target, ANIM_TRIGGER_DEATH, "death", 0.6f);
				} else {
					_trigger_unit_trigger(seq_target, ANIM_TRIGGER_DAMAGE, "hurt", 0.4f);
				}
			}
			action_seq_state = SEQ_DAMAGE_ANIM;
			action_seq_timer = 0.4f;
		} else if (action_seq_state == SEQ_DAMAGE_ANIM) {
			// 3. Fin de la secuencia: UI lista para el siguiente turno limpio
			action_seq_state = SEQ_IDLE;
			_refresh_ui();
			_check_battle_state();
			if (btn_attack && !btn_attack->is_disabled()) {
				btn_attack->grab_focus();
			}
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
	if (!controller.is_valid() || !enemies_field || !party_container) return;

	_update_initiative_bar();
	if (initiative_panel) {
		initiative_panel->set_visible(ClockGearsDebugOverlay::is_show_battle_turns());
	}
	if (battle_log_panel && battle_log_label) {
		battle_log_panel->set_visible(ClockGearsDebugOverlay::is_show_battle_formulas() && !battle_log_label->get_text().is_empty());
	}

	// Actualizar fondo y suelo de la tropa (sin imagen -> fondo oscuro plano)
	battleback_bg_tex = load_texture_with_fallbacks(controller->get_troop_bg_path());
	if (battleback_bg_layer) {
		battleback_bg_layer->set_visible(is_texture_ready(battleback_bg_tex));
		battleback_bg_layer->queue_redraw();
	}
	battleback_floor_tex = load_texture_with_fallbacks(controller->get_troop_floor_path());
	if (battleback_floor_layer) {
		battleback_floor_layer->set_visible(is_texture_ready(battleback_floor_tex));
		battleback_floor_layer->queue_redraw();
	}

	battler_visuals.clear();

	// 1. Refrescar enemigos en sus posiciones guardadas (sin tarjeta/rectángulo)
	while (enemies_field->get_child_count() > 0) {
		Node *c = enemies_field->get_child(0);
		enemies_field->remove_child(c);
		memdelete(c);
	}
	enemy_nodes.clear();
	enemy_name_labels.clear();
	enemy_hp_bars.clear();

	Rect2 field_vp = Rect2(Point2(0, 0), enemies_field->get_size());
	if (field_vp.size.x <= 1.0f || field_vp.size.y <= 1.0f) {
		Vector2 vsz = get_viewport_size();
		field_vp = Rect2(Point2(0, 0), (vsz.x > 1.0f && vsz.y > 1.0f) ? vsz : Size2(BATTLE_REF_WIDTH, BATTLE_REF_HEIGHT));
	}

	Array ens = controller->get_enemy_units();
	for (int i = 0; i < ens.size(); i++) {
		Ref<ClockGearsBattleUnit> en = ens[i];
		if (!en.is_valid()) continue;

		Vector2 bpos = en->get_battle_pos();
		if (bpos.x <= 0.0f && bpos.y <= 0.0f) {
			bpos = Vector2(220.0f + i * 160.0f, 180.0f);
		}

		ClockGearsSpriteSheetData sp_data = en->get_sprite_data();
		String tex_path = en->get_graphic_path();
		String active_slot = en->get_current_anim_slot();
		float active_timer = en->get_anim_slot_timer();
		String target_anim_name;
		if (en->get_is_dead()) {
			target_anim_name = en->resolve_slot_animation_name("death");
		} else if (active_timer > 0.0f && !active_slot.is_empty()) {
			target_anim_name = active_slot;
		} else {
			target_anim_name = en->resolve_slot_animation_name("idle");
		}

		const ClockGearsSpriteAnimation *cur_anim = s_find_animation_ci(sp_data, target_anim_name);
		if (!cur_anim && sp_data.animations.has(target_anim_name)) {
			cur_anim = &sp_data.animations[target_anim_name];
		}
		if (!cur_anim) {
			cur_anim = s_find_animation_ci(sp_data, "idle");
		}
		if (!cur_anim && !sp_data.animations.is_empty()) {
			cur_anim = &sp_data.animations.begin()->value;
		}

		if (cur_anim && !cur_anim->spritesheet_path.is_empty()) {
			tex_path = cur_anim->spritesheet_path;
		}
		Ref<Texture2D> base_tex = load_texture_with_fallbacks(tex_path);
		Ref<AtlasTexture> atlas;
		atlas.instantiate();

		Rect2i r;
		if (is_texture_ready(base_tex) && cur_anim && cur_anim->get_frame_count() > 0) {
			ClockGearsAnimationFrame f0 = cur_anim->get_frame(0, base_tex->get_width(), base_tex->get_height());
			if (!f0.sheet_path.is_empty() && f0.sheet_path != tex_path) {
				Ref<Texture2D> f_tex = load_texture_with_fallbacks(f0.sheet_path);
				if (is_texture_ready(f_tex)) {
					base_tex = f_tex;
					tex_path = f0.sheet_path;
				}
			}
			r = f0.region;
		} else if (is_texture_ready(base_tex)) {
			// Frame 0 por defecto si falta una ranura o está vacía
			r = s_default_unit_frame_rect(sp_data, base_tex);
		}

		if (is_texture_ready(base_tex)) {
			atlas->set_atlas(base_tex);
			atlas->set_region(Rect2(r.position.x, r.position.y, r.size.x, r.size.y));
		} else {
			Ref<Texture2D> ph_tex = create_placeholder_marker_texture();
			if (ph_tex.is_valid()) {
				base_tex = ph_tex;
				r = Rect2i(0, 0, ph_tex->get_width(), ph_tex->get_height());
				atlas->set_atlas(base_tex);
				atlas->set_region(Rect2(r.position.x, r.position.y, r.size.x, r.size.y));
			}
		}

		EnemyVisualLayout layout = compute_enemy_visual_layout(bpos, field_vp, Size2(r.size.x, r.size.y), en->get_sprite_scale(), enemies_field ? enemies_field->get_theme_default_font() : Ref<Font>(), en->get_hp_bar_mode(), en->get_hp_bar_pos());

		Control *en_slot = memnew(Control);
		en_slot->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		en_slot->set_position(layout.block_origin);
		en_slot->set_size(layout.full_box.size);

		// Efecto visual sobre el objetivo (centrado sobre el sprite escalado)
		TextureRect *eff_rect = memnew(TextureRect);
		float vp_s = (field_vp.size.x > 0.0f && field_vp.size.y > 0.0f) ? MIN(field_vp.size.x / BATTLE_REF_WIDTH, field_vp.size.y / BATTLE_REF_HEIGHT) : 1.0f;
		Size2 eff_sz(MAX(48.0f * vp_s, layout.draw_spr_rect.size.x), MAX(48.0f * vp_s, layout.draw_spr_rect.size.y));
		eff_rect->set_position(layout.draw_spr_rect.get_center() - eff_sz * 0.5f - layout.block_origin);
		eff_rect->set_size(eff_sz);
		eff_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		eff_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		eff_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		eff_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		eff_rect->hide();
		en_slot->add_child(eff_rect);

		ClockGearsBattlerVisual vis;
		vis.unit = en;
		vis.texture_rect = nullptr;
		vis.effect_rect = eff_rect;
		vis.base_texture = base_tex;
		vis.atlas_texture = atlas;
		vis.sprite_data = sp_data;
		vis.current_anim = cur_anim ? cur_anim->name : target_anim_name;
		vis.current_frame_idx = 0;
		vis.frame_timer = 0.0f;
		vis.current_frame_duration = (cur_anim && cur_anim->get_frame_count() > 0) ? cur_anim->get_frame(0).get_duration(cur_anim->fps) : 0.1f;
		vis.anim_timer = 0.0f;
		vis.state_duration = active_timer;
		vis.is_actor = false;
		vis.current_sheet_path = tex_path;
		battler_visuals.push_back(vis);

		enemies_field->add_child(en_slot);
		enemy_nodes.push_back(en_slot);
	}
	enemies_field->queue_redraw();

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
		pcard->set_meta("_cg_is_turn", is_turn);
		Dictionary pcard_ov;
		if (is_turn) {
			Dictionary st = ClockGearsDataManager::get_singleton() ? ClockGearsDataManager::get_singleton()->resolve_window_style("battle_party") : Dictionary();
			String bt = String(st.get("border_type", "rounded"));
			if (bt != "none" && bt != "image") {
				pcard_ov["border_color"] = "#ffd959ff";
				pcard_ov["border_width"] = MAX(2, int(st.get("border_width", 2)) + 1);
			}
		}
		ClockGearsWindowStyleManager::apply_window_style(pcard, "battle_party", pcard_ov);

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

			String h_idle = hero->get_combat_animation(ANIM_TRIGGER_IDLE, "idle");
			const ClockGearsSpriteAnimation *h_anim = s_find_animation_ci(h_sp, h_idle);
			if (!h_anim) h_anim = s_find_animation_ci(h_sp, "idle");
			if (!h_anim && !h_sp.animations.is_empty()) h_anim = &h_sp.animations.begin()->value;

			ClockGearsBattlerVisual h_vis;
			h_vis.unit = hero;
			h_vis.texture_rect = ptrect;
			h_vis.base_texture = p_tex;
			h_vis.atlas_texture = patlas;
			h_vis.sprite_data = h_sp;
			h_vis.current_anim = h_anim ? h_anim->name : h_idle;
			h_vis.current_frame_idx = 0;
			h_vis.frame_timer = 0.0f;
			h_vis.current_frame_duration = (h_anim && h_anim->get_frame_count() > 0) ? h_anim->get_frame(0).get_duration(h_anim->fps) : 0.1f;
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
	_refresh_custom_ui_layer(can_act);
}

Control *ClockGearsBattleScene::get_custom_ui_layer() const {
	return custom_ui_layer;
}

void ClockGearsBattleScene::_refresh_custom_ui_layer(bool p_can_act) {
	if (!custom_ui_layer) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !dm->has_custom_ui_design("battle")) {
		custom_ui_layer->set_visible(false);
		custom_ui_layer->clear_elements();
		if (actions_panel) actions_panel->set_visible(true);
		return;
	}
	custom_ui_layer->set_window_type_fallback("battle_commands");
	custom_ui_layer->set_preview_mode(false);
	custom_ui_layer->apply_screen_design("battle");
	custom_ui_layer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	custom_ui_layer->set_visible(true);

	const bool has_custom_cmds = (custom_ui_layer->get_element_control("btn_battle_1") != nullptr);
	if (actions_panel) {
		actions_panel->set_visible(!has_custom_cmds);
	}
	static const char *k_cmd_ids[5] = { "btn_battle_1", "btn_battle_2", "btn_battle_3", "btn_battle_4", "btn_battle_5" };
	for (int i = 0; i < 5; i++) {
		if (Control *ec = custom_ui_layer->get_element_control(k_cmd_ids[i])) {
			if (Button *b = Object::cast_to<Button>(ec->find_child("ActionButton", true, false))) {
				b->set_disabled(!p_can_act);
			}
		}
	}
}

void ClockGearsBattleScene::_on_custom_ui_action(const String &p_action, const String &p_elem_id) {
	Array steps;
	if (custom_ui_layer && !p_elem_id.is_empty()) {
		steps = custom_ui_layer->get_element_actions(p_elem_id);
	}
	if (steps.is_empty() && !p_action.strip_edges().is_empty()) {
		Dictionary single;
		single["type"] = p_action.strip_edges().to_lower();
		steps.push_back(single);
	}
	String eid_low = p_elem_id.strip_edges().to_lower();
	if (eid_low == "btn_battle_1") { _on_btn_attack_pressed(); return; }
	if (eid_low == "btn_battle_2") { _on_btn_skill_pressed(); return; }
	if (eid_low == "btn_battle_3") { _on_btn_item_pressed(); return; }
	if (eid_low == "btn_battle_4") { _on_btn_defend_pressed(); return; }
	if (eid_low == "btn_battle_5") { _on_btn_flee_pressed(); return; }

	for (int i = 0; i < steps.size(); i++) {
		if (steps[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary st = steps[i];
		String tp = String(st.get("type", st.get("action", ""))).strip_edges().to_lower();
		if (tp == "attack" || tp == "atacar") { _on_btn_attack_pressed(); return; }
		if (tp == "skill" || tp == "habilidad") { _on_btn_skill_pressed(); return; }
		if (tp == "item" || tp == "objeto") { _on_btn_item_pressed(); return; }
		if (tp == "defend" || tp == "guard" || tp == "defender") { _on_btn_defend_pressed(); return; }
		if (tp == "flee" || tp == "escape" || tp == "huir") { _on_btn_flee_pressed(); return; }
		if (tp == "common_event") {
			int ce_id = int(st.get("common_event_id", st.get("id", 0)));
			if (ce_id > 0 && is_inside_tree() && get_tree()) {
				if (Node *root = get_tree()->get_root()) {
					Node *interp = root->find_child("ClockGearsEventInterpreter", true, false);
					if (interp) interp->call("execute_common_event", ce_id);
				}
			}
			continue;
		}
		ClockGearsUiDesignLayer::execute_common_action_step(st, custom_ui_layer);
	}
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
		String notice_wt;
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			String scr_id = (dm->has_menu("battle_notice") && (dm->has_custom_ui_design("battle_notice") || !dm->has_custom_ui_design("battle")))
					? String("battle_notice")
					: String("battle");
			Dictionary bdesign = dm->has_custom_ui_design(scr_id)
					? dm->get_ui_screen_design(scr_id)
					: dm->get_default_ui_screen_template("battle_notice");
			Array belems = bdesign.get("elements", Array());
			for (int i = 0; i < belems.size(); i++) {
				if (belems[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary el = belems[i];
				String eid = String(el.get("id", "")).to_lower();
				String ename = String(el.get("name", "")).to_lower();
				if (eid == "battle_notice_frame" || ename == "aviso de batalla") {
					message_window->set_meta("_cg_battle_notice_source", el.get("id", "battle_notice_frame"));
					String ws = String(el.get("window_style_id", "")).strip_edges().to_lower();
					if (!ws.is_empty() && ws != "dialog" && ws != "default" && dm->has_custom_ui_design(scr_id)) {
						notice_wt = ws;
					}
					break;
				}
			}
		}
		message_window->set_message_window_type_override(notice_wt);
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
		if (controller->get_test_mode()) {
			finish_battle_immediate();
			return;
		}
		if (res == ClockGearsBattleController::RESULT_VICTORY) {
			String currency = ClockGearsDataManager::get_singleton() ? ClockGearsDataManager::get_singleton()->get_currency_unit() : String("G");
			String vic_msg = vformat(String::utf8("¡Victoria! Has ganado %d EXP y %d %s."), controller->get_total_exp(), controller->get_total_gold(), currency);
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
		battle_log_panel->set_visible(ClockGearsDebugOverlay::is_show_battle_formulas() && !p_log.is_empty());
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
	ClockGearsWindowStyleManager::apply_window_style(sub_menu_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_buttons_in_container(sub_menu_vbox, "battle_commands");

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
	ClockGearsWindowStyleManager::apply_window_style(sub_menu_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_buttons_in_container(sub_menu_vbox, "battle_commands");

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
	ClockGearsWindowStyleManager::apply_window_style(sub_menu_panel, "battle_commands");
	ClockGearsWindowStyleManager::apply_buttons_in_container(sub_menu_vbox, "battle_commands");

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
		Ref<ClockGearsBattleUnit> active = controller->get_active_unit();
		if (active.is_valid()) {
			_trigger_unit_trigger(active, ANIM_TRIGGER_DEFEND, "defend", 0.5f);
		}
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
	_start_action_sequence(p_action_type, p_target_idx, p_sub_id, p_sub_str);
}

bool ClockGearsBattleScene::select_action_attack(int p_target_idx) {
	if (!controller.is_valid()) return false;
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();
	if (active.is_valid()) {
		_trigger_unit_trigger(active, ANIM_TRIGGER_ATTACK, "attack", 0.45f);
	}
	Array targets = controller->get_enemy_units();
	Ref<ClockGearsBattleUnit> tgt;
	if (p_target_idx >= 0 && p_target_idx < targets.size()) {
		tgt = targets[p_target_idx];
	}
	int tgt_hp_before = tgt.is_valid() ? tgt->get_hp() : 0;
	bool res = controller->action_attack(p_target_idx);
	if (tgt.is_valid()) {
		int delta = tgt_hp_before - tgt->get_hp();
		_spawn_enemy_hit_fx(p_target_idx, Math::abs(delta), delta < 0);
		if (tgt->get_is_dead()) {
			_trigger_unit_trigger(tgt, ANIM_TRIGGER_DEATH, "death", 0.6f);
		} else {
			_trigger_unit_trigger(tgt, ANIM_TRIGGER_DAMAGE, "hurt", 0.4f);
		}
	}
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_skill(int p_skill_id, int p_target_idx) {
	if (!controller.is_valid()) return false;
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();
	if (active.is_valid()) {
		_trigger_unit_trigger(active, ANIM_TRIGGER_SKILL, "attack", 0.45f);
	}
	Array targets = controller->get_enemy_units();
	Ref<ClockGearsBattleUnit> tgt;
	if (p_target_idx >= 0 && p_target_idx < targets.size()) {
		tgt = targets[p_target_idx];
	}
	int tgt_hp_before = tgt.is_valid() ? tgt->get_hp() : 0;
	bool res = controller->action_skill(p_skill_id, p_target_idx);
	if (tgt.is_valid()) {
		int delta = tgt_hp_before - tgt->get_hp();
		_spawn_enemy_hit_fx(p_target_idx, Math::abs(delta), delta < 0);
		if (tgt->get_is_dead()) {
			_trigger_unit_trigger(tgt, ANIM_TRIGGER_DEATH, "death", 0.6f);
		} else {
			_trigger_unit_trigger(tgt, ANIM_TRIGGER_DAMAGE, "hurt", 0.4f);
		}
	}
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_item(const String &p_item_name, int p_target_idx) {
	if (!controller.is_valid()) return false;
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();
	if (active.is_valid()) {
		_trigger_unit_trigger(active, ANIM_TRIGGER_ITEM, "idle", 0.45f);
	}
	bool res = controller->action_item(p_item_name, p_target_idx);
	_refresh_ui();
	_check_battle_state();
	return res;
}

bool ClockGearsBattleScene::select_action_defend() {
	if (!controller.is_valid()) return false;
	Ref<ClockGearsBattleUnit> active = controller->get_active_unit();
	if (active.is_valid()) {
		_trigger_unit_trigger(active, ANIM_TRIGGER_DEFEND, "defend", 0.5f);
	}
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

bool ClockGearsBattleScene::is_initiative_bar_visible() const {
	return initiative_panel && initiative_panel->is_visible();
}

bool ClockGearsBattleScene::is_formula_log_visible() const {
	return battle_log_panel && battle_log_panel->is_visible();
}

Vector2 ClockGearsBattleScene::get_enemy_node_pos(int p_idx) const {
	if (p_idx >= 0 && p_idx < enemy_nodes.size() && enemy_nodes[p_idx]) {
		return enemy_nodes[p_idx]->get_position();
	}
	return Vector2();
}

ClockGearsBattleScene::EnemyVisualLayout ClockGearsBattleScene::_get_enemy_visual_layout(int p_idx) const {
	EnemyVisualLayout empty_layout;
	if (!controller.is_valid()) return empty_layout;
	Array ens = controller->get_enemy_units();
	if (p_idx < 0 || p_idx >= ens.size()) return empty_layout;
	Ref<ClockGearsBattleUnit> en = ens[p_idx];
	if (!en.is_valid()) return empty_layout;
	Rect2 vp_rect = get_battle_field_rect();
	Vector2 bpos = en->get_battle_pos();
	if (bpos.x <= 0.0f && bpos.y <= 0.0f) {
		bpos = Vector2(220.0f + p_idx * 160.0f, 180.0f);
	}
	Size2 src_sz(64.0f, 64.0f);
	for (int b = 0; b < battler_visuals.size(); b++) {
		const ClockGearsBattlerVisual &vis = battler_visuals[b];
		if (!vis.is_actor && vis.unit == en) {
			if (vis.atlas_texture.is_valid()) {
				Rect2 reg = vis.atlas_texture->get_region();
				if (reg.size.x > 0.0f && reg.size.y > 0.0f) {
					src_sz = reg.size;
				}
			} else if (vis.base_texture.is_valid()) {
				src_sz = vis.base_texture->get_size();
			}
			break;
		}
	}
	Ref<Font> font = enemies_field ? enemies_field->get_theme_default_font() : Ref<Font>();
	return compute_enemy_visual_layout(bpos, vp_rect, src_sz, en->get_sprite_scale(), font, en->get_hp_bar_mode(), en->get_hp_bar_pos());
}

Rect2 ClockGearsBattleScene::get_battle_field_rect() const {
	Rect2 vp_rect(Point2(0, 0), enemies_field ? enemies_field->get_size() : Size2(BATTLE_REF_WIDTH, BATTLE_REF_HEIGHT));
	if (vp_rect.size.x <= 1.0f || vp_rect.size.y <= 1.0f) {
		vp_rect.size = Size2(BATTLE_REF_WIDTH, BATTLE_REF_HEIGHT);
	}
	return vp_rect;
}

Rect2 ClockGearsBattleScene::get_enemy_sprite_rect(int p_idx) const {
	return _get_enemy_visual_layout(p_idx).draw_spr_rect;
}

Rect2 ClockGearsBattleScene::get_enemy_hud_group_rect(int p_idx) const {
	return _get_enemy_visual_layout(p_idx).hud_group_box;
}

Rect2 ClockGearsBattleScene::get_enemy_hp_bar_rect(int p_idx) const {
	EnemyVisualLayout layout = _get_enemy_visual_layout(p_idx);
	return layout.hp_bar_visible ? layout.hp_rect : Rect2();
}

Point2 ClockGearsBattleScene::get_enemy_effect_center(int p_idx) const {
	Rect2 spr_r = get_enemy_sprite_rect(p_idx);
	if (spr_r.size.x <= 0.0f && spr_r.size.y <= 0.0f) {
		return Point2();
	}
	return spr_r.get_center();
}

void ClockGearsBattleScene::_spawn_enemy_hit_fx(int p_enemy_idx, int p_damage, bool p_heal) {
	if (!enemies_field || !controller.is_valid()) return;
	Point2 center = get_enemy_effect_center(p_enemy_idx);
	if (center == Point2()) return;
	Point2 global_pos = enemies_field->get_position() + center;
	int dmg_val = p_heal ? -Math::abs(p_damage) : Math::abs(p_damage);
	// Agregar al CanvasLayer de batalla para que _refresh_ui no borre el FX flotante al recrear nodos de enemies_field.
	ClockGearsFXManager::spawn_impact_fx(this, global_pos, dmg_val, false, 0, false, false);
}

String ClockGearsBattleScene::get_enemy_label_text(int p_idx) const {
	if (!controller.is_valid()) return String();
	Array ens = controller->get_enemy_units();
	if (p_idx < 0 || p_idx >= ens.size()) return String();
	Ref<ClockGearsBattleUnit> en = ens[p_idx];
	if (!en.is_valid()) return String();
	return vformat("%s Nv %d", en->get_name(), MAX(1, en->get_level()));
}

bool ClockGearsBattleScene::is_enemy_hp_bar_visible(int p_idx) const {
	if (!controller.is_valid()) return false;
	Array ens = controller->get_enemy_units();
	if (p_idx < 0 || p_idx >= ens.size()) return false;
	Ref<ClockGearsBattleUnit> en = ens[p_idx];
	if (!en.is_valid()) return false;
	return ClockGearsBattleUnit::normalize_hp_bar_mode(en->get_hp_bar_mode()) != "oculta";
}

String ClockGearsBattleScene::get_enemy_current_visual_anim(int p_idx) const {
	int enemy_count = 0;
	for (int i = 0; i < battler_visuals.size(); i++) {
		if (!battler_visuals[i].is_actor) {
			if (enemy_count == p_idx) {
				return battler_visuals[i].current_anim;
			}
			enemy_count++;
		}
	}
	return String();
}

Dictionary ClockGearsBattleScene::default_battleback_cfg(bool p_is_floor, const Size2 &p_tex_size) {
	Dictionary d;
	int mode = BB_MODE_COVER;
	if (p_is_floor) {
		// Comportamiento clásico de suelo: Ajustar si es franja ancha (aspecto <= 16:9), Cubrir abajo si es alta.
		if (p_tex_size.x > 0.0f && p_tex_size.y > 0.0f && (p_tex_size.y / p_tex_size.x) <= (540.0f / 960.0f)) {
			mode = BB_MODE_FIT;
		} else {
			mode = BB_MODE_COVER;
		}
	}
	d["mode"] = mode;
	d["anchor"] = p_is_floor ? int(BB_ANCHOR_BOTTOM) : int(BB_ANCHOR_CENTER);
	d["offset_x"] = 0.0;
	d["offset_y"] = 0.0;
	d["scale"] = 1.0;
	d["integer_scale"] = false;
	return d;
}

Dictionary ClockGearsBattleScene::normalize_battleback_cfg(const Dictionary &p_cfg, bool p_is_floor, const Size2 &p_tex_size) {
	Dictionary def = default_battleback_cfg(p_is_floor, p_tex_size);
	if (!p_cfg.is_empty()) {
		if (p_cfg.has("mode")) def["mode"] = CLAMP(int(p_cfg["mode"]), int(BB_MODE_COVER), int(BB_MODE_TILE));
		if (p_cfg.has("anchor")) def["anchor"] = CLAMP(int(p_cfg["anchor"]), int(BB_ANCHOR_CENTER), int(BB_ANCHOR_BOTTOM));
		if (p_cfg.has("offset_x")) def["offset_x"] = double(p_cfg["offset_x"]);
		if (p_cfg.has("offset_y")) def["offset_y"] = double(p_cfg["offset_y"]);
		if (p_cfg.has("scale")) def["scale"] = CLAMP(double(p_cfg["scale"]), 0.05, 32.0);
		if (p_cfg.has("integer_scale")) def["integer_scale"] = bool(p_cfg["integer_scale"]);
	}
	if (bool(def["integer_scale"])) {
		def["scale"] = double(MAX(1.0, Math::round(double(def["scale"]))));
	}
	return def;
}

Rect2 ClockGearsBattleScene::compute_battleback_screen_rect(const Rect2 &p_vp_rect, const Size2 &p_tex_size, const Dictionary &p_cfg, bool p_is_floor) {
	if (p_vp_rect.size.x <= 0.0f || p_vp_rect.size.y <= 0.0f || p_tex_size.x <= 0.0f || p_tex_size.y <= 0.0f) {
		return Rect2();
	}
	Dictionary cfg = normalize_battleback_cfg(p_cfg, p_is_floor, p_tex_size);
	const float logical_w = BATTLE_REF_WIDTH;
	const float logical_h = BATTLE_REF_HEIGHT;
	float tw = p_tex_size.x;
	float th = p_tex_size.y;
	int mode = int(cfg["mode"]);
	int anchor = int(cfg["anchor"]);
	float scale = float(double(cfg["scale"]));
	float ox = float(double(cfg["offset_x"]));
	float oy = float(double(cfg["offset_y"]));
	bool int_scale = bool(cfg["integer_scale"]);

	Size2 draw_size;
	if (int_scale) {
		float f = MAX(1.0f, Math::round(scale));
		draw_size = Size2(tw * f, th * f);
	} else {
		switch (mode) {
			case BB_MODE_COVER: {
				float bs = MAX(logical_w / tw, logical_h / th);
				draw_size = Size2(tw * bs * scale, th * bs * scale);
			} break;
			case BB_MODE_FIT: {
				float bs = MIN(logical_w / tw, logical_h / th);
				draw_size = Size2(tw * bs * scale, th * bs * scale);
			} break;
			case BB_MODE_STRETCH: {
				draw_size = Size2(logical_w * scale, logical_h * scale);
			} break;
			case BB_MODE_TILE: {
				draw_size = Size2(tw * scale, th * scale);
			} break;
			default: {
				float bs = MAX(logical_w / tw, logical_h / th);
				draw_size = Size2(tw * bs * scale, th * bs * scale);
			} break;
		}
	}

	Point2 base_pos;
	if (anchor == BB_ANCHOR_BOTTOM) {
		base_pos = Point2((logical_w - draw_size.x) * 0.5f, logical_h - draw_size.y);
	} else {
		base_pos = Point2((logical_w - draw_size.x) * 0.5f, (logical_h - draw_size.y) * 0.5f);
	}
	Point2 logical_pos = base_pos + Point2(ox, oy);

	float sx = p_vp_rect.size.x / logical_w;
	float sy = p_vp_rect.size.y / logical_h;
	return Rect2(
			p_vp_rect.position.x + logical_pos.x * sx,
			p_vp_rect.position.y + logical_pos.y * sy,
			draw_size.x * sx,
			draw_size.y * sy);
}

void ClockGearsBattleScene::draw_battleback_layer(Control *p_layer_canvas, const Rect2 &p_vp_rect, const Ref<Texture2D> &p_tex, const Dictionary &p_cfg, bool p_is_floor) {
	if (!p_layer_canvas || !is_texture_ready(p_tex)) {
		return;
	}
	if (p_vp_rect.size.x <= 1.0f || p_vp_rect.size.y <= 1.0f) {
		return;
	}
	Size2 tsz = p_tex->get_size();
	Dictionary cfg = normalize_battleback_cfg(p_cfg, p_is_floor, tsz);
	bool int_scale = bool(cfg["integer_scale"]);
	CanvasItem::TextureFilter want_filter = int_scale ? CanvasItem::TEXTURE_FILTER_NEAREST : CanvasItem::TEXTURE_FILTER_LINEAR;
	if (p_layer_canvas->get_texture_filter() != want_filter) {
		p_layer_canvas->set_texture_filter(want_filter);
	}

	int mode = int(cfg["mode"]);
	Rect2 screen_rect = compute_battleback_screen_rect(p_vp_rect, tsz, cfg, p_is_floor);
	if (screen_rect.size.x <= 0.5f || screen_rect.size.y <= 0.5f) {
		return;
	}

	auto draw_clipped = [&](const Rect2 &r_dest) {
		Rect2 clipped = r_dest.intersection(p_vp_rect);
		if (clipped.size.x <= 0.1f || clipped.size.y <= 0.1f) {
			return;
		}
		float u0 = (clipped.position.x - r_dest.position.x) / r_dest.size.x;
		float v0 = (clipped.position.y - r_dest.position.y) / r_dest.size.y;
		float u1 = (clipped.position.x + clipped.size.x - r_dest.position.x) / r_dest.size.x;
		float v1 = (clipped.position.y + clipped.size.y - r_dest.position.y) / r_dest.size.y;
		Rect2 src_rect(u0 * tsz.x, v0 * tsz.y, (u1 - u0) * tsz.x, (v1 - v0) * tsz.y);
		p_layer_canvas->draw_texture_rect_region(p_tex, clipped, src_rect);
	};

	if (mode == BB_MODE_TILE) {
		float tw_px = MAX(8.0f, screen_rect.size.x);
		float th_px = MAX(8.0f, screen_rect.size.y);
		float start_x = screen_rect.position.x;
		while (start_x > p_vp_rect.position.x) start_x -= tw_px;
		while (start_x + tw_px <= p_vp_rect.position.x) start_x += tw_px;
		float start_y = screen_rect.position.y;
		while (start_y > p_vp_rect.position.y) start_y -= th_px;
		while (start_y + th_px <= p_vp_rect.position.y) start_y += th_px;

		float end_x = p_vp_rect.position.x + p_vp_rect.size.x;
		float end_y = p_vp_rect.position.y + p_vp_rect.size.y;
		int guard = 0;
		for (float y = start_y; y < end_y && guard < 400; y += th_px) {
			for (float x = start_x; x < end_x && guard < 400; x += tw_px) {
				draw_clipped(Rect2(x, y, tw_px, th_px));
				guard++;
			}
		}
	} else {
		draw_clipped(screen_rect);
	}
}

String ClockGearsBattleScene::format_enemy_name_level(const Ref<Font> &p_font, int p_font_sz, const String &p_name, int p_level, float p_max_w) {
	String lvl_suffix = vformat(" Nv %d", MAX(1, p_level));
	String clean = p_name;
	String full = clean + lvl_suffix;
	if (!p_font.is_valid() || p_max_w <= 0.0f) {
		return full;
	}
	if (p_font->get_string_size(full, HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_sz).x <= p_max_w) {
		return full;
	}
	String ell = String::utf8("…");
	while (clean.length() > 1) {
		clean = clean.substr(0, clean.length() - 1);
		String cand = clean + ell + lvl_suffix;
		if (p_font->get_string_size(cand, HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_sz).x <= p_max_w) {
			return cand;
		}
	}
	return ell + lvl_suffix;
}

Point2 ClockGearsBattleScene::troop_pos_to_screen(const Vector2 &p_troop_pos, const Rect2 &p_vp_rect) {
	if (p_vp_rect.size.x <= 0.0f || p_vp_rect.size.y <= 0.0f) {
		return p_vp_rect.position;
	}
	return p_vp_rect.position + Point2(
			(p_troop_pos.x / BATTLE_REF_WIDTH) * p_vp_rect.size.x,
			(p_troop_pos.y / BATTLE_REF_HEIGHT) * p_vp_rect.size.y);
}

Vector2 ClockGearsBattleScene::screen_to_troop_pos(const Point2 &p_screen_pos, const Rect2 &p_vp_rect) {
	if (p_vp_rect.size.x <= 0.0f || p_vp_rect.size.y <= 0.0f) {
		return Vector2();
	}
	return Vector2(
			((p_screen_pos.x - p_vp_rect.position.x) / p_vp_rect.size.x) * BATTLE_REF_WIDTH,
			((p_screen_pos.y - p_vp_rect.position.y) / p_vp_rect.size.y) * BATTLE_REF_HEIGHT);
}

Rect2 ClockGearsBattleScene::get_battle_hud_strip_rect(const Rect2 &p_vp_rect) {
	if (p_vp_rect.size.x <= 0.0f || p_vp_rect.size.y <= 0.0f) {
		return Rect2();
	}
	float hud_h = p_vp_rect.size.y * (BATTLE_HUD_BOTTOM_PX / BATTLE_REF_HEIGHT);
	hud_h = CLAMP(hud_h, 0.0f, p_vp_rect.size.y);
	return Rect2(p_vp_rect.position.x, p_vp_rect.position.y + p_vp_rect.size.y - hud_h, p_vp_rect.size.x, hud_h);
}

ClockGearsBattleScene::EnemyVisualLayout ClockGearsBattleScene::compute_enemy_visual_layout(const Vector2 &p_troop_pos, const Rect2 &p_vp_rect, const Size2 &p_src_size, int p_member_scale, const Ref<Font> &p_font, const String &p_hp_bar_mode, const Vector2i &p_hp_bar_pos) {
	EnemyVisualLayout layout;
	float sx = (p_vp_rect.size.x > 0.0f) ? (p_vp_rect.size.x / BATTLE_REF_WIDTH) : 1.0f;
	float sy = (p_vp_rect.size.y > 0.0f) ? (p_vp_rect.size.y / BATTLE_REF_HEIGHT) : 1.0f;
	float scale = MIN(sx, sy);
	int m_scale = CLAMP(p_member_scale, 1, 6);

	layout.block_origin = troop_pos_to_screen(p_troop_pos, p_vp_rect);
	float block_w = 140.0f * scale;
	// Base fija en el centro de los pies para que al escalar crezca hacia arriba y a los lados sin saltar de posición.
	Point2 feet_scr = layout.block_origin + Point2(70.0f * scale, 110.0f * scale);

	float src_w = (p_src_size.x > 0.0f) ? p_src_size.x : 64.0f;
	float src_h = (p_src_size.y > 0.0f) ? p_src_size.y : 64.0f;
	float draw_w = src_w * float(m_scale) * scale;
	float draw_h = src_h * float(m_scale) * scale;

	layout.draw_spr_rect = Rect2(feet_scr.x - draw_w * 0.5f, feet_scr.y - draw_h, draw_w, draw_h);
	layout.sprite_box = layout.draw_spr_rect;

	layout.hp_bar_mode = ClockGearsBattleUnit::normalize_hp_bar_mode(p_hp_bar_mode);
	layout.hp_bar_visible = (layout.hp_bar_mode != "oculta");

	layout.font_sz = MAX(9, int(13.0f * scale));
	float hp_w = (layout.hp_bar_mode == "libre")
			? (ClockGearsBattleUnit::FREE_HP_BAR_WIDTH * scale)
			: CLAMP(MAX(ClockGearsBattleUnit::FREE_HP_BAR_WIDTH * scale, draw_w), 72.0f * scale, 240.0f * scale);
	layout.label_max_w = (layout.hp_bar_mode == "libre")
			? MAX(block_w, hp_w + 24.0f * scale)
			: MAX(block_w, draw_w + 24.0f * scale);
	float hp_h = MAX(6.0f, 10.0f * scale);

	float lbl_h = float(layout.font_sz) + 3.0f * scale;
	float group_h = lbl_h + 5.0f * scale + hp_h;
	float group_top_y = feet_scr.y;
	layout.group_center_x = feet_scr.x;

	if (layout.hp_bar_mode == "arriba") {
		layout.group_center_x = layout.draw_spr_rect.get_center().x;
		group_top_y = layout.draw_spr_rect.position.y - 4.0f * scale - group_h;
	} else if (layout.hp_bar_mode == "libre") {
		Vector2i clamped_ref = ClockGearsBattleUnit::clamp_free_hp_bar_pos(p_hp_bar_pos, BATTLE_REF_WIDTH, BATTLE_REF_HEIGHT);
		Point2 free_anchor_scr = troop_pos_to_screen(Vector2(float(clamped_ref.x), float(clamped_ref.y)), p_vp_rect);
		float half_w = hp_w * 0.5f;
		float min_cx = p_vp_rect.position.x + half_w;
		float max_cx = MAX(min_cx, p_vp_rect.position.x + p_vp_rect.size.x - half_w);
		float min_ty = p_vp_rect.position.y;
		float max_ty = MAX(min_ty, p_vp_rect.position.y + p_vp_rect.size.y - group_h);
		layout.group_center_x = CLAMP(free_anchor_scr.x, min_cx, max_cx);
		group_top_y = CLAMP(free_anchor_scr.y, min_ty, max_ty);
	}

	layout.label_baseline_y = group_top_y + lbl_h;
	layout.hp_rect = Rect2(layout.group_center_x - hp_w * 0.5f, layout.label_baseline_y + 5.0f * scale, hp_w, hp_h);

	float vis_top_y = group_top_y + 3.0f * scale;
	float vis_bottom_y = layout.hp_bar_visible ? (layout.hp_rect.position.y + layout.hp_rect.size.y) : (layout.label_baseline_y + 3.0f * scale);
	layout.hud_group_box = Rect2(layout.group_center_x - hp_w * 0.5f, vis_top_y, hp_w, MAX(8.0f, vis_bottom_y - vis_top_y));

	Rect2 union_r = layout.draw_spr_rect.merge(layout.hud_group_box);
	union_r = union_r.merge(Rect2(layout.block_origin, Size2(block_w, 110.0f * scale)));
	layout.full_box = union_r;
	return layout;
}

void ClockGearsBattleScene::draw_troop_enemies(Control *p_canvas, const Rect2 &p_vp_rect, const Vector<TroopEnemyDrawEntry> &p_entries, int p_selected_idx, bool p_show_selection, bool p_show_grid, const Ref<Font> &p_font) {
	if (!p_canvas || p_vp_rect.size.x <= 1.0f || p_vp_rect.size.y <= 1.0f) {
		return;
	}
	float sx = p_vp_rect.size.x / BATTLE_REF_WIDTH;
	float sy = p_vp_rect.size.y / BATTLE_REF_HEIGHT;

	if (p_show_grid) {
		float step_x = 32.0f * sx;
		float step_y = 32.0f * sy;
		if (step_x >= 4.0f && step_y >= 4.0f) {
			for (float gx = p_vp_rect.position.x; gx <= p_vp_rect.position.x + p_vp_rect.size.x; gx += step_x) {
				p_canvas->draw_line(Point2(gx, p_vp_rect.position.y), Point2(gx, p_vp_rect.position.y + p_vp_rect.size.y), Color(1, 1, 1, 0.10f), 1.0f);
			}
			for (float gy = p_vp_rect.position.y; gy <= p_vp_rect.position.y + p_vp_rect.size.y; gy += step_y) {
				p_canvas->draw_line(Point2(p_vp_rect.position.x, gy), Point2(p_vp_rect.position.x + p_vp_rect.size.x, gy), Color(1, 1, 1, 0.10f), 1.0f);
			}
		}
	}

	for (int i = 0; i < p_entries.size(); i++) {
		const TroopEnemyDrawEntry &e = p_entries[i];
		Size2 src_sz = (e.src_region.size.x > 0.0f && e.src_region.size.y > 0.0f)
				? e.src_region.size
				: (e.texture.is_valid() ? e.texture->get_size() : Size2(64.0f, 64.0f));
		EnemyVisualLayout layout = compute_enemy_visual_layout(e.troop_pos, p_vp_rect, src_sz, e.sprite_scale, p_font, e.hp_bar_mode, e.hp_bar_pos);

		if (is_texture_ready(e.texture)) {
			Rect2 vis_r = layout.draw_spr_rect.intersection(p_vp_rect);
			if (vis_r.size.x > 0.0f && vis_r.size.y > 0.0f) {
				Rect2 full_src = (e.src_region.size.x > 0.0f && e.src_region.size.y > 0.0f)
						? e.src_region
						: Rect2(Point2(0, 0), e.texture->get_size());
				if (vis_r == layout.draw_spr_rect) {
					p_canvas->draw_texture_rect_region(e.texture, layout.draw_spr_rect, full_src, e.modulate);
				} else {
					float u0 = (vis_r.position.x - layout.draw_spr_rect.position.x) / layout.draw_spr_rect.size.x;
					float v0 = (vis_r.position.y - layout.draw_spr_rect.position.y) / layout.draw_spr_rect.size.y;
					float u1 = (vis_r.position.x + vis_r.size.x - layout.draw_spr_rect.position.x) / layout.draw_spr_rect.size.x;
					float v1 = (vis_r.position.y + vis_r.size.y - layout.draw_spr_rect.position.y) / layout.draw_spr_rect.size.y;
					Rect2 clip_src(
							full_src.position.x + full_src.size.x * u0,
							full_src.position.y + full_src.size.y * v0,
							full_src.size.x * (u1 - u0),
							full_src.size.y * (v1 - v0));
					p_canvas->draw_texture_rect_region(e.texture, vis_r, clip_src, e.modulate);
				}
			}
		}

		if (p_show_selection && i == p_selected_idx) {
			Rect2 sel_rect = layout.draw_spr_rect.grow(3.0f).intersection(p_vp_rect);
			if (sel_rect.size.x > 0.0f && sel_rect.size.y > 0.0f) {
				p_canvas->draw_rect(sel_rect, Color(1.0f, 0.85f, 0.25f, 0.95f), false, 2.0f);
			}
		}

		String title_str = format_enemy_name_level(p_font, layout.font_sz, e.name, e.level, layout.label_max_w);
		if (p_font.is_valid()) {
			float str_w = p_font->get_string_size(title_str, HORIZONTAL_ALIGNMENT_LEFT, -1, layout.font_sz).x;
			float center_x = layout.group_center_x;
			Point2 txt_pos(center_x - str_w * 0.5f, layout.label_baseline_y);
			Color text_col = e.is_dead ? Color(0.65f, 0.65f, 0.65f, 0.85f) : Color(1.0f, 0.94f, 0.86f, 1.0f);
			p_canvas->draw_string(p_font, txt_pos + Point2(1, 1), title_str, HORIZONTAL_ALIGNMENT_LEFT, -1, layout.font_sz, Color(0, 0, 0, 0.88f));
			p_canvas->draw_string(p_font, txt_pos, title_str, HORIZONTAL_ALIGNMENT_LEFT, -1, layout.font_sz, text_col);
		}

		if (layout.hp_bar_visible) {
			float hp_ratio = CLAMP(float(e.hp) / float(MAX(1, e.max_hp)), 0.0f, 1.0f);
			p_canvas->draw_rect(layout.hp_rect, Color(0.12f, 0.06f, 0.06f, 0.92f), true);
			if (hp_ratio > 0.0f) {
				Rect2 fill_r(layout.hp_rect.position, Size2(layout.hp_rect.size.x * hp_ratio, layout.hp_rect.size.y));
				p_canvas->draw_rect(fill_r, Color(0.25f, 0.85f, 0.35f, 0.95f), true);
			}
			p_canvas->draw_rect(layout.hp_rect, Color(0.35f, 0.42f, 0.50f, 0.85f), false, 1.0f);
		}

		if (e.highlight_hud_group) {
			Rect2 hl_r = layout.hud_group_box.grow(3.0f).intersection(p_vp_rect);
			if (hl_r.size.x > 0.0f && hl_r.size.y > 0.0f) {
				p_canvas->draw_rect(hl_r, Color(0.35f, 0.88f, 1.0f, 0.16f), true);
				p_canvas->draw_rect(hl_r, Color(0.35f, 0.92f, 1.0f, 0.95f), false, 2.0f);
			}
		}
	}
}

void ClockGearsBattleScene::_sync_enemy_hit_nodes_layout() {
	if (!enemies_field || !controller.is_valid()) return;
	Rect2 vp_rect(Point2(0, 0), enemies_field->get_size());
	if (vp_rect.size.x <= 1.0f || vp_rect.size.y <= 1.0f) return;

	Array ens = controller->get_enemy_units();
	Ref<Font> font = enemies_field->get_theme_default_font();
	for (int i = 0; i < ens.size() && i < enemy_nodes.size(); i++) {
		Ref<ClockGearsBattleUnit> en = ens[i];
		Control *node = enemy_nodes[i];
		if (!en.is_valid() || !node) continue;
		Vector2 bpos = en->get_battle_pos();
		if (bpos.x <= 0.0f && bpos.y <= 0.0f) {
			bpos = Vector2(220.0f + i * 160.0f, 180.0f);
		}
		Size2 src_sz(64.0f, 64.0f);
		for (int b = 0; b < battler_visuals.size(); b++) {
			if (!battler_visuals[b].is_actor && battler_visuals[b].unit == en && battler_visuals[b].atlas_texture.is_valid()) {
				Rect2 reg = battler_visuals[b].atlas_texture->get_region();
				if (reg.size.x > 0.0f && reg.size.y > 0.0f) {
					src_sz = reg.size;
				}
				break;
			}
		}
		EnemyVisualLayout layout = compute_enemy_visual_layout(bpos, vp_rect, src_sz, en->get_sprite_scale(), font, en->get_hp_bar_mode(), en->get_hp_bar_pos());
		node->set_position(layout.block_origin);
		node->set_size(layout.full_box.size);
		if (node->get_child_count() > 0) {
			TextureRect *eff_rect = Object::cast_to<TextureRect>(node->get_child(0));
			if (eff_rect) {
				float vp_s = (vp_rect.size.x > 0.0f && vp_rect.size.y > 0.0f) ? MIN(vp_rect.size.x / BATTLE_REF_WIDTH, vp_rect.size.y / BATTLE_REF_HEIGHT) : 1.0f;
				Size2 eff_sz(MAX(48.0f * vp_s, layout.draw_spr_rect.size.x), MAX(48.0f * vp_s, layout.draw_spr_rect.size.y));
				eff_rect->set_position(layout.draw_spr_rect.get_center() - eff_sz * 0.5f - layout.block_origin);
				eff_rect->set_size(eff_sz);
			}
		}
	}
	enemies_field->queue_redraw();
}

void ClockGearsBattleScene::_on_enemies_field_draw() {
	if (!enemies_field || !controller.is_valid()) return;
	Rect2 vp_rect(Point2(0, 0), enemies_field->get_size());
	if (vp_rect.size.x <= 1.0f || vp_rect.size.y <= 1.0f) return;

	Array ens = controller->get_enemy_units();
	Vector<TroopEnemyDrawEntry> entries;
	entries.resize(ens.size());
	for (int i = 0; i < ens.size(); i++) {
		Ref<ClockGearsBattleUnit> en = ens[i];
		if (!en.is_valid()) continue;
		TroopEnemyDrawEntry &entry = entries.write[i];
		entry.name = en->get_name();
		entry.level = MAX(1, en->get_level());
		entry.hp = en->get_hp();
		entry.max_hp = MAX(1, en->get_max_hp());
		entry.is_dead = en->get_is_dead();
		entry.sprite_scale = en->get_sprite_scale();
		entry.hp_bar_mode = en->get_hp_bar_mode();
		entry.hp_bar_pos = en->get_hp_bar_pos();
		Vector2 bpos = en->get_battle_pos();
		if (bpos.x <= 0.0f && bpos.y <= 0.0f) {
			bpos = Vector2(220.0f + i * 160.0f, 180.0f);
		}
		entry.troop_pos = bpos;
		entry.modulate = entry.is_dead ? Color(0.5f, 0.5f, 0.5f, 0.45f) : Color(1, 1, 1, 1);

		for (int b = 0; b < battler_visuals.size(); b++) {
			const ClockGearsBattlerVisual &vis = battler_visuals[b];
			if (!vis.is_actor && vis.unit == en) {
				entry.texture = vis.base_texture;
				if (vis.atlas_texture.is_valid()) {
					entry.src_region = vis.atlas_texture->get_region();
				}
				break;
			}
		}
	}
	Ref<Font> font = enemies_field->get_theme_default_font();
	draw_troop_enemies(enemies_field, vp_rect, entries, -1, false, false, font);
}

void ClockGearsBattleScene::_on_battleback_bg_layer_draw() {
	if (!battleback_bg_layer || !controller.is_valid() || !is_texture_ready(battleback_bg_tex)) return;
	Rect2 vp_rect(Point2(0, 0), battleback_bg_layer->get_size());
	draw_battleback_layer(battleback_bg_layer, vp_rect, battleback_bg_tex, controller->get_troop_bg_cfg(), false);
}

void ClockGearsBattleScene::_on_battleback_floor_layer_draw() {
	if (!battleback_floor_layer || !controller.is_valid() || !is_texture_ready(battleback_floor_tex)) return;
	Rect2 vp_rect(Point2(0, 0), battleback_floor_layer->get_size());
	draw_battleback_layer(battleback_floor_layer, vp_rect, battleback_floor_tex, controller->get_troop_floor_cfg(), true);
}

Rect2 ClockGearsBattleScene::get_drawn_battleback_rect(int p_slot) const {
	Control *ctrl = (p_slot == 1) ? battleback_bg_layer : battleback_floor_layer;
	Ref<Texture2D> tex = (p_slot == 1) ? battleback_bg_tex : battleback_floor_tex;
	if (!ctrl || !controller.is_valid() || !is_texture_ready(tex)) return Rect2();
	Rect2 vp_rect(Point2(0, 0), ctrl->get_size());
	Dictionary cfg = (p_slot == 1) ? controller->get_troop_bg_cfg() : controller->get_troop_floor_cfg();
	return compute_battleback_screen_rect(vp_rect, tex->get_size(), cfg, p_slot != 1);
}

ClockGearsBattleScene::ClockGearsBattleScene() {
	controller.instantiate();
}

ClockGearsBattleScene::~ClockGearsBattleScene() {
}
