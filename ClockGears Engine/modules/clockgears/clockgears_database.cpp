#include "clockgears_database.h"
#include "clockgears_battle_window.h"
#include "clockgears_damage_engine.h"
#include "clockgears_event_editor_dialog.h"
#include "clockgears_map.h"
#include "clockgears_workspace.h"
#include "core/clockgears_formulas.h"
#include "core/clockgears_pixel_art.h"
#include "core/clockgears_system_catalog.h"
#include "core/config/project_settings.h"
#include "editor/clockgears_project_manager.h"
#include "editor/clockgears_shape_editor.h"
#include "editor/clockgears_ui_design_editor.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "editor/clockgears_theme.h"
#include "editor/editor_node.h"
#ifdef TOOLS_ENABLED
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"
#endif
#include "editor/file_system/editor_paths.h"
#include "editor/themes/editor_scale.h"
#include "icons/clockgears_icons.h"
#include "runtime/clockgears_credits.h"
#include "runtime/clockgears_game_over.h"
#include "runtime/clockgears_game_menu.h"
#include "runtime/clockgears_hud.h"
#include "runtime/clockgears_title_screen.h"
#include "runtime/clockgears_window_style.h"
#include "scene/gui/aspect_ratio_container.h"
#include "scene/gui/center_container.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/separator.h"
#include "scene/gui/subviewport_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"

#define CG_DB_TR(m_str) String::utf8(m_str)

static int s_get_troop_member_scale(const Dictionary &p_member) {
	int s = 1;
	if (p_member.has("escala")) {
		s = int(p_member["escala"]);
	} else if (p_member.has("scale")) {
		s = int(p_member["scale"]);
	}
	return CLAMP(s, 1, 6);
}

static String s_get_troop_member_hp_bar_mode(const Dictionary &p_member) {
	return ClockGearsBattleUnit::normalize_hp_bar_mode(String(p_member.get("hp_bar_mode", "abajo")));
}

static Vector2i s_get_troop_member_hp_bar_pos(const Dictionary &p_member) {
	if (p_member.has("hp_bar_x") && p_member.has("hp_bar_y")) {
		return ClockGearsBattleUnit::clamp_free_hp_bar_pos(Vector2i(int(p_member["hp_bar_x"]), int(p_member["hp_bar_y"])));
	}
	return ClockGearsBattleUnit::clamp_free_hp_bar_pos(Vector2i(int(p_member.get("x", 200)), int(p_member.get("y", 260))));
}

static int s_hp_bar_mode_to_menu_id(const String &p_mode) {
	if (p_mode == "arriba") return 1;
	if (p_mode == "libre") return 3;
	if (p_mode == "oculta") return 4;
	return 2; // abajo
}

static Size2 s_resolve_troop_member_src_size(const Dictionary &p_member, const Array &p_enemies) {
	int eid = int(p_member.get("enemy_id", 1));
	Dictionary ed;
	for (int k = 0; k < p_enemies.size(); k++) {
		Dictionary cand = p_enemies[k];
		if (int(cand.get("id", k + 1)) == eid) {
			ed = cand;
			break;
		}
	}
	Ref<Texture2D> tex;
	Rect2 src_reg;
	String src_kind;
	ClockGearsBattleScene::resolve_enemy_preview_sprite(ed, tex, src_reg, src_kind);
	if (src_reg.size.x > 0.0f && src_reg.size.y > 0.0f) {
		return src_reg.size;
	}
	if (tex.is_valid()) {
		return tex->get_size();
	}
	return Size2(64.0f, 64.0f);
}

static Vector2i s_snap_troop_member_pos_to_grid(float p_fx, float p_fy, const Size2 &p_src_sz, int p_scale) {
	int m_scale = CLAMP(p_scale, 1, 6);
	double draw_w = double(p_src_sz.x > 0.0f ? p_src_sz.x : 64.0f) * double(m_scale);
	double draw_h = double(p_src_sz.y > 0.0f ? p_src_sz.y : 64.0f) * double(m_scale);
	double spr_left = double(p_fx) + 70.0 - draw_w * 0.5;
	double spr_top = double(p_fy) + 110.0 - draw_h;
	double snapped_left = Math::round(spr_left / 32.0) * 32.0;
	double snapped_top = Math::round(spr_top / 32.0) * 32.0;
	int nx = int(Math::round(snapped_left + draw_w * 0.5 - 70.0));
	int ny = int(Math::round(snapped_top + draw_h - 110.0));
	return Vector2i(nx, ny);
}

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

static bool s_parse_tile_coords_variant(const Variant &p_var, Vector2i &r_coords) {
	switch (p_var.get_type()) {
		case Variant::VECTOR2I: {
			Vector2i v = p_var;
			r_coords = v;
			return (r_coords.x >= 0 && r_coords.y >= 0);
		}
		case Variant::VECTOR2: {
			Vector2 v = p_var;
			r_coords = Vector2i(int(Math::round(v.x)), int(Math::round(v.y)));
			return (r_coords.x >= 0 && r_coords.y >= 0);
		}
		case Variant::ARRAY: {
			Array a = p_var;
			if (a.size() >= 2) {
				r_coords = Vector2i(int(a[0]), int(a[1]));
				return (r_coords.x >= 0 && r_coords.y >= 0);
			}
			return false;
		}
		case Variant::DICTIONARY: {
			Dictionary d = p_var;
			if (d.has("x") && d.has("y")) {
				r_coords = Vector2i(int(d["x"]), int(d["y"]));
				return (r_coords.x >= 0 && r_coords.y >= 0);
			}
			return false;
		}
		case Variant::STRING: {
			String s = String(p_var).strip_edges();
			s = s.replace("(", "").replace(")", "").replace("[", "").replace("]", "").strip_edges();
			PackedStringArray parts = s.split(",", false);
			if (parts.size() >= 2) {
				String p0 = parts[0].strip_edges();
				String p1 = parts[1].strip_edges();
				if (p0.is_valid_int() && p1.is_valid_int()) {
					r_coords = Vector2i(p0.to_int(), p1.to_int());
					return (r_coords.x >= 0 && r_coords.y >= 0);
				}
			}
			return false;
		}
		default:
			return false;
	}
}

static Vector2i s_parse_tile_block_variant(const Variant &p_var) {
	Vector2i blk(1, 1);
	if (s_parse_tile_coords_variant(p_var, blk)) {
		return Vector2i(MAX(1, blk.x), MAX(1, blk.y));
	}
	return Vector2i(1, 1);
}

static Array s_tile_vec_to_json_array(const Vector2i &p_v) {
	Array a;
	a.push_back(p_v.x);
	a.push_back(p_v.y);
	return a;
}

static bool s_is_arpg_category(int p_category) {
	switch (p_category) {
		case CAT_ARPG_ENEMIES:
		case CAT_ARPG_NPCS:
		case CAT_ARPG_WEAPONS:
		case CAT_ARPG_ARMORS:
		case CAT_ARPG_SHIELDS:
		case CAT_ARPG_ACCESSORIES:
			return true;
		default:
			return false;
	}
}

static bool s_is_anim_slot_category(int p_category) {
	return p_category == CAT_ACTORS || p_category == CAT_NPCS || s_is_arpg_category(p_category);
}

static bool s_is_arpg_equip_category(int p_category) {
	return p_category >= CAT_ARPG_WEAPONS && p_category <= CAT_ARPG_ACCESSORIES;
}

static bool s_is_arpg_shape_category(int p_category) {
	return p_category == CAT_ACTORS || s_is_arpg_category(p_category);
}

static bool s_is_category_visible_in_mode(int p_category, int p_combat_mode) {
	if (p_category == ClockGearsDatabaseDialog::DB_CAT_MAIN_MENU ||
			p_category == ClockGearsDatabaseDialog::DB_CAT_WINDOW_STYLES ||
			p_category == ClockGearsDatabaseDialog::DB_CAT_GAME_MENU ||
			p_category == ClockGearsDatabaseDialog::DB_CAT_CREDITS ||
			p_category == ClockGearsDatabaseDialog::DB_CAT_HUD ||
			p_category == ClockGearsDatabaseDialog::DB_CAT_UI_DESIGNER ||
			p_category == ClockGearsDatabaseDialog::DB_CAT_GAME_OVER) {
		return true;
	}
	if (p_combat_mode == COMBAT_MODE_ACTION_RPG) {
		if (p_category == CAT_WEAPONS || p_category == CAT_ARMORS || p_category == CAT_ACCESSORIES) {
			return false;
		}
	} else {
		if (s_is_arpg_equip_category(p_category)) {
			return false;
		}
	}
	return true;
}

static bool s_has_event_tab_category(int p_category) {
	return p_category == CAT_NPCS || p_category == CAT_ARPG_NPCS || p_category == CAT_ARPG_ENEMIES;
}

static bool s_has_body_zones_tab_category(int p_category) {
	return p_category == CAT_ACTORS || p_category == CAT_NPCS || p_category == CAT_ARPG_NPCS || p_category == CAT_ARPG_ENEMIES;
}

static bool s_has_movement_tab_category(int p_category) {
	return p_category == CAT_NPCS || p_category == CAT_ARPG_NPCS || p_category == CAT_ARPG_ENEMIES;
}

static bool s_category_has_record_graphic(int p_category) {
	if (s_is_anim_slot_category(p_category)) {
		return true;
	}
	switch (p_category) {
		case CAT_CLASSES:
		case CAT_SKILLS:
		case CAT_ITEMS:
		case CAT_WEAPONS:
		case CAT_ARMORS:
		case CAT_ACCESSORIES:
		case CAT_ENEMIES:
		case CAT_STATES:
		case CAT_FLORA:
		case CAT_FAUNA:
		case CAT_ANIMATIONS:
			return true;
		default:
			return false;
	}
}

static String s_middle_ellipsize_for_2lines(const String &p_full, const Ref<Font> &p_font, int p_font_size, float p_max_w) {
	if (p_full.is_empty() || p_font.is_null() || p_max_w <= 16.0f) {
		return p_full;
	}
	const float eff_w = MAX(16.0f, p_max_w - 4.0f);
	const float line_h = MAX(1.0f, p_font->get_height(p_font_size));
	const float max_2line_h = line_h * 2.15f;
	const BitField<TextServer::LineBreakFlag> brk = TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_GRAPHEME_BOUND;
	Size2 full_sz = p_font->get_multiline_string_size(p_full, HORIZONTAL_ALIGNMENT_CENTER, eff_w, p_font_size, -1, brk);
	if (full_sz.y <= max_2line_h && full_sz.x <= eff_w + 0.5f) {
		return p_full;
	}
	const String ell = String::utf8("…");
	const int n = p_full.length();
	if (n <= 4) {
		return p_full;
	}
	int lo = 2;
	int hi = n - 1;
	String best = ell;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		int left_len = MAX(1, (mid * 55) / 100);
		int right_len = MAX(1, mid - left_len);
		if (left_len + right_len >= n) {
			right_len = MAX(1, n - 1 - left_len);
		}
		String cand = p_full.substr(0, left_len) + ell + p_full.substr(n - right_len, right_len);
		Size2 sz = p_font->get_multiline_string_size(cand, HORIZONTAL_ALIGNMENT_CENTER, eff_w, p_font_size, -1, brk);
		if (sz.y <= max_2line_h && sz.x <= eff_w + 0.5f) {
			best = cand;
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	return best;
}

// ============================================================================
// ClockGearsTilesetCollisionEditor
// ============================================================================

void ClockGearsTilesetCollisionEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Size2 sz = get_size();
		draw_rect(Rect2(Point2(), sz), Color(0.10f, 0.11f, 0.13f, 1.0f), true);

		// Checkerboard background
		const int chk = 16;
		int cols_chk = Math::ceil(sz.x / chk);
		int rows_chk = Math::ceil(sz.y / chk);
		for (int ry = 0; ry < rows_chk; ry++) {
			for (int rx = 0; rx < cols_chk; rx++) {
				if ((rx + ry) % 2 == 1) {
					draw_rect(Rect2(rx * chk, ry * chk, chk, chk), Color(0.14f, 0.15f, 0.18f, 1.0f), true);
				}
			}
		}

		if (tileset_texture.is_null()) {
			Ref<Font> font = get_theme_default_font();
			int font_size = get_theme_default_font_size();
			if (font.is_valid()) {
				float max_w = MAX(120.0f, sz.x - 24.0f);
				draw_multiline_string(font, Point2(12, 32), CG_DB_TR("Sin hoja cargada. Haz clic en 'Cargar Hoja de Tileset...'"), HORIZONTAL_ALIGNMENT_LEFT, max_w, font_size, -1, Color(0.65f, 0.68f, 0.75f));
				draw_multiline_string(font, Point2(12, 82), CG_DB_TR("Luego haz clic sobre cada tile para alternar:\nO (Pasa) / X (Bloqueado) / ★ (Pasa por detrás)"), HORIZONTAL_ALIGNMENT_LEFT, max_w, font_size, -1, Color(0.55f, 0.80f, 0.95f));
			}
			return;
		}

		Size2 tex_size = tileset_texture->get_size();
		draw_texture(tileset_texture, Point2());

		int cols = MAX(1, (int)(tex_size.x / tile_size));
		int rows = MAX(1, (int)(tex_size.y / tile_size));
		Ref<Font> font = get_theme_default_font();

		for (int ty = 0; ty < rows; ty++) {
			for (int tx = 0; tx < cols; tx++) {
				Rect2 cell_rect(tx * tile_size, ty * tile_size, tile_size, tile_size);
				draw_rect(cell_rect, Color(0.0f, 0.0f, 0.0f, 0.45f), false, 1.0f);

				String key = vformat("%d,%d", tx, ty);
				int mode = collision_map.has(key) ? (int)collision_map[key] : 0;

				String symbol = "O";
				Color sym_color = Color(0.35f, 0.95f, 0.45f, 0.95f);
				if (mode == 1) {
					symbol = "X";
					sym_color = Color(1.0f, 0.30f, 0.30f, 0.95f);
				} else if (mode == 2) {
					symbol = CG_DB_TR("★");
					sym_color = Color(1.0f, 0.90f, 0.25f, 0.95f);
				}

				// Dark badge behind symbol for visibility on any tileset
				Point2 center = cell_rect.position + cell_rect.size * 0.5f;
				draw_circle(center, 9.0f, Color(0.05f, 0.05f, 0.07f, 0.72f));

				if (font.is_valid()) {
					draw_string(font, Point2(cell_rect.position.x + 10, cell_rect.position.y + 22), symbol, HORIZONTAL_ALIGNMENT_CENTER, -1, 14, sym_color);
				}
			}
		}
	}
}

void ClockGearsTilesetCollisionEditor::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		if (tileset_texture.is_null()) {
			return;
		}
		Vector2 pos = mb->get_position();
		Size2 tex_size = tileset_texture->get_size();
		if (pos.x < 0 || pos.y < 0 || pos.x >= tex_size.x || pos.y >= tex_size.y) {
			return;
		}
		int tx = (int)(pos.x / tile_size);
		int ty = (int)(pos.y / tile_size);
		String key = vformat("%d,%d", tx, ty);
		int cur = collision_map.has(key) ? (int)collision_map[key] : 0;
		int next = (cur + 1) % 3; // 0: O (Pasa), 1: X (Bloqueado), 2: ★ (Pasa por detrás)
		collision_map[key] = next;
		queue_redraw();
		if (on_collision_changed.is_valid()) {
			on_collision_changed.call();
		}
		accept_event();
	}
}

void ClockGearsTilesetCollisionEditor::set_tileset_texture(const Ref<Texture2D> &p_tex) {
	tileset_texture = p_tex;
	if (tileset_texture.is_valid()) {
		set_custom_minimum_size(tileset_texture->get_size().max(Size2(280, 280)));
	} else {
		set_custom_minimum_size(Size2(280, 280));
	}
	queue_redraw();
}

void ClockGearsTilesetCollisionEditor::set_collision_map(const Dictionary &p_map) {
	collision_map = p_map.duplicate(true);
	queue_redraw();
}

ClockGearsTilesetCollisionEditor::ClockGearsTilesetCollisionEditor() {
	set_custom_minimum_size(Size2(280, 280));
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
}

// ============================================================================
// ClockGearsDatabaseDialog
// ============================================================================

void ClockGearsDatabaseDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_database"), &ClockGearsDatabaseDialog::open_database);
	ClassDB::bind_method(D_METHOD("select_category", "category"), &ClockGearsDatabaseDialog::select_category);
	ClassDB::bind_method(D_METHOD("add_tileset_record", "name"), &ClockGearsDatabaseDialog::add_tileset_record, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("remove_tileset_record", "index"), &ClockGearsDatabaseDialog::remove_tileset_record, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_tileset_count"), &ClockGearsDatabaseDialog::get_tileset_count);
	ClassDB::bind_method(D_METHOD("select_tileset_slot", "slot_key"), &ClockGearsDatabaseDialog::select_tileset_slot);
	ClassDB::bind_method(D_METHOD("get_active_tileset_slot"), &ClockGearsDatabaseDialog::get_active_tileset_slot);
	ClassDB::bind_method(D_METHOD("import_tileset_slot", "slot_key", "file_path"), &ClockGearsDatabaseDialog::import_tileset_slot);
	ClassDB::bind_method(D_METHOD("clear_tileset_slot", "slot_key"), &ClockGearsDatabaseDialog::clear_tileset_slot);
	ClassDB::bind_method(D_METHOD("get_tileset_slot_path", "record_idx", "slot_key"), &ClockGearsDatabaseDialog::get_tileset_slot_path);
	ClassDB::bind_method(D_METHOD("set_tileset_slot_tile_meta", "slot_key", "coords", "meta"), &ClockGearsDatabaseDialog::set_tileset_slot_tile_meta);
	ClassDB::bind_method(D_METHOD("drag_paint_tiles_meta", "slot_key", "from", "to", "meta"), &ClockGearsDatabaseDialog::drag_paint_tiles_meta);
	ClassDB::bind_method(D_METHOD("get_tileset_slot_tile_meta", "slot_key", "coords"), &ClockGearsDatabaseDialog::get_tileset_slot_tile_meta);
	ClassDB::bind_method(D_METHOD("save_to_disk"), &ClockGearsDatabaseDialog::save_to_disk);
	ClassDB::bind_method(D_METHOD("load_from_disk"), &ClockGearsDatabaseDialog::load_from_disk);
	ClassDB::bind_method(D_METHOD("set_remembered_window_size", "size"), &ClockGearsDatabaseDialog::set_remembered_window_size);
	ClassDB::bind_method(D_METHOD("get_remembered_window_size"), &ClockGearsDatabaseDialog::get_remembered_window_size);
	ClassDB::bind_method(D_METHOD("check_layout_fits_window", "window_size"), &ClockGearsDatabaseDialog::check_layout_fits_window);
	ClassDB::bind_method(D_METHOD("get_troop_member_preview_info", "troop_idx", "member_idx"), &ClockGearsDatabaseDialog::get_troop_member_preview_info);
	ClassDB::bind_method(D_METHOD("get_troop_preview_viewport_rect"), &ClockGearsDatabaseDialog::get_troop_preview_viewport_rect);
	ClassDB::bind_method(D_METHOD("get_troop_preview_enemy_layout", "member_idx"), &ClockGearsDatabaseDialog::get_troop_preview_enemy_layout);
	ClassDB::bind_method(D_METHOD("get_enemy_anim_slot_info", "enemy_idx", "slot_id"), &ClockGearsDatabaseDialog::get_enemy_anim_slot_info);
	ClassDB::bind_method(D_METHOD("reassign_enemy_other_animation", "enemy_idx", "other_name", "target_slot"), &ClockGearsDatabaseDialog::reassign_enemy_other_animation);
	ClassDB::bind_method(D_METHOD("remove_enemy_other_animation", "enemy_idx", "other_name"), &ClockGearsDatabaseDialog::remove_enemy_other_animation);
	ClassDB::bind_method(D_METHOD("select_anim_slot", "slot_id"), &ClockGearsDatabaseDialog::select_anim_slot);
	ClassDB::bind_method(D_METHOD("remove_anim_frame_at", "frame_idx"), &ClockGearsDatabaseDialog::remove_anim_frame_at);
	ClassDB::bind_method(D_METHOD("set_system_movement_directions", "dirs"), &ClockGearsDatabaseDialog::set_system_movement_directions);
	ClassDB::bind_method(D_METHOD("get_system_movement_directions"), &ClockGearsDatabaseDialog::get_system_movement_directions);
	ClassDB::bind_method(D_METHOD("set_system_mannequin_size", "size"), &ClockGearsDatabaseDialog::set_system_mannequin_size);
	ClassDB::bind_method(D_METHOD("get_system_mannequin_size"), &ClockGearsDatabaseDialog::get_system_mannequin_size);
	ClassDB::bind_method(D_METHOD("set_anim_submode", "mode"), &ClockGearsDatabaseDialog::set_anim_submode);
	ClassDB::bind_method(D_METHOD("get_anim_submode"), &ClockGearsDatabaseDialog::get_anim_submode);
	ClassDB::bind_method(D_METHOD("get_shape_editor"), &ClockGearsDatabaseDialog::get_shape_editor);
	ClassDB::bind_method(D_METHOD("select_shape_frame", "frame_idx"), &ClockGearsDatabaseDialog::select_shape_frame);
	ClassDB::bind_method(D_METHOD("copy_shapes_to_next_frame"), &ClockGearsDatabaseDialog::copy_shapes_to_next_frame);
	ClassDB::bind_method(D_METHOD("copy_shapes_to_all_frames"), &ClockGearsDatabaseDialog::copy_shapes_to_all_frames);
	ClassDB::bind_method(D_METHOD("copy_shapes_from_direction", "from_dir", "mode"), &ClockGearsDatabaseDialog::copy_shapes_from_direction, DEFVAL("auto"));
	ClassDB::bind_method(D_METHOD("set_actor_directional_idle", "enabled"), &ClockGearsDatabaseDialog::set_actor_directional_idle);
	ClassDB::bind_method(D_METHOD("is_actor_directional_idle"), &ClockGearsDatabaseDialog::is_actor_directional_idle);
	ClassDB::bind_method(D_METHOD("get_troop_battleback_cfg", "troop_idx", "slot"), &ClockGearsDatabaseDialog::get_troop_battleback_cfg);
	ClassDB::bind_method(D_METHOD("set_troop_battleback_cfg", "troop_idx", "slot", "cfg"), &ClockGearsDatabaseDialog::set_troop_battleback_cfg);
	ClassDB::bind_method(D_METHOD("get_formatted_troop_enemy_label", "troop_idx", "member_idx", "max_w"), &ClockGearsDatabaseDialog::get_formatted_troop_enemy_label, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("get_chapter_structure_info"), &ClockGearsDatabaseDialog::get_chapter_structure_info);
	ClassDB::bind_method(D_METHOD("select_category_by_key", "key"), &ClockGearsDatabaseDialog::select_category_by_key);
	ClassDB::bind_method(D_METHOD("get_record_list_icon", "row_idx"), &ClockGearsDatabaseDialog::get_record_list_icon);
	ClassDB::bind_method(D_METHOD("open_record_event_editor"), &ClockGearsDatabaseDialog::open_record_event_editor);
	ClassDB::bind_method(D_METHOD("get_body_zones_editor"), &ClockGearsDatabaseDialog::get_body_zones_editor);
	ClassDB::bind_method(D_METHOD("get_movement_shape_editor"), &ClockGearsDatabaseDialog::get_movement_shape_editor);
	ClassDB::bind_method(D_METHOD("get_main_editor_tabs"), &ClockGearsDatabaseDialog::get_main_editor_tabs);
	ClassDB::bind_method(D_METHOD("select_main_tab_by_name", "name"), &ClockGearsDatabaseDialog::select_main_tab_by_name);
	ClassDB::bind_method(D_METHOD("check_all_tabs_layout", "window_size"), &ClockGearsDatabaseDialog::check_all_tabs_layout);
	ClassDB::bind_method(D_METHOD("adapt_to_window_size", "window_size"), &ClockGearsDatabaseDialog::adapt_to_window_size);
	ClassDB::bind_method(D_METHOD("set_menus_preview_resolution", "width", "height"), &ClockGearsDatabaseDialog::set_menus_preview_resolution);
	ClassDB::bind_method(D_METHOD("get_menus_preview_resolution"), &ClockGearsDatabaseDialog::get_menus_preview_resolution);
	ClassDB::bind_method(D_METHOD("is_realtime_combat_section_visible"), &ClockGearsDatabaseDialog::is_realtime_combat_section_visible);
	ClassDB::bind_method(D_METHOD("set_system_combat_mode", "mode"), &ClockGearsDatabaseDialog::set_system_combat_mode);
	ClassDB::bind_method(D_METHOD("get_system_config_copy"), &ClockGearsDatabaseDialog::get_system_config_copy);
	ClassDB::bind_method(D_METHOD("set_system_realtime_value", "key", "value"), &ClockGearsDatabaseDialog::set_system_realtime_value);
	ClassDB::bind_method(D_METHOD("get_title_menu_draft_copy"), &ClockGearsDatabaseDialog::get_title_menu_draft_copy);
	ClassDB::bind_method(D_METHOD("set_title_menu_entry_state", "id", "enabled", "text"), &ClockGearsDatabaseDialog::set_title_menu_entry_state);
	ClassDB::bind_method(D_METHOD("move_title_menu_entry", "id", "delta"), &ClockGearsDatabaseDialog::move_title_menu_entry);
	ClassDB::bind_method(D_METHOD("set_title_menu_background", "type", "color_or_path", "scale_mode", "filter"), &ClockGearsDatabaseDialog::set_title_menu_background, DEFVAL("fit"), DEFVAL("nearest"));
	ClassDB::bind_method(D_METHOD("set_title_menu_title_config", "type", "text_or_path", "filter"), &ClockGearsDatabaseDialog::set_title_menu_title_config, DEFVAL("nearest"));
	ClassDB::bind_method(D_METHOD("set_title_menu_bgm", "path"), &ClockGearsDatabaseDialog::set_title_menu_bgm);
	ClassDB::bind_method(D_METHOD("get_title_menu_live_preview"), &ClockGearsDatabaseDialog::get_title_menu_live_preview);
	ClassDB::bind_method(D_METHOD("open_title_menu_test_popup"), &ClockGearsDatabaseDialog::open_title_menu_test_popup);
	ClassDB::bind_method(D_METHOD("get_title_menu_test_popup"), &ClockGearsDatabaseDialog::get_title_menu_test_popup);
	ClassDB::bind_method(D_METHOD("get_window_styles_draft_copy"), &ClockGearsDatabaseDialog::get_window_styles_draft_copy);
	ClassDB::bind_method(D_METHOD("select_window_style", "index"), &ClockGearsDatabaseDialog::select_window_style);
	ClassDB::bind_method(D_METHOD("get_window_styles_ui_handles"), &ClockGearsDatabaseDialog::get_window_styles_ui_handles);
	ClassDB::bind_method(D_METHOD("get_window_style_preview_info"), &ClockGearsDatabaseDialog::get_window_style_preview_info);
	ClassDB::bind_method(D_METHOD("get_game_menu_draft_copy"), &ClockGearsDatabaseDialog::get_game_menu_draft_copy);
	ClassDB::bind_method(D_METHOD("set_game_menu_entry_state", "id", "enabled", "text"), &ClockGearsDatabaseDialog::set_game_menu_entry_state);
	ClassDB::bind_method(D_METHOD("move_game_menu_entry", "id", "delta"), &ClockGearsDatabaseDialog::move_game_menu_entry);
	ClassDB::bind_method(D_METHOD("select_game_menu_preview_screen", "screen_id"), &ClockGearsDatabaseDialog::select_game_menu_preview_screen);
	ClassDB::bind_method(D_METHOD("set_game_menu_screen_background", "screen_id", "type", "color_or_path", "scale_mode", "filter"), &ClockGearsDatabaseDialog::set_game_menu_screen_background, DEFVAL("fit"), DEFVAL("nearest"));
	ClassDB::bind_method(D_METHOD("get_game_menu_live_preview"), &ClockGearsDatabaseDialog::get_game_menu_live_preview);
	ClassDB::bind_method(D_METHOD("open_game_menu_test_popup"), &ClockGearsDatabaseDialog::open_game_menu_test_popup);
	ClassDB::bind_method(D_METHOD("get_game_menu_test_popup"), &ClockGearsDatabaseDialog::get_game_menu_test_popup);
	ClassDB::bind_method(D_METHOD("get_game_menu_editor_ui_handles"), &ClockGearsDatabaseDialog::get_game_menu_editor_ui_handles);
	ClassDB::bind_method(D_METHOD("get_credits_draft_copy"), &ClockGearsDatabaseDialog::get_credits_draft_copy);
	ClassDB::bind_method(D_METHOD("add_credits_item", "type", "fields"), &ClockGearsDatabaseDialog::add_credits_item, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_credits_item", "index"), &ClockGearsDatabaseDialog::remove_credits_item);
	ClassDB::bind_method(D_METHOD("move_credits_item", "index", "delta"), &ClockGearsDatabaseDialog::move_credits_item);
	ClassDB::bind_method(D_METHOD("duplicate_credits_item", "index"), &ClockGearsDatabaseDialog::duplicate_credits_item);
	ClassDB::bind_method(D_METHOD("select_credits_item", "index"), &ClockGearsDatabaseDialog::select_credits_item);
	ClassDB::bind_method(D_METHOD("set_credits_item_field", "index", "key", "val"), &ClockGearsDatabaseDialog::set_credits_item_field);
	ClassDB::bind_method(D_METHOD("get_credits_live_preview"), &ClockGearsDatabaseDialog::get_credits_live_preview);
	ClassDB::bind_method(D_METHOD("open_credits_test_popup"), &ClockGearsDatabaseDialog::open_credits_test_popup);
	ClassDB::bind_method(D_METHOD("get_credits_test_popup"), &ClockGearsDatabaseDialog::get_credits_test_popup);
	ClassDB::bind_method(D_METHOD("get_credits_test_player"), &ClockGearsDatabaseDialog::get_credits_test_player);
	ClassDB::bind_method(D_METHOD("get_credits_editor_ui_handles"), &ClockGearsDatabaseDialog::get_credits_editor_ui_handles);
	ClassDB::bind_method(D_METHOD("get_hud_draft_copy"), &ClockGearsDatabaseDialog::get_hud_draft_copy);
	ClassDB::bind_method(D_METHOD("add_hud_widget", "type", "fields"), &ClockGearsDatabaseDialog::add_hud_widget, DEFVAL("bar"), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_hud_widget", "index"), &ClockGearsDatabaseDialog::remove_hud_widget);
	ClassDB::bind_method(D_METHOD("duplicate_hud_widget", "index"), &ClockGearsDatabaseDialog::duplicate_hud_widget);
	ClassDB::bind_method(D_METHOD("move_hud_widget_layer", "index", "delta"), &ClockGearsDatabaseDialog::move_hud_widget_layer);
	ClassDB::bind_method(D_METHOD("select_hud_widget", "index"), &ClockGearsDatabaseDialog::select_hud_widget);
	ClassDB::bind_method(D_METHOD("set_hud_widget_field", "index", "key", "val"), &ClockGearsDatabaseDialog::set_hud_widget_field);
	ClassDB::bind_method(D_METHOD("set_hud_preview_resolution", "width", "height"), &ClockGearsDatabaseDialog::set_hud_preview_resolution);
	ClassDB::bind_method(D_METHOD("set_hud_preview_sample_values", "hp", "gold", "item_qty"), &ClockGearsDatabaseDialog::set_hud_preview_sample_values);
	ClassDB::bind_method(D_METHOD("get_hud_live_preview"), &ClockGearsDatabaseDialog::get_hud_live_preview);
	ClassDB::bind_method(D_METHOD("get_hud_hearts_show_bg_checkbox"), &ClockGearsDatabaseDialog::get_hud_hearts_show_bg_checkbox);
	ClassDB::bind_method(D_METHOD("get_hud_editor_ui_handles"), &ClockGearsDatabaseDialog::get_hud_editor_ui_handles);
	ClassDB::bind_method(D_METHOD("get_ui_designer_editor"), &ClockGearsDatabaseDialog::get_ui_designer_editor);
	ClassDB::bind_method(D_METHOD("get_ui_designer_ui_handles"), &ClockGearsDatabaseDialog::get_ui_designer_ui_handles);
	ClassDB::bind_method(D_METHOD("get_game_over_draft_copy"), &ClockGearsDatabaseDialog::get_game_over_draft_copy);
	ClassDB::bind_method(D_METHOD("get_game_over_live_preview"), &ClockGearsDatabaseDialog::get_game_over_live_preview);
	ClassDB::bind_method(D_METHOD("get_game_over_editor_ui_handles"), &ClockGearsDatabaseDialog::get_game_over_editor_ui_handles);
	ClassDB::bind_method(D_METHOD("_on_file_selected", "path"), &ClockGearsDatabaseDialog::_on_file_selected);
}

void ClockGearsDatabaseDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (EditorFileSystem::get_singleton()) {
				EditorFileSystem *efs = EditorFileSystem::get_singleton();
				if (!efs->is_connected("filesystem_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_filesystem_changed))) {
					efs->connect("filesystem_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_filesystem_changed));
				}
				if (!efs->is_connected("resources_reimported", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_resources_reimported))) {
					efs->connect("resources_reimported", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_resources_reimported));
				}
			}
			if (EditorSettings::get_singleton() && !EditorSettings::get_singleton()->is_connected("settings_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_settings_changed))) {
				EditorSettings::get_singleton()->connect("settings_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_settings_changed));
			}
#endif
			if (ClockGearsIcons::get_editor_icon_scale(this) != last_icon_scale) {
				_refresh_icon_scale();
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
#ifdef TOOLS_ENABLED
			if (EditorFileSystem::get_singleton()) {
				EditorFileSystem *efs = EditorFileSystem::get_singleton();
				if (efs->is_connected("filesystem_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_filesystem_changed))) {
					efs->disconnect("filesystem_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_filesystem_changed));
				}
				if (efs->is_connected("resources_reimported", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_resources_reimported))) {
					efs->disconnect("resources_reimported", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_resources_reimported));
				}
			}
			if (EditorSettings::get_singleton() && EditorSettings::get_singleton()->is_connected("settings_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_settings_changed))) {
				EditorSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_editor_settings_changed));
			}
#endif
		} break;
		case NOTIFICATION_PROCESS: {
			if (anim_preview_playing && is_visible()) {
				_update_anim_playback(get_process_delta_time());
			}
			if (troop_waiting_tex_ready && is_visible() && current_category == CAT_TROOPS) {
				troop_tex_poll_timer += get_process_delta_time();
				if (troop_tex_poll_timer >= 0.15f) {
					troop_tex_poll_timer = 0.0f;
					troop_waiting_tex_ready = false;
					_clear_troop_texture_cache();
					_queue_troop_preview_redraw();
				}
			}
		} break;
		case NOTIFICATION_WM_CLOSE_REQUEST:
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!is_visible()) {
				_restore_hud_preview_runtime_state();
				_clear_troop_texture_cache();
				_save_editor_ui_state();
			}
		} break;
	}
}

String ClockGearsDatabaseDialog::_get_slot_key(int p_slot_idx) {
	static const char *slot_keys[TILESET_SLOT_COUNT] = { "A1", "A2", "A3", "A4", "A5", "B", "C", "D", "E" };
	if (p_slot_idx < 0 || p_slot_idx >= TILESET_SLOT_COUNT) {
		return "A5";
	}
	return String(slot_keys[p_slot_idx]);
}

String ClockGearsDatabaseDialog::_get_slot_label(int p_slot_idx) {
	static const char *slot_labels[TILESET_SLOT_COUNT] = {
		"A1 (Agua / Animados - 32x32):",
		"A2 (Suelo / Autotile - 32x32):",
		"A3 (Edificios / Fachadas - 32x32):",
		"A4 (Muros / Techos - 32x32):",
		"A5 (Tiles Normales - 32x32):",
		"B (Objetos y Decoración 1 - 32x32):",
		"C (Objetos y Decoración 2 - 32x32):",
		"D (Objetos y Decoración 3 - 32x32):",
		"E (Objetos y Decoración 4 - 32x32):"
	};
	if (p_slot_idx < 0 || p_slot_idx >= TILESET_SLOT_COUNT) {
		return CG_DB_TR("A5 (Tiles Normales - 32x32):");
	}
	return CG_DB_TR(slot_labels[p_slot_idx]);
}

int ClockGearsDatabaseDialog::_find_slot_index(const String &p_slot_key) {
	String up = p_slot_key.strip_edges().to_upper();
	for (int i = 0; i < TILESET_SLOT_COUNT; i++) {
		if (_get_slot_key(i) == up) {
			return i;
		}
	}
	return 4; // A5 por defecto
}

void ClockGearsDatabaseDialog::_init_empty_database() {
	// STRICT ZERO-PRESETS RULE: All categories initialize completely empty.
	for (int i = 0; i < CAT_MAX; i++) {
		category_data[i].clear();
	}
	system_config.clear();
	system_config["game_title"] = "";
	system_config["currency"] = "G";
	system_config["combat_mode"] = COMBAT_MODE_TURN_BASED;
	system_config["movement_directions"] = 4;
	system_config["mannequin_size"] = 32;
	system_config["arpg_move_while_attacking"] = true;
	system_config["arpg_attack_move_speed_pct"] = 100;
	system_config["arpg_move_while_blocking"] = false;
	system_config["arpg_block_move_speed_pct"] = 50;
	system_config["start_map"] = "";
	system_config["start_x"] = 0;
	system_config["start_y"] = 0;
	system_config["camera_zoom"] = 2;
	system_config["camera_smooth"] = true;
	system_config["camera_clamp"] = true;
	system_config["party"] = Array();
	menu_title_config = ClockGearsDataManager::get_default_title_menu_config();
	game_menu_config_draft = ClockGearsDataManager::get_default_game_menu_config();
}

String ClockGearsDatabaseDialog::_get_category_name(int p_category) const {
	switch (p_category) {
		case CAT_ACTORS:
			return CG_DB_TR("Actores");
		case CAT_CLASSES:
			return CG_DB_TR("Clases");
		case CAT_SKILLS:
			return CG_DB_TR("Habilidades");
		case CAT_ITEMS:
			return CG_DB_TR("Objetos");
		case CAT_WEAPONS:
			return CG_DB_TR("Armas");
		case CAT_ARMORS:
			return CG_DB_TR("Armaduras");
		case CAT_ACCESSORIES:
			return CG_DB_TR("Accesorios");
		case CAT_ENEMIES:
			return CG_DB_TR("Enemigos");
		case CAT_TROOPS:
			return CG_DB_TR("Tropas");
		case CAT_STATES:
			return CG_DB_TR("Estados");
		case CAT_FLORA:
			return CG_DB_TR("Flora");
		case CAT_FAUNA:
			return CG_DB_TR("Fauna");
		case CAT_ANIMATIONS:
			return CG_DB_TR("Animaciones");
		case CAT_TILESETS:
			return CG_DB_TR("Tilesets");
		case CAT_COMMON_EVENTS:
			return CG_DB_TR("Eventos Comunes");
		case CAT_SYSTEM:
			return CG_DB_TR("Sistema");
		case CAT_TYPES:
			return CG_DB_TR("Tipos");
		case CAT_VOCABULARY:
			return CG_DB_TR("Vocabulario");
		case CAT_SWITCHES_VARS:
			return CG_DB_TR("Interruptores");
		case CAT_NPCS:
			return CG_DB_TR("NPC");
		case CAT_ARPG_ENEMIES:
			return CG_DB_TR("Enemigos ARPG");
		case CAT_ARPG_NPCS:
			return CG_DB_TR("NPC ARPG");
		case CAT_ARPG_WEAPONS:
			return CG_DB_TR("Armas ARPG");
		case CAT_ARPG_ARMORS:
			return CG_DB_TR("Armaduras ARPG");
		case CAT_ARPG_SHIELDS:
			return CG_DB_TR("Escudos ARPG");
		case CAT_ARPG_ACCESSORIES:
			return CG_DB_TR("Accesorios ARPG");
		case DB_CAT_MAIN_MENU:
			return CG_DB_TR("Menú principal");
		case DB_CAT_WINDOW_STYLES:
			return CG_DB_TR("Estilos de ventana");
		case DB_CAT_GAME_MENU:
			return CG_DB_TR("Menú del juego");
		case DB_CAT_CREDITS:
			return CG_DB_TR("Créditos");
		case DB_CAT_HUD:
			return CG_DB_TR("HUD");
		case DB_CAT_UI_DESIGNER:
			return CG_DB_TR("Diseño de interfaz");
		case DB_CAT_GAME_OVER:
			return CG_DB_TR("Game Over");
		default:
			return CG_DB_TR("Datos");
	}
}

Dictionary ClockGearsDatabaseDialog::_create_empty_record(int p_category, int p_id) const {
	Dictionary d;
	d["id"] = p_id;
	d["name"] = "";
	d["desc"] = "";
	d["type"] = 0;
	d["secondary_type"] = 0;
	d["tertiary_type"] = 0;
	d["formula"] = "max(a.atk * 4 - b.def * 2, 0)";
	bool is_equip = (p_category == CAT_WEAPONS || p_category == CAT_ARMORS || p_category == CAT_ACCESSORIES);
	bool is_arpg_equip = s_is_arpg_equip_category(p_category);
	bool zero_stats = is_equip || is_arpg_equip;
	d["hp"] = zero_stats ? 0 : 100;
	d["mp"] = zero_stats ? 0 : 20;
	d["atk"] = zero_stats ? 0 : 10;
	d["def"] = zero_stats ? 0 : 10;
	d["mat"] = zero_stats ? 0 : 10;
	d["mdf"] = zero_stats ? 0 : 10;
	d["agi"] = zero_stats ? 0 : 10;
	d["luk"] = zero_stats ? 0 : 10;
	d["graphic_path"] = "";
	d["traits"] = Array();
	if (p_category == CAT_ACTORS) {
		d["class_id"] = 0;
		d["directional_idle"] = false;
		d["arpg_equip_slots"] = ClockGearsDataManager::get_default_arpg_equip_slots();
		Dictionary ca;
		ca["idle"] = "";
		ca["attack"] = "";
		ca["damage"] = "";
		ca["death"] = "";
		ca["defend"] = "";
		ca["victory"] = "";
		d["combat_animations"] = ca;
	} else if (s_is_anim_slot_category(p_category)) {
		d["directional_idle"] = false;
		d["sprite_data"] = Dictionary();
		if (is_arpg_equip) {
			d["draw_mode"] = "completo";
			d["layer_order"] = ClockGearsCharacterStateRegistry::get_default_arpg_layer_order(p_category);
			d["simple_images"] = Dictionary();
		}
		if (s_has_event_tab_category(p_category)) {
			d["event_pages"] = Array();
		}
	} else if (p_category == CAT_CLASSES) {
		// Sección 9.3: Parámetros por defecto de la fórmula de EXP (base=30, extra=20, acc_a=30, acc_b=30)
		d["exp_base"] = 30;
		d["exp_extra"] = 20;
		d["exp_acc_a"] = 30;
		d["exp_acc_b"] = 30;
	} else if (is_equip) {
		// Sección 5.1: Armas, Armaduras y Accesorios
		d["price"] = 0;
		d["attack_anim_id"] = 0;
		d["animation_id"] = 0;
		if (p_category == CAT_WEAPONS) {
			d["secondary_type"] = 0; // Mano Derecha (Arma)
		} else if (p_category == CAT_ARMORS) {
			d["secondary_type"] = 3; // Cuerpo (Pechera)
		} else if (p_category == CAT_ACCESSORIES) {
			d["type"] = 4; // Accesorio
		}
	} else if (p_category == CAT_ITEMS || p_category == CAT_SKILLS) {
		// Sección 5.2: Objetos y Habilidades
		d["price"] = 0;
		d["attack_anim_id"] = 0;
		d["animation_id"] = 0;
		d["consumable"] = true;
		d["speed"] = 0;
		d["success_rate"] = 100;
		d["repeats"] = 1;
		d["tp_gain"] = 0;
		d["hit_type"] = 0; // 0 = Seguro, 1 = Físico, 2 = Mágico
		d["element_id"] = 0;
		d["variance"] = 20;
		d["critical"] = false;
		d["effects"] = Array();
	} else if (p_category == CAT_ENEMIES) {
		// Sección 5.3: Enemigos (Nivel, EXP, Oro, Ranuras fijas de animación, 3 Drops, Patrones de Acción)
		d["level"] = 1;
		d["exp"] = 0;
		d["gold"] = 0;
		Dictionary ca;
		ca["idle"] = "idle";
		ca["attack"] = "attack";
		ca["damage"] = "hurt";
		ca["death"] = "death";
		ca["buff"] = "buff";
		ca["defend"] = "idle";
		d["combat_animations"] = ca;
		d["cast_animations"] = Array();
		d["other_animations"] = Array();
		ClockGearsSpriteSheetData sp_init;
		const char *fixed_slots[] = { "idle", "attack", "hurt", "death", "buff" };
		for (size_t fs = 0; fs < 5; fs++) {
			ClockGearsSpriteAnimation sa;
			sa.name = String::utf8(fixed_slots[fs]);
			sa.fps = 6.0f;
			sa.loop = (fs == 0);
			sp_init.animations[sa.name] = sa;
		}
		d["sprite_data"] = sp_init.to_dict();
		Array drops;
		for (int s = 0; s < 3; s++) {
			Dictionary dr;
			dr["kind"] = 0;
			dr["item_id"] = 0;
			dr["prob"] = 100;
			drops.push_back(dr);
		}
		d["drops"] = drops;
		d["actions"] = Array();
	} else if (p_category == CAT_TROOPS) {
		d["members"] = Array();
		d["battleback1"] = "";
		d["battleback2"] = "";
	} else if (p_category == CAT_TILESETS) {
		Dictionary slots;
		Dictionary slot_cols;
		for (int s = 0; s < TILESET_SLOT_COUNT; s++) {
			String sk = _get_slot_key(s);
			slots[sk] = "";
			slot_cols[sk] = Dictionary();
		}
		d["slots"] = slots;
		d["slot_collisions"] = slot_cols;
		d["collisions"] = Dictionary();
	}
	return d;
}

void ClockGearsDatabaseDialog::_populate_db_records_option(OptionButton *p_opt, int p_db_category, const String &p_empty_text, bool p_only_user_records_when_non_empty) {
	if (!p_opt || p_db_category < 0 || p_db_category >= CAT_MAX) {
		return;
	}
	p_opt->clear();
	const Array &arr = category_data[p_db_category];
	if (arr.is_empty()) {
		p_opt->add_item(p_empty_text, 0);
		return;
	}
	if (!p_only_user_records_when_non_empty) {
		p_opt->add_item(p_empty_text, 0);
	}
	for (int i = 0; i < arr.size(); i++) {
		Dictionary rec = arr[i];
		int rec_id = int(rec.get("id", i + 1));
		String rec_name = rec.get("name", "");
		p_opt->add_item(vformat("%03d: %s", rec_id, rec_name), rec_id);
	}
}

void ClockGearsDatabaseDialog::_populate_catalog_option(OptionButton *p_opt, const PackedStringArray &p_items) {
	if (!p_opt) {
		return;
	}
	p_opt->clear();
	for (int i = 0; i < p_items.size(); i++) {
		p_opt->add_item(p_items[i], i);
	}
}

void ClockGearsDatabaseDialog::_select_option_by_id_or_index(OptionButton *p_opt, int p_id_or_idx) {
	if (!p_opt || p_opt->get_item_count() == 0) {
		return;
	}
	int by_id = p_opt->get_item_index(p_id_or_idx);
	if (by_id >= 0) {
		p_opt->select(by_id);
	} else if (p_id_or_idx >= 0 && p_id_or_idx < p_opt->get_item_count()) {
		p_opt->select(p_id_or_idx);
	} else {
		p_opt->select(0);
	}
}

void ClockGearsDatabaseDialog::_update_exp_preview() {
	if (!exp_preview_label || !exp_spins[0] || !exp_spins[1] || !exp_spins[2] || !exp_spins[3]) {
		return;
	}
	int base = int(exp_spins[0]->get_value());
	int extra = int(exp_spins[1]->get_value());
	int acc_a = int(exp_spins[2]->get_value());
	int acc_b = int(exp_spins[3]->get_value());
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	if (fm) {
		int64_t exp2 = fm->calculate_exp_for_level(2, base, extra, acc_a, acc_b);
		int64_t exp10 = fm->calculate_exp_for_level(10, base, extra, acc_a, acc_b);
		int64_t exp50 = fm->calculate_exp_for_level(50, base, extra, acc_a, acc_b);
		int64_t exp99 = fm->calculate_exp_for_level(99, base, extra, acc_a, acc_b);
		exp_preview_label->set_text(vformat(CG_DB_TR("Nv 2: %d | Nv 10: %d | Nv 50: %d | Nv 99: %d"), exp2, exp10, exp50, exp99));
	}
}

Ref<Texture2D> ClockGearsDatabaseDialog::_load_texture_from_path(const String &p_path) const {
	if (p_path.strip_edges().is_empty()) {
		return Ref<Texture2D>();
	}

	String fixed_path = s_fix_mojibake(p_path);

	Vector<String> candidates;
	candidates.push_back(fixed_path);
	if (fixed_path != p_path) {
		candidates.push_back(p_path);
	}
	if (ProjectSettings::get_singleton()) {
		String loc_p = ProjectSettings::get_singleton()->localize_path(fixed_path);
		if (!loc_p.is_empty() && loc_p != fixed_path) {
			candidates.push_back(loc_p);
		}
		String glob_p = ProjectSettings::get_singleton()->globalize_path(fixed_path);
		if (!glob_p.is_empty() && glob_p != fixed_path) {
			candidates.push_back(glob_p);
		}
	}
	String fname = fixed_path.get_file();
	if (!fname.is_empty()) {
		candidates.push_back(String("res://img/battlebacks/").path_join(fname));
		candidates.push_back(String("res://img/battlers/").path_join(fname));
		candidates.push_back(String("res://img/characters/").path_join(fname));
		candidates.push_back(String("res://img/enemies/").path_join(fname));
		candidates.push_back(String("res://img/tilesets/").path_join(fname));
		candidates.push_back(String("res://img/system/").path_join(fname));
		candidates.push_back(String("res://img/").path_join(fname));
		candidates.push_back(String("res://").path_join(fname));

		if (ProjectSettings::get_singleton()) {
			candidates.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/battlebacks/" + fname));
			candidates.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/battlers/" + fname));
			candidates.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/enemies/" + fname));
			candidates.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/characters/" + fname));
			candidates.push_back(ProjectSettings::get_singleton()->globalize_path("res://img/" + fname));
		}

		const char *known_subdirs[] = { "img/battlebacks", "img/battlers", "img/characters", "img/enemies", "img/tilesets", "img/system", "img" };
		for (size_t s = 0; s < sizeof(known_subdirs) / sizeof(known_subdirs[0]); s++) {
			String g_dir = ClockGearsProjectStructure::resolve_project_folder_global(known_subdirs[s]);
			if (!g_dir.is_empty()) {
				candidates.push_back(g_dir.path_join(fname));
			}
		}
	}

	for (int i = 0; i < candidates.size(); i++) {
		const String &cand = candidates[i];
		if (cand.is_empty()) continue;
		if (ClockGearsBattleScene::can_safely_resource_load_image(cand)) {
			Ref<Texture2D> res = ResourceLoader::load(cand);
			if (ClockGearsBattleScene::is_texture_ready(res)) {
				return res;
			}
		}
		String file_read_path = cand;
		if (cand.begins_with("res://") && ProjectSettings::get_singleton()) {
			String gp = ProjectSettings::get_singleton()->globalize_path(cand);
			if (!gp.is_empty() && FileAccess::exists(gp)) {
				file_read_path = gp;
			}
		}
		if (FileAccess::exists(file_read_path)) {
			Ref<Image> local_img;
			local_img.instantiate();
			if (ImageLoader::load_image(file_read_path, local_img) == OK && local_img.is_valid() && !local_img->is_empty()) {
				Ref<ImageTexture> itex = ImageTexture::create_from_image(local_img);
				if (ClockGearsBattleScene::is_texture_ready(itex)) {
					return itex;
				}
			}
		}
	}

	// Fallback por escaneo en subdirectorios img/
	String search_dirs[] = { "res://img/battlers", "res://img/enemies", "res://img/characters", "res://img" };
	for (size_t d = 0; d < sizeof(search_dirs) / sizeof(search_dirs[0]); d++) {
		Ref<DirAccess> da = DirAccess::open(search_dirs[d]);
		if (da.is_valid()) {
			da->list_dir_begin();
			String file = da->get_next();
			while (!file.is_empty()) {
				if (!da->current_is_dir() && !file.ends_with(".import")) {
					if (file.to_lower() == fname.to_lower() || (fname.get_extension() == file.get_extension() && file.begins_with(fname.substr(0, 3)))) {
						String cand_p = search_dirs[d].path_join(file);
						Ref<Image> s_img = Image::load_from_file(cand_p);
						if (s_img.is_valid() && !s_img->is_empty()) {
							Ref<ImageTexture> it = ImageTexture::create_from_image(s_img);
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

static String s_get_editor_db_ui_config_path() {
	if (EditorPaths::get_singleton() && !EditorPaths::get_singleton()->get_config_dir().is_empty()) {
		return EditorPaths::get_singleton()->get_config_dir().path_join("clockgears_database_ui.json");
	}
	return "user://clockgears_database_ui.json";
}

static void s_reparent_control(Control *p_ctrl, Control *p_new_parent) {
	if (!p_ctrl || !p_new_parent || p_ctrl->get_parent() == p_new_parent) {
		return;
	}
	if (p_ctrl->get_parent()) {
		p_ctrl->get_parent()->remove_child(p_ctrl);
	}
	p_new_parent->add_child(p_ctrl);
}

void ClockGearsDatabaseDialog::_init_chapters() {
	chapters.clear();
	auto add_chap = [&](const char *p_name, std::initializer_list<int> p_cats) {
		ChapterDef ch;
		ch.name = CG_DB_TR(p_name);
		for (int c : p_cats) {
			ch.categories.push_back(c);
		}
		chapters.push_back(ch);
	};

	add_chap("Personajes", { CAT_ACTORS, CAT_NPCS, CAT_CLASSES });
	add_chap("Equipo", { CAT_WEAPONS, CAT_ARMORS, CAT_ACCESSORIES });
	add_chap("Objetos", { CAT_ITEMS });
	add_chap("Combate", { CAT_SKILLS, CAT_ENEMIES, CAT_TROOPS, CAT_STATES });
	add_chap("Action RPG", { CAT_ARPG_ENEMIES, CAT_ARPG_NPCS, CAT_ARPG_WEAPONS, CAT_ARPG_ARMORS, CAT_ARPG_SHIELDS, CAT_ARPG_ACCESSORIES });
	add_chap("Animaciones", { CAT_ANIMATIONS });
	add_chap("Mundo", { CAT_TILESETS, CAT_FLORA, CAT_FAUNA, CAT_COMMON_EVENTS });
	add_chap("Sistema", { CAT_SYSTEM, CAT_TYPES, CAT_VOCABULARY, CAT_SWITCHES_VARS });
	add_chap("Menús", { DB_CAT_MAIN_MENU, DB_CAT_WINDOW_STYLES, DB_CAT_GAME_MENU, DB_CAT_CREDITS, DB_CAT_HUD, DB_CAT_UI_DESIGNER, DB_CAT_GAME_OVER });

	// Cualquier categoría futura que no esté listada entra al capítulo más cercano sin perderse
	for (int c = 0; c < CAT_MAX; c++) {
		bool found = false;
		for (int i = 0; i < chapters.size(); i++) {
			if (chapters[i].categories.has(c)) {
				found = true;
				break;
			}
		}
		if (!found && !chapters.is_empty()) {
			chapters.write[chapters.size() - 1].categories.push_back(c);
		}
	}
}

int ClockGearsDatabaseDialog::_find_chapter_for_category(int p_category) const {
	for (int i = 0; i < chapters.size(); i++) {
		if (chapters[i].categories.has(p_category)) {
			return i;
		}
	}
	return 0;
}

void ClockGearsDatabaseDialog::_update_chapter_and_category_visibility() {
	int mode = current_combat_mode;
	if (ClockGearsDataManager::get_singleton()) {
		Dictionary dm_sys = ClockGearsDataManager::get_singleton()->get_system_config();
		if (dm_sys.has("combat_mode")) {
			mode = int(dm_sys["combat_mode"]);
			current_combat_mode = mode;
			system_config["combat_mode"] = mode;
		}
	}

	for (int i = 0; i < chapters.size() && i < chapter_buttons.size(); i++) {
		bool any_visible = false;
		for (int j = 0; j < chapters[i].categories.size(); j++) {
			if (s_is_category_visible_in_mode(chapters[i].categories[j], mode)) {
				any_visible = true;
				break;
			}
		}
		if (chapter_buttons[i]) {
			chapter_buttons[i]->set_visible(any_visible);
			chapter_buttons[i]->set_pressed_no_signal(i == current_chapter);
		}
	}

	const Vector<int> &chap_cats = (current_chapter >= 0 && current_chapter < chapters.size())
			? chapters[current_chapter].categories
			: Vector<int>();
	for (int c = 0; c < category_buttons.size(); c++) {
		if (category_buttons[c]) {
			bool vis = (chap_cats.is_empty() || chap_cats.has(c)) && s_is_category_visible_in_mode(c, mode);
			category_buttons[c]->set_visible(vis);
			category_buttons[c]->set_pressed_no_signal(c == current_category);
		}
	}
}

void ClockGearsDatabaseDialog::_on_chapter_selected(int p_chapter_idx) {
	if (p_chapter_idx < 0 || p_chapter_idx >= chapters.size()) {
		return;
	}
	int mode = ClockGearsDataManager::get_singleton()
			? int(ClockGearsDataManager::get_singleton()->get_system_config().get("combat_mode", current_combat_mode))
			: current_combat_mode;
	current_chapter = p_chapter_idx;
	_update_chapter_and_category_visibility();
	const Vector<int> &cats = chapters[current_chapter].categories;
	int first_vis = -1;
	for (int i = 0; i < cats.size(); i++) {
		if (s_is_category_visible_in_mode(cats[i], mode)) {
			first_vis = cats[i];
			break;
		}
	}
	if ((!cats.has(current_category) || !s_is_category_visible_in_mode(current_category, mode)) && first_vis >= 0) {
		_on_category_selected(first_vis);
	} else {
		_save_editor_ui_state();
	}
}

Dictionary ClockGearsDatabaseDialog::_load_editor_ui_state() const {
	String path = s_get_editor_db_ui_config_path();
	if (FileAccess::exists(path)) {
		Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
		if (f.is_valid()) {
			Variant v = JSON::parse_string(f->get_as_text());
			if (v.get_type() == Variant::DICTIONARY) {
				return Dictionary(v);
			}
		}
	}
	return Dictionary();
}

void ClockGearsDatabaseDialog::_save_editor_ui_state() {
	Dictionary d = _load_editor_ui_state();
	Size2i cur_sz = get_size();
	Size2i min_sz = get_min_size();
	if (is_inside_tree() && cur_sz.x >= min_sz.x && cur_sz.y >= min_sz.y) {
		d["window_w"] = cur_sz.x;
		d["window_h"] = cur_sz.y;
	}
	if (is_inside_tree() && is_visible()) {
		Point2i pos = get_position();
		d["window_x"] = pos.x;
		d["window_y"] = pos.y;
		d["has_pos"] = true;
	}
	d["last_chapter"] = current_chapter;
	d["last_category"] = current_category;

	String path = s_get_editor_db_ui_config_path();
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_string(JSON::stringify(d, "\t"));
	}
}

void ClockGearsDatabaseDialog::_on_category_selected(int p_category) {
	if (p_category < 0 || (p_category >= CAT_MAX && p_category != DB_CAT_MAIN_MENU && p_category != DB_CAT_WINDOW_STYLES && p_category != DB_CAT_GAME_MENU && p_category != DB_CAT_CREDITS && p_category != DB_CAT_HUD && p_category != DB_CAT_UI_DESIGNER && p_category != DB_CAT_GAME_OVER)) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !has_pending_changes) {
		for (int i = 0; i < CAT_MAX; i++) {
			Array dm_arr = dm->get_category_array(i);
			if (!dm_arr.is_empty() || category_data[i].is_empty()) {
				category_data[i] = dm_arr;
			}
		}
	}
	int prev_category = current_category;
	current_category = p_category;
	int mode = ClockGearsDataManager::get_singleton()
			? int(ClockGearsDataManager::get_singleton()->get_system_config().get("combat_mode", current_combat_mode))
			: current_combat_mode;
	if (!s_is_category_visible_in_mode(current_category, mode)) {
		current_category = CAT_ACTORS;
	}
	current_chapter = _find_chapter_for_category(current_category);
	_update_chapter_and_category_visibility();
	_save_editor_ui_state();
	_refresh_category_view();
	if (main_editor_tabs && current_category != prev_category) {
		main_editor_tabs->set_current_tab(0);
	}
}

void ClockGearsDatabaseDialog::_refresh_category_view() {
	updating_ui = true;

	String cat_name = _get_category_name(current_category);
	if (col1_title) {
		col1_title->set_text(cat_name);
	}
	if (col2_title) {
		col2_title->set_text(CG_DB_TR("Datos generales"));
	}

	bool is_system = (current_category == CAT_SYSTEM);
	bool is_main_menu = (current_category == DB_CAT_MAIN_MENU);
	bool is_window_styles = (current_category == DB_CAT_WINDOW_STYLES);
	bool is_game_menu = (current_category == DB_CAT_GAME_MENU);
	bool is_credits = (current_category == DB_CAT_CREDITS);
	bool is_hud = (current_category == DB_CAT_HUD);
	bool is_ui_designer = (current_category == DB_CAT_UI_DESIGNER);
	bool is_game_over = (current_category == DB_CAT_GAME_OVER);
	bool hide_record_col = (is_system || is_main_menu || is_game_menu || is_credits || is_hud || is_ui_designer || is_game_over);
	bool is_tileset = (current_category == CAT_TILESETS);
	bool is_troops = (current_category == CAT_TROOPS);
	bool is_enemy = (current_category == CAT_ENEMIES);
	bool is_arpg_enemy = (current_category == CAT_ARPG_ENEMIES);
	bool is_any_enemy = (is_enemy || is_arpg_enemy);
	bool is_actor = (current_category == CAT_ACTORS);
	bool is_classes = (current_category == CAT_CLASSES);
	bool is_anim_slot_cat = s_is_anim_slot_category(current_category);
	bool is_arpg_equip = s_is_arpg_equip_category(current_category);
	bool is_actor_or_enemy = (is_anim_slot_cat || is_enemy);
	bool has_event_tab = s_has_event_tab_category(current_category);
	bool has_single_graphic = (current_category == CAT_ITEMS || current_category == CAT_WEAPONS ||
			current_category == CAT_ARMORS || current_category == CAT_ACCESSORIES ||
			current_category == CAT_SKILLS || current_category == CAT_STATES ||
			current_category == CAT_CLASSES || current_category == CAT_FLORA ||
			current_category == CAT_FAUNA || current_category == CAT_ANIMATIONS);

	if (ClockGearsDataManager::get_singleton() && !is_system) {
		Dictionary dm_sys = ClockGearsDataManager::get_singleton()->get_system_config();
		if (dm_sys.has("combat_mode")) {
			system_config["combat_mode"] = dm_sys["combat_mode"];
			current_combat_mode = int(dm_sys["combat_mode"]);
		}
		if (dm_sys.has("movement_directions")) {
			system_config["movement_directions"] = dm_sys["movement_directions"];
		}
		if (dm_sys.has("mannequin_size")) {
			system_config["mannequin_size"] = ClockGearsDataManager::normalize_mannequin_size(int(dm_sys["mannequin_size"]));
		}
	}
	if (!s_is_arpg_shape_category(current_category)) {
		anim_submode = 0;
	}
	_update_chapter_and_category_visibility();

	if (col1_box) {
		col1_box->set_visible(!hide_record_col);
	}
	if (btn_change_max) {
		btn_change_max->set_visible(!is_window_styles);
	}
	if (btn_paste_record) {
		btn_paste_record->set_disabled(is_window_styles ? ws_clipboard_style.is_empty() : clipboard_record.is_empty());
	}
	if (btn_remove_record && !is_window_styles) {
		btn_remove_record->set_disabled(false);
	}
	if (general_section_box) {
		general_section_box->set_visible(!hide_record_col && !is_window_styles);
	}
	if (params_grid) {
		params_grid->set_visible(!hide_record_col && !is_window_styles && !is_classes);
	}
	if (system_panel) {
		system_panel->set_visible(is_system);
	}
	if (is_main_menu) {
		_setup_main_menu_editor_ui();
	}
	if (main_menu_editor_panel) {
		main_menu_editor_panel->set_visible(is_main_menu);
		if (is_main_menu) {
			_refresh_main_menu_editor_ui();
		}
	}
	if (is_window_styles) {
		_setup_window_styles_editor_ui();
	}
	if (window_styles_editor_panel) {
		window_styles_editor_panel->set_visible(is_window_styles);
	}
	if (is_game_menu) {
		_setup_game_menu_editor_ui();
	}
	if (game_menu_editor_panel) {
		game_menu_editor_panel->set_visible(is_game_menu);
		if (is_game_menu) {
			_refresh_game_menu_editor_ui();
		}
	}
	if (is_credits) {
		_setup_credits_editor_ui();
	}
	if (credits_editor_panel) {
		credits_editor_panel->set_visible(is_credits);
		if (is_credits) {
			_refresh_credits_editor_ui();
		}
	}
	if (is_hud) {
		_setup_hud_editor_ui();
	}
	if (hud_editor_panel) {
		hud_editor_panel->set_visible(is_hud);
		if (is_hud) {
			_refresh_hud_editor_ui();
		} else {
			_restore_hud_preview_runtime_state();
		}
	}
	if (is_ui_designer) {
		_setup_ui_design_editor_ui();
	}
	if (ui_design_editor_panel) {
		ui_design_editor_panel->set_visible(is_ui_designer);
		if (is_ui_designer) {
			ui_design_editor_panel->refresh_from_data_manager();
			ui_design_editor_panel->adapt_to_width(int(get_size().x));
		}
	}
	if (is_game_over) {
		_setup_game_over_editor_ui();
	}
	if (game_over_editor_panel) {
		game_over_editor_panel->set_visible(is_game_over);
		if (is_game_over) {
			_refresh_game_over_editor_ui();
		}
	}
	if (is_main_menu || is_window_styles || is_game_menu || is_credits || is_hud || is_ui_designer || is_game_over) {
		adapt_to_window_size(get_size());
	}
	if (main_editor_tabs) {
		main_editor_tabs->set_tabs_visible(is_actor_or_enemy);
		main_editor_tabs->set_tab_hidden(0, false);
		main_editor_tabs->set_tab_title(1, is_arpg_equip ? CG_DB_TR("Rasgos") : CG_DB_TR("Combate"));
		main_editor_tabs->set_tab_hidden(1, !(is_any_enemy || is_arpg_equip));
		main_editor_tabs->set_tab_hidden(2, !is_actor_or_enemy);
		if (main_editor_tabs->get_tab_count() > 3) {
			main_editor_tabs->set_tab_hidden(3, !has_event_tab);
		}
		if (main_editor_tabs->get_tab_count() > 4) {
			main_editor_tabs->set_tab_hidden(4, !s_has_body_zones_tab_category(current_category));
		}
		if (main_editor_tabs->get_tab_count() > 5) {
			main_editor_tabs->set_tab_hidden(5, !s_has_movement_tab_category(current_category));
		}
		int cur_tab = main_editor_tabs->get_current_tab();
		if (!is_actor_or_enemy || (cur_tab >= 0 && cur_tab < main_editor_tabs->get_tab_count() && main_editor_tabs->is_tab_hidden(cur_tab))) {
			main_editor_tabs->set_current_tab(0);
		}
	}
	if (stats_section_box && combat_tab_vbox && col2_box) {
		s_reparent_control(stats_section_box, is_arpg_equip ? (Control *)combat_tab_vbox : (Control *)col2_box);
		if (is_arpg_equip) {
			combat_tab_vbox->move_child(stats_section_box, 0);
		} else {
			int g_idx = general_section_box && general_section_box->get_parent() == col2_box ? general_section_box->get_index() : -1;
			col2_box->move_child(stats_section_box, g_idx >= 0 ? g_idx + 1 : 0);
		}
	}
	if (enemy_drops_box && col2_box) {
		if (enemy_drops_box->get_parent() == col2_box) {
			int st_idx = (stats_section_box && stats_section_box->get_parent() == col2_box) ? stats_section_box->get_index() : 0;
			col2_box->move_child(enemy_drops_box, st_idx + 1);
		}
	}
	if (speeds_section_box && col2_box) {
		if (speeds_section_box->get_parent() == col2_box) {
			int after_idx = general_section_box && general_section_box->get_parent() == col2_box ? general_section_box->get_index() : 0;
			if (is_any_enemy && enemy_drops_box && enemy_drops_box->get_parent() == col2_box) {
				after_idx = enemy_drops_box->get_index();
			} else if (is_actor && stats_section_box && stats_section_box->get_parent() == col2_box) {
				after_idx = stats_section_box->get_index();
			}
			col2_box->move_child(speeds_section_box, after_idx + 1);
		}
	}
	if (traits_box && combat_tab_vbox && col2_box) {
		s_reparent_control(traits_box, (is_enemy || is_arpg_equip) ? (Control *)combat_tab_vbox : (Control *)col2_box);
	}
	bool has_anim_records = is_actor_or_enemy && !category_data[current_category].is_empty();
	if (sprite_anim_editor_box) {
		sprite_anim_editor_box->set_visible(has_anim_records);
	}
	if (anim_empty_state_label) {
		anim_empty_state_label->set_visible(is_actor_or_enemy && !has_anim_records);
	}
	if (troop_members_box) {
		troop_members_box->set_visible(is_troops);
	}
	if (troop_placement_col3_box) {
		troop_placement_col3_box->set_visible(is_troops);
	}
	if (standard_graphic_box) {
		standard_graphic_box->set_visible(has_single_graphic);
	}
	if (general_section_box && col2_box && tileset_left_col) {
		s_reparent_control(general_section_box, is_tileset ? (Control *)tileset_left_col : (Control *)col2_box);
		Control *gen_parent = general_section_box->get_parent_control();
		if (gen_parent) {
			gen_parent->move_child(general_section_box, 0);
		}
	}
	if (params_grid) {
		params_grid->set_columns(is_tileset ? 2 : 4);
		params_grid->add_theme_constant_override("h_separation", int((is_tileset ? 8 : 12) * EDSCALE));
		const float row_h_min = 32.0f * EDSCALE;
		if (name_edit) {
			name_edit->set_custom_minimum_size(Size2((is_tileset ? 115.0f : 150.0f) * EDSCALE, row_h_min));
		}
		if (desc_edit) {
			desc_edit->set_custom_minimum_size(Size2((is_tileset ? 115.0f : 150.0f) * EDSCALE, row_h_min));
		}
		if (type_option) {
			type_option->set_fit_to_longest_item(!is_tileset);
			type_option->set_custom_minimum_size(Size2((is_tileset ? 208.0f : 140.0f) * EDSCALE, row_h_min));
		}
	}
	if (tileset_main_hbox) {
		tileset_main_hbox->set_visible(is_tileset);
	}
	if (tileset_left_col) {
		tileset_left_col->set_visible(is_tileset);
	}
	if (tileset_slots_box) {
		tileset_slots_box->set_visible(is_tileset);
	}
	if (tileset_collision_panel) {
		tileset_collision_panel->set_visible(is_tileset);
	}
	if (tileset_collision_box) {
		tileset_collision_box->set_visible(is_tileset);
	}
	if (tab0_margin_container) {
		int m_lr = is_tileset ? int(8 * EDSCALE) : int(12 * EDSCALE);
		int m_tb = is_tileset ? 0 : int(12 * EDSCALE);
		tab0_margin_container->add_theme_constant_override("margin_left", m_lr);
		tab0_margin_container->add_theme_constant_override("margin_top", m_tb);
		tab0_margin_container->add_theme_constant_override("margin_right", m_lr);
		tab0_margin_container->add_theme_constant_override("margin_bottom", m_tb);
	}
	if (col2_scroll_container) {
		col2_scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		col2_scroll_container->set_vertical_scroll_mode(is_tileset ? ScrollContainer::SCROLL_MODE_DISABLED : ScrollContainer::SCROLL_MODE_AUTO);
	}
	if (is_tileset) {
		callable_mp(this, &ClockGearsDatabaseDialog::_update_tileset_split_layout).call_deferred();
	}
	if (enemy_element_rates_box) {
		enemy_element_rates_box->set_visible(is_any_enemy);
	}
	if (enemy_drops_box) {
		enemy_drops_box->set_visible(is_any_enemy);
	}
	if (enemy_actions_box) {
		enemy_actions_box->set_visible(is_enemy);
	}
	if (arpg_ai_box) {
		arpg_ai_box->set_visible(is_arpg_enemy);
	}
	if (arpg_attack_box) {
		arpg_attack_box->set_visible(is_arpg_enemy);
	}
	if (enemy_casts_box) {
		enemy_casts_box->set_visible(is_enemy);
	}
	if (enemy_others_box) {
		enemy_others_box->set_visible(is_enemy);
	}
	if (combat_anims_box) {
		combat_anims_box->set_visible(is_actor);
	}
	if (actor_dir_idle_chk) {
		actor_dir_idle_chk->set_visible(is_anim_slot_cat);
	}
	if (anim_behind_body_chk) {
		anim_behind_body_chk->set_visible(is_arpg_equip);
	}
	if (arpg_piece_mode_bar) {
		arpg_piece_mode_bar->set_visible(is_arpg_equip);
	}
	if (arpg_preview_box) {
		arpg_preview_box->set_visible(is_arpg_equip);
	}
	_refresh_arpg_piece_anim_mode_ui();
	if (btn_add_anim) {
		btn_add_anim->set_text(is_enemy ? CG_DB_TR("Nueva") : CG_DB_TR("Nueva"));
		btn_add_anim->set_tooltip_text(is_enemy ? CG_DB_TR("Añadir una nueva ranura de Lanzamiento") : CG_DB_TR("Añadir una nueva animación vacía"));
	}

	if (is_system) {
		if (stats_section_box) stats_section_box->set_visible(false);
		if (exp_section_box) exp_section_box->set_visible(false);
		if (invocation_section_box) invocation_section_box->set_visible(false);
		if (traits_box) traits_box->set_visible(false);
		if (effects_box) effects_box->set_visible(false);
		if (ClockGearsDataManager::get_singleton()) {
			system_config = ClockGearsDataManager::get_singleton()->get_system_config();
		}
		if (sys_game_title_edit) {
			sys_game_title_edit->set_text(system_config.get("game_title", ""));
		}
		if (sys_currency_edit) {
			sys_currency_edit->set_text(system_config.get("currency", "G"));
		}
		if (combat_mode_selector) {
			combat_mode_selector->select((int)system_config.get("combat_mode", COMBAT_MODE_TURN_BASED));
		}
		if (sys_arpg_move_atk_chk) {
			bool m_atk = system_config.has("arpg_move_while_attacking") ? bool(system_config["arpg_move_while_attacking"]) : true;
			sys_arpg_move_atk_chk->set_pressed(m_atk);
		}
		if (sys_arpg_atk_spd_spin) {
			int spd_atk = CLAMP(int(system_config.get("arpg_attack_move_speed_pct", 100)), 0, 200);
			sys_arpg_atk_spd_spin->set_value(spd_atk);
			sys_arpg_atk_spd_spin->set_meta("cg_last_valid_val", double(spd_atk));
		}
		if (sys_arpg_move_blk_chk) {
			bool m_blk = system_config.has("arpg_move_while_blocking") ? bool(system_config["arpg_move_while_blocking"]) : false;
			sys_arpg_move_blk_chk->set_pressed(m_blk);
		}
		if (sys_arpg_blk_spd_spin) {
			int spd_blk = CLAMP(int(system_config.get("arpg_block_move_speed_pct", 50)), 0, 200);
			sys_arpg_blk_spd_spin->set_value(spd_blk);
			sys_arpg_blk_spd_spin->set_meta("cg_last_valid_val", double(spd_blk));
		}
		if (sys_arpg_invuln_spin && sys_arpg_invuln_slider) {
			double inv = CLAMP(double(system_config.get("arpg_invuln_seconds", 0.5)), 0.0, 3.0);
			sys_arpg_invuln_spin->set_value(inv);
			sys_arpg_invuln_spin->set_meta("cg_last_valid_val", inv);
			sys_arpg_invuln_slider->set_value(inv);
		}
		if (sys_arpg_block_red_spin && sys_arpg_block_red_slider) {
			double red = CLAMP(double(system_config.get("arpg_block_reduction_pct", 50.0)), 0.0, 100.0);
			sys_arpg_block_red_spin->set_value(red);
			sys_arpg_block_red_spin->set_meta("cg_last_valid_val", red);
			sys_arpg_block_red_slider->set_value(red);
		}
		if (sys_arpg_dodge_type_opt) {
			String dt = String(system_config.get("arpg_dodge_type", "voltereta")).to_lower();
			sys_arpg_dodge_type_opt->select(dt == "dash" ? 1 : 0);
		}
		if (sys_arpg_dodge_dur_spin && sys_arpg_dodge_dur_slider) {
			double dd = CLAMP(double(system_config.get("arpg_dodge_duration", 0.25)), 0.05, 2.0);
			sys_arpg_dodge_dur_spin->set_value(dd);
			sys_arpg_dodge_dur_spin->set_meta("cg_last_valid_val", dd);
			sys_arpg_dodge_dur_slider->set_value(dd);
		}
		if (sys_arpg_dodge_dist_spin && sys_arpg_dodge_dist_slider) {
			double ds = CLAMP(double(system_config.get("arpg_dodge_distance", 96.0)), 16.0, 320.0);
			sys_arpg_dodge_dist_spin->set_value(ds);
			sys_arpg_dodge_dist_spin->set_meta("cg_last_valid_val", ds);
			sys_arpg_dodge_dist_slider->set_value(ds);
		}
		if (sys_arpg_dodge_cd_spin && sys_arpg_dodge_cd_slider) {
			double dc = CLAMP(double(system_config.get("arpg_dodge_cooldown", 0.4)), 0.0, 5.0);
			sys_arpg_dodge_cd_spin->set_value(dc);
			sys_arpg_dodge_cd_spin->set_meta("cg_last_valid_val", dc);
			sys_arpg_dodge_cd_slider->set_value(dc);
		}
		if (sys_arpg_show_dmg_chk) {
			bool show_dmg = bool(system_config.get("arpg_show_damage_numbers", false));
			system_config["arpg_show_damage_numbers"] = show_dmg;
			sys_arpg_show_dmg_chk->set_pressed(show_dmg);
		}
		_update_system_arpg_controls_visibility();
		if (sys_movement_dirs_opt) {
			int dirs = (int(system_config.get("movement_directions", 4)) == 8) ? 8 : 4;
			sys_movement_dirs_opt->select(dirs == 8 ? 1 : 0);
		}
		if (sys_mannequin_size_opt) {
			int msz = ClockGearsDataManager::normalize_mannequin_size(int(system_config.get("mannequin_size", 32)));
			int m_idx = (msz == 64) ? 2 : (msz == 48 ? 1 : 0);
			sys_mannequin_size_opt->select(m_idx);
		}
		if (sys_start_x_spin) {
			sys_start_x_spin->set_value((int)system_config.get("start_x", 0));
		}
		if (sys_start_y_spin) {
			sys_start_y_spin->set_value((int)system_config.get("start_y", 0));
		}
		if (sys_camera_zoom_opt) {
			int cz = int(system_config.get("camera_zoom", 2));
			int idx = (cz >= 1 && cz <= 4) ? cz - 1 : 1;
			sys_camera_zoom_opt->select(idx);
		}
		if (sys_camera_smooth_check) {
			sys_camera_smooth_check->set_pressed(bool(system_config.get("camera_smooth", true)));
		}
		if (sys_camera_clamp_check) {
			sys_camera_clamp_check->set_pressed(bool(system_config.get("camera_clamp", true)));
		}
		_refresh_system_maps_and_party();
		updating_ui = false;
		return;
	}

	if (is_main_menu) {
		if (stats_section_box) stats_section_box->set_visible(false);
		if (exp_section_box) exp_section_box->set_visible(false);
		if (invocation_section_box) invocation_section_box->set_visible(false);
		if (traits_box) traits_box->set_visible(false);
		if (effects_box) effects_box->set_visible(false);
		if (speeds_section_box) speeds_section_box->set_visible(false);
		if (col3_box) col3_box->set_visible(false);
		_refresh_main_menu_editor_ui();
		updating_ui = false;
		return;
	}

	// Configurar desplegables por categoría usando EXCLUSIVAMENTE catálogos del sistema
	// (ClockGearsSystemCatalog) o registros creados por el usuario en la DB (Regla Cero).
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	PackedStringArray sys_elements = cat ? cat->get_elements() : PackedStringArray();
	PackedStringArray sys_weapons = cat ? cat->get_weapon_types() : PackedStringArray();
	PackedStringArray sys_armors = cat ? cat->get_armor_types() : PackedStringArray();
	PackedStringArray sys_slots = cat ? cat->get_equip_slots() : PackedStringArray();
	PackedStringArray sys_scopes = cat ? cat->get_scopes() : PackedStringArray();
	PackedStringArray sys_occasions = cat ? cat->get_occasions() : PackedStringArray();

	type_option->clear();
	secondary_option->clear();
	tertiary_option->clear();

	bool show_primary_type = true;
	bool show_secondary_type = false;
	bool show_tertiary_type = false;

	switch (current_category) {
		case CAT_ACTORS: {
			// Retoque 1: Actores tiene selector "Clase" que lista SOLO las clases creadas
			// por el usuario en la base de datos. Si no hay ninguna, muestra "(ninguna)".
			type_label->set_text(CG_DB_TR("Clase:"));
			_populate_db_records_option(type_option, CAT_CLASSES, CG_DB_TR("(ninguna)"), true);
		} break;

		case CAT_CLASSES: {
			// Retoque 2: Eliminada "Curva de Progresión" con opciones fijas.
			// Reemplazada por los 4 campos numéricos de la fórmula de EXP (base, extra, acc_a, acc_b).
			show_primary_type = false;
		} break;

		case CAT_SKILLS: {
			type_label->set_text(CG_DB_TR("Ámbito (Scope):"));
			_populate_catalog_option(type_option, sys_scopes);

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Elemento:"));
			_populate_catalog_option(secondary_option, sys_elements);

			show_tertiary_type = true;
			tertiary_label->set_text(CG_DB_TR("Ocasión:"));
			_populate_catalog_option(tertiary_option, sys_occasions);
		} break;

		case CAT_ITEMS: {
			// Sección 5.2: Tipo de objeto del sistema + Ámbito + Ocasión desde ClockGearsSystemCatalog
			type_label->set_text(CG_DB_TR("Tipo de Objeto:"));
			type_option->add_item(CG_DB_TR("Objeto Regular"), 0);
			type_option->add_item(CG_DB_TR("Objeto Clave"), 1);
			type_option->add_item(CG_DB_TR("Objeto Oculto A"), 2);
			type_option->add_item(CG_DB_TR("Objeto Oculto B"), 3);

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Ámbito (Scope):"));
			_populate_catalog_option(secondary_option, sys_scopes);

			show_tertiary_type = true;
			tertiary_label->set_text(CG_DB_TR("Ocasión:"));
			_populate_catalog_option(tertiary_option, sys_occasions);
		} break;

		case CAT_WEAPONS: {
			// Sección 5.1: Tipo de Arma, Ranura y Elemento desde ClockGearsSystemCatalog
			type_label->set_text(CG_DB_TR("Tipo de Arma:"));
			_populate_catalog_option(type_option, sys_weapons);

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Ranura de Equipo:"));
			_populate_catalog_option(secondary_option, sys_slots);

			show_tertiary_type = true;
			tertiary_label->set_text(CG_DB_TR("Elemento de Ataque:"));
			_populate_catalog_option(tertiary_option, sys_elements);
		} break;

		case CAT_ARMORS: {
			// Sección 5.1: Tipo de Armadura y Ranura desde ClockGearsSystemCatalog
			type_label->set_text(CG_DB_TR("Tipo de Armadura:"));
			_populate_catalog_option(type_option, sys_armors);

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Ranura de Equipo:"));
			_populate_catalog_option(secondary_option, sys_slots);
		} break;

		case CAT_ACCESSORIES: {
			// Sección 5.1: Accesorio sin tipo de arma/armadura; solo ranura desde ClockGearsSystemCatalog
			type_label->set_text(CG_DB_TR("Ranura de Equipo:"));
			_populate_catalog_option(type_option, sys_slots);
		} break;

		case CAT_ENEMIES: {
			// Sección 5.3: Enemigos usa sus paneles dedicados de 3 Drops y Patrones de Acción (Tree)
			show_primary_type = false;
			show_secondary_type = false;
			show_tertiary_type = false;
		} break;

		case CAT_TROOPS: {
			show_primary_type = false;
			show_secondary_type = false;
			show_tertiary_type = false;
		} break;

		case CAT_STATES: {
			// Sección 1.2 y 9.10: Catálogo del sistema de restricción y expiración de estado
			type_label->set_text(CG_DB_TR("Restricción de Estado:"));
			type_option->add_item(CG_DB_TR("Ninguna"), 0);
			type_option->add_item(CG_DB_TR("Atacar a un Enemigo"), 1);
			type_option->add_item(CG_DB_TR("Atacar a Cualquiera"), 2);
			type_option->add_item(CG_DB_TR("Atacar a un Aliado"), 3);
			type_option->add_item(CG_DB_TR("No puede Actuar"), 4);

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Modo de Expiración:"));
			secondary_option->add_item(CG_DB_TR("Sin expiración automática"), 0);
			secondary_option->add_item(CG_DB_TR("Al final de la acción"), 1);
			secondary_option->add_item(CG_DB_TR("Al final del turno"), 2);
			secondary_option->add_item(CG_DB_TR("Por pasos en el mapa"), 3);
			secondary_option->add_item(CG_DB_TR("Por daño recibido"), 4);
		} break;

		case CAT_FLORA: {
			// Sección 10.1: Categoría de Flora del sistema + selectores de objetos de la DB
			type_label->set_text(CG_DB_TR("Categoría de Flora:"));
			const char *flora_cats[] = { "Árbol", "Arbusto", "Flor", "Hongo", "Cultivo", "Hierba", "Planta Acuática", "Personalizada" };
			for (size_t i = 0; i < sizeof(flora_cats) / sizeof(flora_cats[0]); i++) {
				type_option->add_item(CG_DB_TR(flora_cats[i]), int(i));
			}

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Herramienta Requerida:"));
			_populate_db_records_option(secondary_option, CAT_ITEMS, CG_DB_TR("(ninguna)"), false);

			show_tertiary_type = true;
			tertiary_label->set_text(CG_DB_TR("Recurso Recolectado:"));
			_populate_db_records_option(tertiary_option, CAT_ITEMS, CG_DB_TR("(ninguno)"), false);
		} break;

		case CAT_FAUNA: {
			// Sección 10.2: Categoría de Fauna + Comportamiento (máquina de estados) + Enemigo de la DB
			type_label->set_text(CG_DB_TR("Categoría de Fauna:"));
			const char *fauna_cats[] = { "Mamífero", "Ave", "Reptil", "Pez", "Insecto", "Criatura Mágica", "Personalizada" };
			for (size_t i = 0; i < sizeof(fauna_cats) / sizeof(fauna_cats[0]); i++) {
				type_option->add_item(CG_DB_TR(fauna_cats[i]), int(i));
			}

			show_secondary_type = true;
			secondary_label->set_text(CG_DB_TR("Comportamiento (IA):"));
			const char *fauna_beh[] = { "Pasivo", "Huidizo", "Neutral", "Agresivo", "Territorial", "Manada" };
			for (size_t i = 0; i < sizeof(fauna_beh) / sizeof(fauna_beh[0]); i++) {
				secondary_option->add_item(CG_DB_TR(fauna_beh[i]), int(i));
			}

			show_tertiary_type = true;
			tertiary_label->set_text(CG_DB_TR("Enemigo Vinculado:"));
			_populate_db_records_option(tertiary_option, CAT_ENEMIES, CG_DB_TR("(ninguno)"), false);
		} break;

		case CAT_ANIMATIONS: {
			type_label->set_text(CG_DB_TR("Posición de Alineación:"));
			type_option->add_item(CG_DB_TR("Centro del Objetivo"), 0);
			type_option->add_item(CG_DB_TR("Cabeza del Objetivo"), 1);
			type_option->add_item(CG_DB_TR("Base / Pies del Objetivo"), 2);
			type_option->add_item(CG_DB_TR("Pantalla Completa"), 3);
		} break;

		case CAT_TILESETS: {
			type_label->set_text(CG_DB_TR("Modo de Tileset:"));
			type_option->add_item(CG_DB_TR("Área / Interior (32x32)"), 0);
			type_option->add_item(CG_DB_TR("Mapamundi (Overworld)"), 1);
		} break;

		case CAT_COMMON_EVENTS: {
			type_label->set_text(CG_DB_TR("Disparador (Trigger):"));
			type_option->add_item(CG_DB_TR("Ninguno (Llamada Manual)"), 0);
			type_option->add_item(CG_DB_TR("Ejecución Automática (Autorun)"), 1);
			type_option->add_item(CG_DB_TR("Proceso Paralelo"), 2);
		} break;

		case CAT_TYPES: {
			// Sección 1.2: Catálogo del sistema editable en la pestaña "Tipos"
			type_label->set_text(CG_DB_TR("Catálogo del Sistema:"));
			type_option->add_item(CG_DB_TR("Elementos"), 0);
			type_option->add_item(CG_DB_TR("Tipos de Arma"), 1);
			type_option->add_item(CG_DB_TR("Tipos de Armadura"), 2);
			type_option->add_item(CG_DB_TR("Ranuras de Equipo"), 3);
		} break;

		default: {
			// CAT_VOCABULARY y CAT_SWITCHES_VARS no llevan desplegable inventado
			show_primary_type = false;
		} break;
	}

	type_label->set_visible(show_primary_type);
	type_option->set_visible(show_primary_type);
	secondary_label->set_visible(show_secondary_type);
	secondary_option->set_visible(show_secondary_type);
	tertiary_label->set_visible(show_tertiary_type);
	tertiary_option->set_visible(show_tertiary_type);

	// Mostrar los 4 SpinBox numéricos de la fórmula de EXP (Sección 9.3) solo en CAT_CLASSES
	if (exp_section_box) {
		exp_section_box->set_visible(is_classes);
	}
	for (int i = 0; i < 4; i++) {
		if (exp_labels[i]) {
			exp_labels[i]->set_visible(is_classes);
		}
		if (exp_spins[i]) {
			exp_spins[i]->set_visible(is_classes);
		}
	}
	if (exp_preview_title) {
		exp_preview_title->set_visible(is_classes);
	}
	if (exp_preview_label) {
		exp_preview_label->set_visible(is_classes);
	}

	bool is_equip = (current_category == CAT_WEAPONS || current_category == CAT_ARMORS || current_category == CAT_ACCESSORIES);
	bool is_item = (current_category == CAT_ITEMS);
	bool is_skill = (current_category == CAT_SKILLS);

	// Sección 5.1 y 5.2: Precio y Precio de Venta
	bool show_price = (is_equip || is_item);
	if (price_label) {
		price_label->set_visible(show_price);
	}
	if (price_spin) {
		price_spin->set_visible(show_price);
	}
	if (sell_price_title) {
		sell_price_title->set_visible(show_price);
	}
	if (sell_price_label) {
		sell_price_label->set_visible(show_price);
	}

	// Sección 5.1: Animación de Efecto en Armas, Habilidades y Objetos
	bool show_atk_anim = (current_category == CAT_WEAPONS || current_category == CAT_SKILLS || current_category == CAT_ITEMS);
	if (attack_anim_label) {
		attack_anim_label->set_visible(show_atk_anim);
		attack_anim_label->set_text(CG_DB_TR("Animación de efecto:"));
	}
	if (attack_anim_option) {
		attack_anim_option->set_visible(show_atk_anim);
		if (show_atk_anim) {
			_populate_db_records_option(attack_anim_option, CAT_ANIMATIONS, CG_DB_TR("(ninguna)"), false);
		}
	}

	// Sección 5.2: Campos de Objeto y Habilidad (Consumible, Velocidad, Éxito, Repeticiones, TP Gain, Tipo Golpe, Elemento, Varianza, Crítico)
	if (consumable_label) {
		consumable_label->set_visible(is_item);
	}
	if (consumable_check) {
		consumable_check->set_visible(is_item);
	}
	bool show_invocation = (is_item || is_skill);
	if (item_speed_label) {
		item_speed_label->set_visible(show_invocation);
	}
	if (item_speed_spin) {
		item_speed_spin->set_visible(show_invocation);
	}
	if (item_success_label) {
		item_success_label->set_visible(show_invocation);
	}
	if (item_success_spin) {
		item_success_spin->set_visible(show_invocation);
	}
	if (item_repeats_label) {
		item_repeats_label->set_visible(show_invocation);
	}
	if (item_repeats_spin) {
		item_repeats_spin->set_visible(show_invocation);
	}
	if (item_tp_gain_label) {
		item_tp_gain_label->set_visible(show_invocation);
	}
	if (item_tp_gain_spin) {
		item_tp_gain_spin->set_visible(show_invocation);
	}
	if (hit_type_label) {
		hit_type_label->set_visible(show_invocation);
	}
	if (hit_type_option) {
		hit_type_option->set_visible(show_invocation);
	}
	if (item_element_label) {
		item_element_label->set_visible(is_item);
	}
	if (item_element_option) {
		item_element_option->set_visible(is_item);
		if (is_item) {
			_populate_catalog_option(item_element_option, sys_elements);
		}
	}
	if (variance_label) {
		variance_label->set_visible(show_invocation);
	}
	if (variance_spin) {
		variance_spin->set_visible(show_invocation);
	}
	if (critical_label) {
		critical_label->set_visible(show_invocation);
	}
	if (critical_check) {
		critical_check->set_visible(show_invocation);
	}

	// Sección 5.3 / ARPG 3A: Enemigos (Nivel, EXP, Oro, Debilidades y Resistencias, 3 Drops, Patrones de Acción / IA ARPG)
	if (enemy_level_label) {
		enemy_level_label->set_visible(is_any_enemy);
	}
	if (enemy_level_spin) {
		enemy_level_spin->set_visible(is_any_enemy);
	}
	if (enemy_exp_label) {
		enemy_exp_label->set_visible(is_any_enemy);
	}
	if (enemy_exp_spin) {
		enemy_exp_spin->set_visible(is_any_enemy);
	}
	if (enemy_gold_label) {
		enemy_gold_label->set_visible(is_any_enemy);
	}
	if (enemy_gold_spin) {
		enemy_gold_spin->set_visible(is_any_enemy);
	}
	if (battler_frames_label) {
		battler_frames_label->set_visible(false);
	}
	if (battler_frames_spin) {
		battler_frames_spin->set_visible(false);
	}
	if (battler_fps_label) {
		battler_fps_label->set_visible(false);
	}
	if (battler_fps_spin) {
		battler_fps_spin->set_visible(false);
	}
	if (enemy_element_rates_box) {
		enemy_element_rates_box->set_visible(is_any_enemy);
	}
	if (enemy_drops_box) {
		enemy_drops_box->set_visible(is_any_enemy);
	}
	if (enemy_actions_box) {
		enemy_actions_box->set_visible(is_enemy);
	}
	if (arpg_ai_box) {
		arpg_ai_box->set_visible(is_arpg_enemy);
	}
	if (arpg_attack_box) {
		arpg_attack_box->set_visible(is_arpg_enemy);
	}

	bool is_arpg_equip_cur = s_is_arpg_equip_category(current_category);
	bool show_stats = (current_category == CAT_ACTORS || current_category == CAT_CLASSES ||
			is_equip || is_arpg_equip_cur || is_any_enemy);
	if (stats_section_box) {
		stats_section_box->set_visible(show_stats);
	}
	bool allow_neg_stats = (is_equip || is_arpg_equip_cur);
	for (int i = 0; i < 8; i++) {
		bool show_stat_i = show_stats && !(is_arpg_enemy && (i == 1 || i == 6));
		stat_labels[i]->set_visible(show_stat_i);
		stat_spins[i]->set_visible(show_stat_i);
		// Sección 5.1: Equipamiento permite modificadores negativos (-999..9999)
		if (is_arpg_enemy && i == 0) {
			stat_spins[i]->set_min(1);
		} else {
			stat_spins[i]->set_min(allow_neg_stats ? -999 : 0);
		}
		stat_spins[i]->set_max(allow_neg_stats ? 9999 : 999999);
	}

	bool show_speeds = (current_category == CAT_ACTORS || current_category == CAT_NPCS ||
			current_category == CAT_ARPG_NPCS || is_arpg_enemy);
	bool show_attack_speed = (current_category == CAT_ARPG_NPCS || is_arpg_enemy ||
			(current_category == CAT_ACTORS && current_combat_mode == COMBAT_MODE_ACTION_RPG));
	if (speeds_section_box) {
		speeds_section_box->set_visible(show_speeds);
	}
	if (speed_attack_label) {
		speed_attack_label->set_visible(show_attack_speed);
	}
	if (speed_attack_spin) {
		speed_attack_spin->set_visible(show_attack_speed);
	}
	if (speeds_help_label) {
		speeds_help_label->set_visible(show_speeds);
	}

	bool show_formula = (is_skill || is_item || current_category == CAT_COMMON_EVENTS);
	if (invocation_section_box) {
		invocation_section_box->set_visible(show_invocation || show_formula);
	}
	formula_label->set_visible(show_formula);
	formula_edit->set_visible(show_formula);
	if (current_category == CAT_COMMON_EVENTS) {
		formula_label->set_text(CG_DB_TR("Interruptor Condición:"));
	} else {
		formula_label->set_text(CG_DB_TR("Fórmula de Daño:"));
	}

	// Mostrar las 9 ranuras de imagen (A1..A5, B, C, D, E) en Columna 2 cuando estamos en CAT_TILESETS
	if (tileset_slots_box) {
		tileset_slots_box->set_visible(is_tileset);
	}

	// Sección 4.2 / 4.3 / 5.1 / 5.2 / 5.3: Mostrar editor de Rasgos (Traits) o Efectos
	bool show_traits = (current_category == CAT_ACTORS || current_category == CAT_CLASSES ||
			is_equip || is_arpg_equip_cur || is_enemy || current_category == CAT_STATES);
	if (traits_box) {
		traits_box->set_visible(show_traits);
	}
	if (effects_box) {
		effects_box->set_visible(show_invocation);
	}

	_refresh_record_list();
	updating_ui = false;

	if (is_window_styles) {
		int target_ws = 0;
		if (record_list && record_list->get_item_count() > 0) {
			target_ws = CLAMP(ws_selected_index, 0, record_list->get_item_count() - 1);
			record_list->select(target_ws);
			_on_record_selected(target_ws);
		} else {
			_on_record_selected(-1);
		}
	} else if (record_list->get_item_count() > 0) {
		record_list->select(0);
		_on_record_selected(0);
	} else {
		_on_record_selected(-1);
	}
}

void ClockGearsDatabaseDialog::_refresh_record_list() {
	if (!record_list || current_category < 0 || (current_category >= CAT_MAX && current_category != DB_CAT_WINDOW_STYLES)) {
		return;
	}
	int prev_sel = -1;
	Vector<int> sel = record_list->get_selected_items();
	if (!sel.is_empty()) {
		prev_sel = sel[0];
	}

	record_list->clear();
	const int icon_scale = ClockGearsIcons::get_editor_icon_scale(this);
	const int icon_px = 32 * icon_scale;
	record_list->set_fixed_icon_size(Size2i(icon_px, icon_px));
	record_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	Ref<Texture2D> cat_icon;
	if (ClockGearsIcons::get_singleton()) {
		cat_icon = ClockGearsIcons::get_singleton()->get_db_category_icon(current_category, float(icon_scale));
	}
	if (current_category == DB_CAT_WINDOW_STYLES) {
		window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
		Array styles = window_styles_config_draft.get("styles", Array());
		for (int i = 0; i < styles.size(); i++) {
			Dictionary st = styles[i];
			String nm = String(st.get("name", "Estilo"));
			bool is_bi = bool(st.get("built_in", false));
			String label = is_bi ? vformat("%03d: %s (incluido)", i + 1, nm) : vformat("%03d: %s", i + 1, nm);
			record_list->add_item(label, cat_icon);
		}
		if (prev_sel >= 0 && prev_sel < record_list->get_item_count()) {
			record_list->select(prev_sel);
		}
		return;
	}
	const Array &arr = category_data[current_category];
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		int id = d.get("id", i + 1);
		String nm = d.get("name", "");
		Ref<Texture2D> row_icon = _build_record_list_icon_texture(current_category, d, icon_px, cat_icon);
		record_list->add_item(vformat("%03d: %s", id, nm), row_icon);
	}

	if (prev_sel >= 0 && prev_sel < record_list->get_item_count()) {
		record_list->select(prev_sel);
	}
}

void ClockGearsDatabaseDialog::_on_record_selected(int p_index) {
	if (current_category == DB_CAT_WINDOW_STYLES) {
		Array styles = window_styles_config_draft.get("styles", Array());
		if (styles.is_empty()) {
			ws_selected_index = -1;
		} else {
			ws_selected_index = CLAMP(p_index, 0, styles.size() - 1);
		}
		_refresh_window_styles_editor_ui();
		return;
	}
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	updating_ui = true;
	troop_member_dragging = false;
	troop_hp_bar_dragging = false;
	Array &arr = category_data[current_category];
	bool valid = (p_index >= 0 && p_index < arr.size());

	// Bug 4a: Mantener todos los controles de edición habilitados (editable = true)
	// para que el usuario siempre pueda escribir tras pulsar '+' o directamente
	// al hacer clic en un campo (lo cual auto-crea el registro 001 si la lista está vacía).
	name_edit->set_editable(true);
	desc_edit->set_editable(true);
	type_option->set_disabled(false);
	secondary_option->set_disabled(false);
	tertiary_option->set_disabled(false);
	formula_edit->set_editable(true);
	for (int i = 0; i < 4; i++) {
		if (exp_spins[i]) {
			exp_spins[i]->set_editable(true);
			if (exp_spins[i]->get_line_edit()) {
				exp_spins[i]->get_line_edit()->set_editable(true);
			}
		}
	}
	for (int i = 0; i < 8; i++) {
		stat_spins[i]->set_editable(true);
		if (stat_spins[i]->get_line_edit()) {
			stat_spins[i]->get_line_edit()->set_editable(true);
		}
	}
	if (btn_select_graphic) {
		btn_select_graphic->set_disabled(!valid);
	}
	if (btn_pick_tile_graphic) {
		btn_pick_tile_graphic->set_disabled(!valid);
		btn_pick_tile_graphic->set_visible(current_category == CAT_FLORA || current_category == CAT_FAUNA || current_category == CAT_ITEMS);
	}
	// PROBLEMA B: El botón "Cargar Hoja de Tileset..." SIEMPRE está habilitado; si no hay registro, auto-crea el 001 al cargar.
	if (btn_load_tileset_sheet) {
		btn_load_tileset_sheet->set_disabled(false);
	}
	if (btn_remove_record) {
		btn_remove_record->set_disabled(!valid);
	}
	if (btn_copy_record) {
		btn_copy_record->set_disabled(!valid);
	}
	if (btn_paste_record) {
		btn_paste_record->set_disabled(clipboard_record.is_empty());
	}

	const int exp_defaults[4] = { 30, 20, 30, 30 };

	if (!valid) {
		name_edit->set_text("");
		desc_edit->set_text("");
		if (type_option->get_item_count() > 0) {
			type_option->select(0);
		}
		if (secondary_option->get_item_count() > 0) {
			secondary_option->select(0);
		}
		if (tertiary_option->get_item_count() > 0) {
			tertiary_option->select(0);
		}
		formula_edit->set_text("max(a.atk * 4 - b.def * 2, 0)");
		for (int i = 0; i < 4; i++) {
			if (exp_spins[i]) {
				exp_spins[i]->set_value(exp_defaults[i]);
				exp_spins[i]->set_meta("cg_last_valid_val", double(exp_defaults[i]));
			}
		}
		_update_exp_preview();
		if (price_spin) {
			price_spin->set_value(0);
		}
		_update_sell_price_label(0);
		if (attack_anim_option && attack_anim_option->get_item_count() > 0) {
			attack_anim_option->select(0);
		}
		if (consumable_check) {
			consumable_check->set_pressed(true);
		}
		if (item_speed_spin) {
			item_speed_spin->set_value(0);
		}
		if (item_success_spin) {
			item_success_spin->set_value(100);
		}
		if (item_repeats_spin) {
			item_repeats_spin->set_value(1);
		}
		if (item_tp_gain_spin) {
			item_tp_gain_spin->set_value(0);
		}
		if (hit_type_option && hit_type_option->get_item_count() > 0) {
			hit_type_option->select(0);
		}
		if (item_element_option && item_element_option->get_item_count() > 0) {
			item_element_option->select(0);
		}
		if (variance_spin) {
			variance_spin->set_value(20);
		}
		if (critical_check) {
			critical_check->set_pressed(false);
		}
		if (enemy_level_spin) {
			enemy_level_spin->set_value(1);
		}
		if (enemy_exp_spin) {
			enemy_exp_spin->set_value(current_category == CAT_ARPG_ENEMIES ? 10 : 0);
		}
		if (enemy_gold_spin) {
			enemy_gold_spin->set_value(current_category == CAT_ARPG_ENEMIES ? 5 : 0);
		}
		if (speed_walk_spin) {
			speed_walk_spin->set_value(100);
			speed_walk_spin->set_meta("cg_last_valid_val", 100.0);
		}
		if (speed_run_spin) {
			speed_run_spin->set_value(100);
			speed_run_spin->set_meta("cg_last_valid_val", 100.0);
		}
		if (speed_attack_spin) {
			speed_attack_spin->set_value(100);
			speed_attack_spin->set_meta("cg_last_valid_val", 100.0);
		}
		if (arpg_ai_type_opt) {
			arpg_ai_type_opt->select(2);
		}
		if (arpg_ai_vision_spin) {
			arpg_ai_vision_spin->set_value(6.0);
			arpg_ai_vision_spin->set_meta("cg_last_valid_val", 6.0);
		}
		if (arpg_ai_range_spin) {
			arpg_ai_range_spin->set_value(1.0);
			arpg_ai_range_spin->set_meta("cg_last_valid_val", 1.0);
		}
		if (arpg_ai_interval_spin) {
			arpg_ai_interval_spin->set_value(1.5);
			arpg_ai_interval_spin->set_meta("cg_last_valid_val", 1.5);
		}
		if (arpg_ai_kb_resist_spin) {
			arpg_ai_kb_resist_spin->set_value(0);
			arpg_ai_kb_resist_spin->set_meta("cg_last_valid_val", 0.0);
		}
		if (battler_frames_spin) {
			battler_frames_spin->set_value(1);
		}
		if (battler_fps_spin) {
			battler_fps_spin->set_value(8);
		}
		for (int s = 0; s < 3; s++) {
			if (drop_kind_opt[s]) {
				drop_kind_opt[s]->select(0);
			}
			_populate_drop_item_option(s, 0);
			if (drop_prob_spin[s]) {
				drop_prob_spin[s]->set_value(100);
			}
		}
		_refresh_enemy_element_rates_ui();
		_refresh_action_patterns_tree(Array());
		_refresh_traits_tree(Array());
		_refresh_effects_tree(Array());
		for (int i = 0; i < 8; i++) {
			stat_spins[i]->set_value(0);
			stat_spins[i]->set_meta("cg_last_valid_val", 0.0);
		}
		if (graphic_preview) {
			graphic_preview->set_texture(Ref<Texture2D>());
		}
		_set_graphic_path_label_text(CG_DB_TR("(Sin registro)"));
		if (current_category == CAT_TILESETS) {
			_refresh_tileset_slots_ui();
		} else {
			if (tileset_collision_editor) {
				tileset_collision_editor->set_tileset_texture(Ref<Texture2D>());
				tileset_collision_editor->set_collision_map(Dictionary());
			}
			if (db_tile_picker) {
				db_tile_picker->set_atlas_texture(Ref<Texture2D>());
				db_tile_picker->set_tile_metadata_map(Dictionary());
			}
		}
		updating_ui = false;
		if (s_is_anim_slot_category(current_category) || current_category == CAT_ENEMIES) {
			_clear_and_disable_anim_editor();
		}
		if (body_zones_editor) {
			body_zones_editor->set_visible(false);
		}
		if (bz_empty_label) {
			bz_empty_label->set_visible(true);
		}
		if (movement_tab_hbox) {
			movement_tab_hbox->set_visible(false);
		}
		if (mov_empty_label) {
			mov_empty_label->set_visible(true);
		}
		if (s_has_event_tab_category(current_category)) {
			_refresh_event_tab_summary();
		}
		return;
	}

	Dictionary d = arr[p_index];
	name_edit->set_text(d.get("name", ""));
	desc_edit->set_text(d.get("desc", d.get("notes", "")));
	if (current_category == CAT_ACTORS) {
		int cls_id = int(d.get("class_id", d.get("type", 0)));
		_select_option_by_id_or_index(type_option, cls_id);
	} else {
		int t_idx = int(d.get("type", 0));
		_select_option_by_id_or_index(type_option, t_idx);
	}
	_select_option_by_id_or_index(secondary_option, int(d.get("secondary_type", 0)));
	_select_option_by_id_or_index(tertiary_option, int(d.get("tertiary_type", 0)));

	int price_val = int(d.get("price", 0));
	if (price_spin) {
		price_spin->set_value(price_val);
		price_spin->set_meta("cg_last_valid_val", double(price_val));
	}
	_update_sell_price_label(price_val);
	if (attack_anim_option) {
		int anim_val = int(d.get("attack_anim_id", d.get("animation_id", 0)));
		_select_option_by_id_or_index(attack_anim_option, anim_val);
	}
	if (consumable_check) {
		consumable_check->set_pressed(bool(d.get("consumable", true)));
	}
	if (item_speed_spin) {
		item_speed_spin->set_value(double(d.get("speed", 0)));
	}
	if (item_success_spin) {
		item_success_spin->set_value(double(d.get("success_rate", 100)));
	}
	if (item_repeats_spin) {
		item_repeats_spin->set_value(double(d.get("repeats", 1)));
	}
	if (item_tp_gain_spin) {
		item_tp_gain_spin->set_value(double(d.get("tp_gain", 0)));
	}
	if (hit_type_option) {
		_select_option_by_id_or_index(hit_type_option, int(d.get("hit_type", 0)));
	}
	if (item_element_option) {
		_select_option_by_id_or_index(item_element_option, int(d.get("element_id", 0)));
	}
	if (variance_spin) {
		variance_spin->set_value(double(d.get("variance", 20)));
	}
	if (critical_check) {
		critical_check->set_pressed(bool(d.get("critical", false)));
	}
	if (enemy_level_spin) {
		int lvl = MAX(1, int(d.get("level", 1)));
		enemy_level_spin->set_value(double(lvl));
		enemy_level_spin->set_meta("cg_last_valid_val", double(lvl));
	}
	if (enemy_exp_spin) {
		enemy_exp_spin->set_value(double(d.get("exp", 0)));
	}
	if (enemy_gold_spin) {
		enemy_gold_spin->set_value(double(d.get("gold", 0)));
	}
	if (battler_frames_spin) {
		battler_frames_spin->set_value(double(d.get("battler_frames", 1)));
	}
	if (battler_fps_spin) {
		battler_fps_spin->set_value(double(d.get("battler_fps", 8)));
	}
	if (current_category == CAT_ENEMIES || current_category == CAT_ARPG_ENEMIES) {
		_refresh_enemy_element_rates_ui();
		Array drops = d.get("drops", Array());
		for (int s = 0; s < 3; s++) {
			Dictionary dr = (s < drops.size() && drops[s].get_type() == Variant::DICTIONARY) ? Dictionary(drops[s]) : Dictionary();
			int kind = int(dr.get("kind", 0));
			int item_id = int(dr.get("item_id", 0));
			int prob = int(dr.get("prob", 100));
			if (drop_kind_opt[s]) {
				_select_option_by_id_or_index(drop_kind_opt[s], kind);
			}
			_populate_drop_item_option(s, kind);
			if (drop_item_opt[s]) {
				_select_option_by_id_or_index(drop_item_opt[s], item_id);
			}
			if (drop_prob_spin[s]) {
				drop_prob_spin[s]->set_value(prob);
			}
		}
		if (current_category == CAT_ENEMIES) {
			Array actions = d.get("actions", d.get("action_patterns", Array()));
			_refresh_action_patterns_tree(actions);
		}
		if (current_category == CAT_ARPG_ENEMIES) {
			if (arpg_ai_type_opt) {
				String ai_type = String(d.get("ai_type", "persigue_y_ataca")).strip_edges().to_lower();
				int sel_ai = 2;
				for (int ai_i = 0; ai_i < arpg_ai_type_opt->get_item_count(); ai_i++) {
					if (String(arpg_ai_type_opt->get_item_metadata(ai_i)) == ai_type) {
						sel_ai = ai_i;
						break;
					}
				}
				arpg_ai_type_opt->select(sel_ai);
			}
			if (arpg_ai_vision_spin) {
				double v = MAX(0.0, double(d.get("vision_radius", 6.0)));
				arpg_ai_vision_spin->set_value(v);
				arpg_ai_vision_spin->set_meta("cg_last_valid_val", v);
			}
			if (arpg_atk_range_spin) {
				double r = MAX(0.0, double(d.get("attack_range", 1.0)));
				arpg_atk_range_spin->set_value(r);
				arpg_atk_range_spin->set_meta("cg_last_valid_val", r);
			}
			if (arpg_atk_interval_spin) {
				double iv = MAX(0.0, double(d.get("attack_interval", 1.5)));
				arpg_atk_interval_spin->set_value(iv);
				arpg_atk_interval_spin->set_meta("cg_last_valid_val", iv);
			}
			if (arpg_atk_interval_var_spin) {
				double iv_var = CLAMP(double(d.get("attack_interval_variance_pct", 0.0)), 0.0, 100.0);
				arpg_atk_interval_var_spin->set_value(iv_var);
				arpg_atk_interval_var_spin->set_meta("cg_last_valid_val", iv_var);
			}
			if (arpg_atk_speed_spin) {
				double spd = MAX(1.0, double(d.get("attack_speed_pct", 100.0)));
				arpg_atk_speed_spin->set_value(spd);
				arpg_atk_speed_spin->set_meta("cg_last_valid_val", spd);
			}
			if (arpg_atk_windup_spin) {
				double wnd = MAX(0.0, double(d.get("attack_windup_ms", 0)));
				arpg_atk_windup_spin->set_value(wnd);
				arpg_atk_windup_spin->set_meta("cg_last_valid_val", wnd);
			}
			if (arpg_atk_recovery_spin) {
				double rec = MAX(0.0, double(d.get("attack_recovery_ms", 0)));
				arpg_atk_recovery_spin->set_value(rec);
				arpg_atk_recovery_spin->set_meta("cg_last_valid_val", rec);
			}
			if (arpg_atk_dmg_mult_spin) {
				double dm = MAX(0.0, double(d.get("attack_damage_mult_pct", 100.0)));
				arpg_atk_dmg_mult_spin->set_value(dm);
				arpg_atk_dmg_mult_spin->set_meta("cg_last_valid_val", dm);
			}
			if (arpg_ai_kb_resist_spin) {
				double kb = CLAMP(double(d.get("knockback_resist", 0)), 0.0, 100.0);
				arpg_ai_kb_resist_spin->set_value(kb);
				arpg_ai_kb_resist_spin->set_meta("cg_last_valid_val", kb);
			}
		}
	}
	_refresh_traits_tree(d.get("traits", Array()));
	_refresh_effects_tree(d.get("effects", Array()));

	formula_edit->set_text(d.get("formula", "max(a.atk * 4 - b.def * 2, 0)"));

	const char *exp_keys[4] = { "exp_base", "exp_extra", "exp_acc_a", "exp_acc_b" };
	for (int i = 0; i < 4; i++) {
		if (exp_spins[i]) {
			double eval = double(d.get(exp_keys[i], exp_defaults[i]));
			exp_spins[i]->set_value(eval);
			exp_spins[i]->set_meta("cg_last_valid_val", eval);
		}
	}
	_update_exp_preview();

	const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	for (int i = 0; i < 8; i++) {
		double val = (double)d.get(stat_keys[i], 0);
		stat_spins[i]->set_value(val);
		stat_spins[i]->set_meta("cg_last_valid_val", val);
	}

	if (current_category == CAT_ACTORS || current_category == CAT_NPCS ||
			current_category == CAT_ARPG_NPCS || current_category == CAT_ARPG_ENEMIES) {
		if (speed_walk_spin) {
			double w = MAX(1.0, double(d.get("walk_speed_pct", 100)));
			speed_walk_spin->set_value(w);
			speed_walk_spin->set_meta("cg_last_valid_val", w);
		}
		if (speed_run_spin) {
			double rn = MAX(1.0, double(d.get("run_speed_pct", 100)));
			speed_run_spin->set_value(rn);
			speed_run_spin->set_meta("cg_last_valid_val", rn);
		}
		if (speed_attack_spin) {
			double atk_sp = MAX(1.0, double(d.get("attack_speed_pct", 100)));
			speed_attack_spin->set_value(atk_sp);
			speed_attack_spin->set_meta("cg_last_valid_val", atk_sp);
		}
	}

	if (current_category == CAT_TILESETS) {
		_refresh_tileset_slots_ui();
	} else if (current_category == CAT_TROOPS) {
		Array mems = d.get("members", Array());
		if (!d.has("members") && d.has("type") && int(d.get("type", 0)) > 0) {
			int old_en = int(d.get("type", 1));
			Dictionary m0;
			m0["enemy_id"] = old_en;
			m0["x"] = 280;
			m0["y"] = 140;
			mems.push_back(m0);
			d["members"] = mems;
			arr[p_index] = d;
		}
		_refresh_troop_members_ui();
		if (col2_scroll_container) {
			col2_scroll_container->set_v_scroll(0);
			col2_scroll_container->call_deferred("set_v_scroll", 0);
		}
	} else if (s_is_anim_slot_category(current_category) || current_category == CAT_ENEMIES) {
		current_sprite_data = ClockGearsSpriteSheetData();
		anim_preview_playing = false;
		anim_preview_timer = 0.0f;
		anim_preview_frame_idx = 0;
		if (anim_empty_state_label) {
			anim_empty_state_label->set_visible(false);
		}
		if (sprite_anim_editor_box) {
			sprite_anim_editor_box->set_visible(true);
		}
		_set_anim_editor_controls_enabled(true);
		if (current_category == CAT_ENEMIES) {
			ClockGearsSpriteSheetData::migrate_enemy_record(d);
			Dictionary sp_dict = d.get("sprite_data", Dictionary());
			sp_dict["graphic_path"] = d.get("graphic_path", "");
			current_sprite_data.from_dict(sp_dict);
			_ensure_enemy_fixed_anim_slots(d);
			arr[p_index] = d;
			if (ClockGearsDataManager::get_singleton()) {
				ClockGearsDataManager::get_singleton()->set_category_array(CAT_ENEMIES, arr);
			}
			_refresh_enemy_casts_ui();
			_refresh_enemy_others_ui();
		} else {
			if (actor_dir_idle_chk) {
				actor_dir_idle_chk->set_pressed(bool(d.get("directional_idle", false)));
			}
			Dictionary sp_dict = d.get("sprite_data", Dictionary());
			if (!sp_dict.is_empty()) {
				sp_dict["graphic_path"] = d.get("graphic_path", "");
				current_sprite_data.from_dict(sp_dict);
			} else {
				int old_frames = int(d.get("battler_frames", 1));
				float old_fps = float(d.get("battler_fps", 8));
				current_sprite_data = ClockGearsSpriteSheetData();
				current_sprite_data.cols = MAX(1, old_frames);
				current_sprite_data.rows = 1;
				if (!String(d.get("graphic_path", "")).is_empty()) {
					ClockGearsSpriteAnimation idle_anim;
					idle_anim.name = "idle";
					idle_anim.spritesheet_path = d.get("graphic_path", "");
					idle_anim.fps = old_fps > 0 ? old_fps : 6.0f;
					idle_anim.loop = true;
					for (int f = 0; f < old_frames; f++) {
						idle_anim.frames.push_back(f);
					}
					current_sprite_data.animations[idle_anim.name] = idle_anim;
				}
			}
		}

		_refresh_anim_track_options();
		_refresh_anim_submode_ui();
		_refresh_shape_editor_context();
		if (s_is_arpg_equip_category(current_category)) {
			String dm_mode = String(d.get("draw_mode", "completo")).strip_edges().to_lower();
			int lorder = d.has("layer_order") ? int(d["layer_order"]) : ClockGearsCharacterStateRegistry::get_default_arpg_layer_order(current_category);
			if (arpg_draw_mode_opt) {
				arpg_draw_mode_opt->select(dm_mode == "simple" ? 1 : 0);
			}
			if (arpg_layer_order_spin) {
				arpg_layer_order_spin->set_value(lorder);
				arpg_layer_order_spin->set_meta("cg_last_valid_val", double(lorder));
			}
			_refresh_arpg_piece_anim_mode_ui();
			_refresh_arpg_simple_fields();
			_refresh_arpg_preview_selectors();
			_update_arpg_preview_canvas();
		}
		if (s_has_event_tab_category(current_category)) {
			_refresh_event_tab_summary();
		}
		if (s_has_body_zones_tab_category(current_category)) {
			_load_body_zones_tab_from_record(d);
		}
		if (s_has_movement_tab_category(current_category)) {
			_load_movement_tab_from_record(d);
		}
	} else {
		_update_graphic_preview_ui(d);
	}

	updating_ui = false;
}

static Dictionary s_finalize_new_db_record(int p_category, Dictionary p_rec) {
	if (p_category == CAT_ARPG_ENEMIES) {
		p_rec.erase("hp");
		p_rec.erase("mp");
		p_rec.erase("atk");
		p_rec.erase("def");
		p_rec.erase("mat");
		p_rec.erase("mdf");
		p_rec.erase("agi");
		p_rec.erase("luk");
	}
	ClockGearsDataManager::normalize_record_defaults(p_category, p_rec);
	return p_rec;
}

void ClockGearsDatabaseDialog::_on_add_record_pressed() {
	if (current_category == DB_CAT_WINDOW_STYLES) {
		window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
		Array styles = window_styles_config_draft.get("styles", Array());
		HashSet<String> used_ids;
		for (int i = 0; i < styles.size(); i++) {
			used_ids.insert(String(Dictionary(styles[i]).get("id", "")));
		}
		int n = styles.size() + 1;
		String new_id = vformat("style_%d", n);
		while (used_ids.has(new_id)) {
			n++;
			new_id = vformat("style_%d", n);
		}
		Dictionary tmpl;
		if (ws_selected_index >= 0 && ws_selected_index < styles.size()) {
			tmpl = Dictionary(styles[ws_selected_index]).duplicate(true);
		} else if (!styles.is_empty()) {
			tmpl = Dictionary(styles[0]).duplicate(true);
		}
		tmpl["id"] = new_id;
		tmpl["name"] = vformat(CG_DB_TR("Estilo %d"), n);
		tmpl["built_in"] = false;
		tmpl = ClockGearsDataManager::normalize_window_style(tmpl);
		styles.push_back(tmpl);
		window_styles_config_draft["styles"] = styles;
		int new_idx = styles.size() - 1;
		ws_selected_index = new_idx;
		_refresh_record_list();
		if (record_list && new_idx >= 0 && new_idx < record_list->get_item_count()) {
			record_list->select(new_idx);
		}
		_on_record_selected(new_idx);
		_mark_database_dirty();
		if (ws_name_edit && ws_name_edit->is_visible_in_tree()) {
			ws_name_edit->grab_focus();
			ws_name_edit->select_all();
		}
		return;
	}
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	int new_id = arr.size() + 1;
	Dictionary rec = s_finalize_new_db_record(current_category, _create_empty_record(current_category, new_id));
	if (current_category == CAT_TILESETS && String(rec.get("name", "")).is_empty()) {
		rec["name"] = vformat("Tileset %03d", new_id);
	}
	arr.push_back(rec);
	if (current_category == CAT_TILESETS) {
		print_line(vformat(String::utf8("[ClockGears DB Tilesets] Botón '+' -> Creado Tileset #%03d ('%s'). Total: %d."), new_id, String(rec["name"]), arr.size()));
	}
	// Si es el primer Actor creado y el Grupo Inicial está vacío, lo agregamos al grupo inicial automáticamente
	if (current_category == CAT_ACTORS) {
		Array party = system_config.get("party", Array());
		if (party.is_empty()) {
			party.push_back(new_id);
			system_config["party"] = party;
		}
	}
	_refresh_record_list();
	int last_idx = arr.size() - 1;
	record_list->select(last_idx);
	_on_record_selected(last_idx);
	_mark_database_dirty();
	if (name_edit && name_edit->is_visible_in_tree()) {
		name_edit->grab_focus();
		name_edit->select_all();
	}
}

void ClockGearsDatabaseDialog::_on_remove_record_pressed() {
	if (current_category == DB_CAT_WINDOW_STYLES) {
		window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
		Array styles = window_styles_config_draft.get("styles", Array());
		if (styles.size() <= 1 || !record_list) {
			return;
		}
		Vector<int> sel = record_list->get_selected_items();
		int rm_idx = sel.is_empty() ? ws_selected_index : sel[0];
		if (rm_idx < 0 || rm_idx >= styles.size()) {
			return;
		}
		Dictionary rm_st = styles[rm_idx];
		if (bool(rm_st.get("built_in", false))) {
			return; // Los estilos incluidos nunca se borran
		}
		String rm_id = String(rm_st.get("id", ""));
		styles.remove_at(rm_idx);
		window_styles_config_draft["styles"] = styles;
		if (String(window_styles_config_draft.get("default_style_id", "")) == rm_id) {
			String fallback_id = !styles.is_empty() ? String(Dictionary(styles[0]).get("id", "rounded")) : String("rounded");
			window_styles_config_draft["default_style_id"] = fallback_id;
		}
		Dictionary assigns = window_styles_config_draft.get("assignments", Dictionary());
		Array akeys = assigns.keys();
		for (int i = 0; i < akeys.size(); i++) {
			Variant k = akeys[i];
			if (String(assigns[k]) == rm_id) {
				assigns[k] = "";
			}
		}
		window_styles_config_draft["assignments"] = assigns;
		window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
		Array norm_styles = window_styles_config_draft.get("styles", Array());
		int next_sel = norm_styles.is_empty() ? -1 : MIN(rm_idx, norm_styles.size() - 1);
		ws_selected_index = next_sel;
		_refresh_record_list();
		if (next_sel >= 0 && record_list && next_sel < record_list->get_item_count()) {
			record_list->select(next_sel);
		}
		_on_record_selected(next_sel);
		_mark_database_dirty();
		return;
	}
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	if (arr.is_empty()) {
		return;
	}
	Vector<int> sel = record_list->get_selected_items();
	int rm_idx = sel.is_empty() ? (arr.size() - 1) : sel[0];
	Dictionary rm_rec = arr[rm_idx];
	int rm_id = int(rm_rec.get("id", rm_idx + 1));
	String rm_name = rm_rec.get("name", "");
	arr.remove_at(rm_idx);

	// Re-index IDs cleanly
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		d["id"] = i + 1;
		arr[i] = d;
	}
	if (current_category == CAT_TILESETS) {
		print_line(vformat(String::utf8("[ClockGears DB Tilesets] Botón '-' -> Eliminado Tileset #%03d ('%s'). Restantes: %d."), rm_id, rm_name, arr.size()));
	}
	if (current_category == CAT_ACTORS) {
		Array party = system_config.get("party", Array());
		Array new_party;
		for (int i = 0; i < party.size(); i++) {
			int pid = int(party[i]);
			if (pid == rm_id) {
				continue;
			}
			new_party.push_back(pid > rm_id ? pid - 1 : pid);
		}
		system_config["party"] = new_party;
	}
	_refresh_record_list();
	int next_sel = MIN(rm_idx, arr.size() - 1);
	if (next_sel >= 0) {
		record_list->select(next_sel);
	}
	_on_record_selected(next_sel);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_change_max_pressed() {
	if (current_category < 0 || current_category >= CAT_MAX || !change_max_dialog || !change_max_spin) {
		return;
	}
	change_max_spin->set_value(category_data[current_category].size());
	change_max_dialog->popup_centered(Size2(320, 130));
}

void ClockGearsDatabaseDialog::_on_change_max_confirmed() {
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	int target_max = (int)change_max_spin->get_value();
	Array &arr = category_data[current_category];
	while (arr.size() < target_max) {
		int new_id = arr.size() + 1;
		arr.push_back(s_finalize_new_db_record(current_category, _create_empty_record(current_category, new_id)));
	}
	while (arr.size() > target_max) {
		arr.remove_at(arr.size() - 1);
	}
	_refresh_record_list();
	if (record_list->get_item_count() > 0) {
		record_list->select(0);
		_on_record_selected(0);
	} else {
		_on_record_selected(-1);
	}
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_copy_record_pressed() {
	if (current_category == DB_CAT_WINDOW_STYLES) {
		Array styles = window_styles_config_draft.get("styles", Array());
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		int idx = sel.is_empty() ? ws_selected_index : sel[0];
		if (idx < 0 || idx >= styles.size()) {
			return;
		}
		ws_clipboard_style = Dictionary(styles[idx]).duplicate(true);
		if (btn_paste_record) {
			btn_paste_record->set_disabled(false);
		}
		return;
	}
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	int idx = sel[0];
	const Array &arr = category_data[current_category];
	if (idx < 0 || idx >= arr.size()) {
		return;
	}
	clipboard_record = Dictionary(arr[idx]).duplicate(true);
	if (btn_paste_record) {
		btn_paste_record->set_disabled(false);
	}
}

void ClockGearsDatabaseDialog::_on_paste_record_pressed() {
	if (current_category == DB_CAT_WINDOW_STYLES) {
		if (ws_clipboard_style.is_empty()) {
			return;
		}
		window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
		Array styles = window_styles_config_draft.get("styles", Array());
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		int sel_idx = (!sel.is_empty() && sel[0] >= 0 && sel[0] < styles.size()) ? sel[0] : -1;
		int target_idx = -1;
		if (sel_idx >= 0 && !bool(Dictionary(styles[sel_idx]).get("built_in", false))) {
			Dictionary cur = styles[sel_idx];
			String keep_id = String(cur.get("id", ""));
			Dictionary pasted = ws_clipboard_style.duplicate(true);
			pasted["id"] = keep_id;
			pasted["built_in"] = false;
			if (String(pasted.get("name", "")).is_empty() || String(pasted.get("name", "")) == String(ws_clipboard_style.get("name", ""))) {
				// Conservar el nombre pegado para duplicados o indicar copia
				String base_name = String(ws_clipboard_style.get("name", CG_DB_TR("Estilo")));
				pasted["name"] = base_name + String(CG_DB_TR(" (copia)"));
			}
			styles[sel_idx] = ClockGearsDataManager::normalize_window_style(pasted);
			target_idx = sel_idx;
		} else {
			HashSet<String> used_ids;
			for (int i = 0; i < styles.size(); i++) {
				used_ids.insert(String(Dictionary(styles[i]).get("id", "")));
			}
			int n = styles.size() + 1;
			String new_id = vformat("style_%d", n);
			while (used_ids.has(new_id)) {
				n++;
				new_id = vformat("style_%d", n);
			}
			Dictionary pasted = ws_clipboard_style.duplicate(true);
			pasted["id"] = new_id;
			pasted["built_in"] = false;
			String base_name = String(ws_clipboard_style.get("name", CG_DB_TR("Estilo")));
			pasted["name"] = base_name + String(CG_DB_TR(" (copia)"));
			styles.push_back(ClockGearsDataManager::normalize_window_style(pasted));
			target_idx = styles.size() - 1;
		}
		window_styles_config_draft["styles"] = styles;
		ws_selected_index = target_idx;
		_refresh_record_list();
		if (record_list && target_idx >= 0 && target_idx < record_list->get_item_count()) {
			record_list->select(target_idx);
		}
		_on_record_selected(target_idx);
		_mark_database_dirty();
		return;
	}
	if (clipboard_record.is_empty() || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	int target_idx = -1;
	if (!sel.is_empty() && sel[0] >= 0 && sel[0] < arr.size()) {
		target_idx = sel[0];
		Dictionary cur = arr[target_idx];
		int keep_id = int(cur.get("id", target_idx + 1));
		Dictionary pasted = clipboard_record.duplicate(true);
		pasted["id"] = keep_id;
		ClockGearsDataManager::normalize_record_defaults(current_category, pasted);
		arr[target_idx] = pasted;
	} else {
		int new_id = arr.size() + 1;
		Dictionary pasted = clipboard_record.duplicate(true);
		pasted["id"] = new_id;
		ClockGearsDataManager::normalize_record_defaults(current_category, pasted);
		arr.push_back(pasted);
		target_idx = arr.size() - 1;
	}
	_refresh_record_list();
	record_list->select(target_idx);
	_on_record_selected(target_idx);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_param_changed() {
	if (updating_ui || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	int idx = -1;
	bool auto_created_new = false;
	if (sel.is_empty()) {
		// Si el usuario escribe directamente cuando la lista está vacía, auto-creamos el registro 001
		if (arr.is_empty()) {
			current_sprite_data = ClockGearsSpriteSheetData();
			anim_preview_playing = false;
			if (actor_dir_idle_chk) {
				updating_ui = true;
				actor_dir_idle_chk->set_pressed(false);
				updating_ui = false;
			}
			arr.push_back(s_finalize_new_db_record(current_category, _create_empty_record(current_category, 1)));
			updating_ui = true;
			_refresh_record_list();
			record_list->select(0);
			if (btn_select_graphic) {
				btn_select_graphic->set_disabled(false);
			}
			if (btn_load_tileset_sheet) {
				btn_load_tileset_sheet->set_disabled(false);
			}
			if (btn_remove_record) {
				btn_remove_record->set_disabled(false);
			}
			if (btn_copy_record) {
				btn_copy_record->set_disabled(false);
			}
			updating_ui = false;
			idx = 0;
			auto_created_new = true;
		} else {
			idx = 0;
			record_list->select(0);
		}
	} else {
		idx = sel[0];
	}
	if (idx < 0 || idx >= arr.size()) {
		return;
	}
	if (auto_created_new) {
		if (s_is_anim_slot_category(current_category) || current_category == CAT_ENEMIES) {
			if (anim_empty_state_label) {
				anim_empty_state_label->set_visible(false);
			}
			if (sprite_anim_editor_box) {
				sprite_anim_editor_box->set_visible(true);
			}
			_set_anim_editor_controls_enabled(true);
			_refresh_anim_track_options();
		}
		_refresh_event_tab_summary();
	}

	Dictionary d = arr[idx];
	d["name"] = name_edit->get_text();
	d["desc"] = desc_edit->get_text();
	if (current_category == CAT_TROOPS) {
		d["notes"] = d["desc"];
	}

	int sel_primary = type_option->get_selected();
	int primary_id = (sel_primary >= 0) ? type_option->get_item_id(sel_primary) : 0;
	if (current_category == CAT_ACTORS) {
		d["class_id"] = primary_id;
		d["type"] = primary_id;
	} else {
		d["type"] = primary_id;
	}

	int sel_sec = secondary_option ? secondary_option->get_selected() : -1;
	d["secondary_type"] = (sel_sec >= 0 && secondary_option) ? secondary_option->get_item_id(sel_sec) : 0;

	int sel_ter = tertiary_option ? tertiary_option->get_selected() : -1;
	d["tertiary_type"] = (sel_ter >= 0 && tertiary_option) ? tertiary_option->get_item_id(sel_ter) : 0;

	d["formula"] = formula_edit->get_text();

	if (current_category == CAT_CLASSES) {
		const char *exp_keys[4] = { "exp_base", "exp_extra", "exp_acc_a", "exp_acc_b" };
		for (int i = 0; i < 4; i++) {
			if (exp_spins[i]) {
				d[exp_keys[i]] = (int)exp_spins[i]->get_value();
			}
		}
		_update_exp_preview();
	}

	if (current_category == CAT_WEAPONS || current_category == CAT_ARMORS || current_category == CAT_ACCESSORIES || current_category == CAT_ITEMS) {
		int pval = price_spin ? int(price_spin->get_value()) : 0;
		d["price"] = pval;
		_update_sell_price_label(pval);
	}
	if ((current_category == CAT_WEAPONS || current_category == CAT_SKILLS || current_category == CAT_ITEMS) && attack_anim_option) {
		int sa = attack_anim_option->get_selected();
		int aid = (sa >= 0) ? attack_anim_option->get_item_id(sa) : 0;
		d["attack_anim_id"] = aid;
		d["animation_id"] = aid;
	}
	if (current_category == CAT_ITEMS && consumable_check) {
		d["consumable"] = consumable_check->is_pressed();
	}
	if (current_category == CAT_ITEMS || current_category == CAT_SKILLS) {
		if (item_speed_spin) {
			d["speed"] = int(item_speed_spin->get_value());
		}
		if (item_success_spin) {
			d["success_rate"] = int(item_success_spin->get_value());
		}
		if (item_repeats_spin) {
			d["repeats"] = int(item_repeats_spin->get_value());
		}
		if (item_tp_gain_spin) {
			d["tp_gain"] = int(item_tp_gain_spin->get_value());
		}
		if (hit_type_option) {
			int sh = hit_type_option->get_selected();
			d["hit_type"] = (sh >= 0) ? hit_type_option->get_item_id(sh) : 0;
		}
		if (current_category == CAT_ITEMS && item_element_option) {
			int se = item_element_option->get_selected();
			d["element_id"] = (se >= 0) ? item_element_option->get_item_id(se) : 0;
		}
		if (variance_spin) {
			d["variance"] = int(variance_spin->get_value());
		}
		if (critical_check) {
			d["critical"] = critical_check->is_pressed();
		}
	}
	if (current_category == CAT_ENEMIES || (current_category == CAT_ARPG_ENEMIES && !auto_created_new)) {
		if (enemy_level_spin) {
			d["level"] = MAX(1, int(enemy_level_spin->get_value()));
		}
		if (enemy_exp_spin) {
			d["exp"] = MAX(0, int(enemy_exp_spin->get_value()));
		}
		if (enemy_gold_spin) {
			d["gold"] = MAX(0, int(enemy_gold_spin->get_value()));
		}
	}
	if (s_is_anim_slot_category(current_category) || current_category == CAT_ENEMIES) {
		if (s_is_anim_slot_category(current_category) && actor_dir_idle_chk) {
			d["directional_idle"] = actor_dir_idle_chk->is_pressed();
		}
		d["sprite_data"] = current_sprite_data.to_dict();
		d["animations"] = current_sprite_data.to_dict().get("animations", Dictionary());
		if (s_is_arpg_equip_category(current_category)) {
			if (arpg_draw_mode_opt) {
				d["draw_mode"] = (arpg_draw_mode_opt->get_selected() == 1) ? "simple" : "completo";
			}
			if (arpg_layer_order_spin) {
				d["layer_order"] = int(arpg_layer_order_spin->get_value());
			}
		}
		if (current_category == CAT_ACTORS || current_category == CAT_ENEMIES) {
			Dictionary ca = d.get("combat_animations", Dictionary());
			if (current_category == CAT_ENEMIES) {
				ca["idle"] = "idle";
				ca["attack"] = "attack";
				ca["damage"] = "hurt";
				ca["death"] = "death";
				ca["buff"] = "buff";
				ca["defend"] = "idle";
			} else {
				if (opt_anim_idle) {
					ca["idle"] = opt_anim_idle->get_selected() > 0 ? opt_anim_idle->get_item_text(opt_anim_idle->get_selected()) : "";
				}
				if (opt_anim_attack) {
					ca["attack"] = opt_anim_attack->get_selected() > 0 ? opt_anim_attack->get_item_text(opt_anim_attack->get_selected()) : "";
				}
				if (opt_anim_damage) {
					ca["damage"] = opt_anim_damage->get_selected() > 0 ? opt_anim_damage->get_item_text(opt_anim_damage->get_selected()) : "";
				}
				if (opt_anim_death) {
					ca["death"] = opt_anim_death->get_selected() > 0 ? opt_anim_death->get_item_text(opt_anim_death->get_selected()) : "";
				}
				if (opt_anim_defend) {
					ca["defend"] = opt_anim_defend->get_selected() > 0 ? opt_anim_defend->get_item_text(opt_anim_defend->get_selected()) : "";
				}
				if (opt_anim_victory) {
					ca["victory"] = opt_anim_victory->get_selected() > 0 ? opt_anim_victory->get_item_text(opt_anim_victory->get_selected()) : "";
				}
			}
			d["combat_animations"] = ca;

			Dictionary tt = d.get("trigger_table", Dictionary());
			tt[itos(ANIM_TRIGGER_IDLE)] = ca["idle"];
			tt[itos(ANIM_TRIGGER_ATTACK)] = ca["attack"];
			tt[itos(ANIM_TRIGGER_DAMAGE)] = ca["damage"];
			tt[itos(ANIM_TRIGGER_DEATH)] = ca["death"];
			tt[itos(ANIM_TRIGGER_DEFEND)] = ca["defend"];
			if (current_category == CAT_ACTORS) {
				tt[itos(ANIM_TRIGGER_VICTORY)] = ca.get("victory", "");
			}
			d["trigger_table"] = tt;
		}
	}

	if (current_category == CAT_ACTORS || current_category == CAT_NPCS ||
			current_category == CAT_ARPG_NPCS || current_category == CAT_ARPG_ENEMIES) {
		if (!auto_created_new) {
			if (speed_walk_spin) {
				d["walk_speed_pct"] = MAX(1, int(speed_walk_spin->get_value()));
			}
			if (speed_run_spin) {
				d["run_speed_pct"] = MAX(1, int(speed_run_spin->get_value()));
			}
			bool save_atk_spd = (current_category == CAT_ARPG_NPCS || current_category == CAT_ARPG_ENEMIES ||
					(current_category == CAT_ACTORS && current_combat_mode == COMBAT_MODE_ACTION_RPG));
			if (save_atk_spd && speed_attack_spin) {
				d["attack_speed_pct"] = MAX(1, int(speed_attack_spin->get_value()));
			}
		}
	}

	if (current_category == CAT_ARPG_ENEMIES) {
		if (!auto_created_new) {
			const int arpg_indices[6] = { 0, 2, 3, 4, 5, 7 }; // hp, atk, def, mat, mdf, luk (sin MP ni AGI)
			const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
			for (int k = 0; k < 6; k++) {
				int si = arpg_indices[k];
				d[stat_keys[si]] = (int)stat_spins[si]->get_value();
			}
		}
		d.erase("mp");
		d.erase("agi");
		ClockGearsDataManager::normalize_record_defaults(CAT_ARPG_ENEMIES, d);
	} else {
		const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
		for (int i = 0; i < 8; i++) {
			d[stat_keys[i]] = (int)stat_spins[i]->get_value();
		}
		ClockGearsDataManager::normalize_record_defaults(current_category, d);
	}

	arr[idx] = d;
	if (auto_created_new && current_category == CAT_ARPG_ENEMIES) {
		updating_ui = true;
		if (enemy_level_spin) {
			enemy_level_spin->set_value(double(d.get("level", 1)));
		}
		if (enemy_exp_spin) {
			enemy_exp_spin->set_value(double(d.get("exp", 10)));
		}
		if (enemy_gold_spin) {
			enemy_gold_spin->set_value(double(d.get("gold", 5)));
		}
		const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
		for (int i = 0; i < 8; i++) {
			double val = double(d.get(stat_keys[i], 0));
			stat_spins[i]->set_value(val);
			stat_spins[i]->set_meta("cg_last_valid_val", val);
		}
		if (speed_walk_spin) {
			speed_walk_spin->set_value(double(d.get("walk_speed_pct", 100)));
		}
		if (speed_run_spin) {
			speed_run_spin->set_value(double(d.get("run_speed_pct", 100)));
		}
		if (speed_attack_spin) {
			speed_attack_spin->set_value(double(d.get("attack_speed_pct", 100)));
		}
		if (arpg_ai_vision_spin) {
			arpg_ai_vision_spin->set_value(double(d.get("vision_radius", 6.0)));
		}
		if (arpg_ai_range_spin) {
			arpg_ai_range_spin->set_value(double(d.get("attack_range", 1.0)));
		}
		if (arpg_ai_interval_spin) {
			arpg_ai_interval_spin->set_value(double(d.get("attack_interval", 1.5)));
		}
		if (arpg_ai_kb_resist_spin) {
			arpg_ai_kb_resist_spin->set_value(double(d.get("knockback_resist", 0)));
		}
		_refresh_enemy_element_rates_ui();
		updating_ui = false;
	}
	int id = d.get("id", idx + 1);
	record_list->set_item_text(idx, vformat("%03d: %s", id, String(d["name"])));
	if (s_is_anim_slot_category(current_category) || current_category == CAT_ENEMIES) {
		_invalidate_record_list_icon(current_category, idx);
		_update_record_list_item_icon(idx);
	}
	if (s_has_body_zones_tab_category(current_category) || s_has_movement_tab_category(current_category)) {
		_sync_entity_preview_textures();
	}
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_param_text_changed(const String &p_new_text) {
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_param_option_changed(int p_idx) {
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_param_spin_changed(double p_val) {
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_param_check_toggled(bool p_pressed) {
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_update_sell_price_label(int p_price) {
	if (!sell_price_label) {
		return;
	}
	int sell_val = int(Math::floor(double(p_price) * 0.5));
	String curr = system_config.get("currency", "G");
	sell_price_label->set_text(vformat("%d %s (50%%)", sell_val, curr));
}

void ClockGearsDatabaseDialog::_populate_drop_item_option(int p_slot, int p_kind) {
	if (p_slot < 0 || p_slot >= 3 || !drop_item_opt[p_slot]) {
		return;
	}
	OptionButton *opt = drop_item_opt[p_slot];
	opt->clear();
	if (p_kind == CAT_ITEMS || p_kind == CAT_WEAPONS || p_kind == CAT_ARMORS) {
		_populate_db_records_option(opt, p_kind, CG_DB_TR("(ninguno)"), false);
		opt->set_disabled(false);
	} else {
		opt->add_item(CG_DB_TR("(ninguno)"), 0);
		opt->set_disabled(true);
	}
}

void ClockGearsDatabaseDialog::_on_drop_kind_changed(int p_idx, int p_slot) {
	if (updating_ui || p_slot < 0 || p_slot >= 3 || !drop_kind_opt[p_slot]) {
		return;
	}
	int kind = drop_kind_opt[p_slot]->get_item_id(p_idx);
	updating_ui = true;
	_populate_drop_item_option(p_slot, kind);
	updating_ui = false;
	_on_drop_field_changed(p_slot);
}

void ClockGearsDatabaseDialog::_on_drop_field_changed(int p_slot) {
	if (updating_ui || (current_category != CAT_ENEMIES && current_category != CAT_ARPG_ENEMIES)) {
		return;
	}
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) {
		return;
	}
	int idx = sel[0];
	Dictionary d = arr[idx];
	Array drops = d.get("drops", Array());
	while (drops.size() < 3) {
		Dictionary dr;
		dr["kind"] = 0;
		dr["item_id"] = 0;
		dr["prob"] = 100;
		drops.push_back(dr);
	}
	for (int s = 0; s < 3; s++) {
		Dictionary dr = (drops[s].get_type() == Variant::DICTIONARY) ? Dictionary(drops[s]) : Dictionary();
		int k_sel = drop_kind_opt[s] ? drop_kind_opt[s]->get_selected() : -1;
		int i_sel = drop_item_opt[s] ? drop_item_opt[s]->get_selected() : -1;
		dr["kind"] = (k_sel >= 0 && drop_kind_opt[s]) ? drop_kind_opt[s]->get_item_id(k_sel) : 0;
		dr["item_id"] = (i_sel >= 0 && drop_item_opt[s]) ? drop_item_opt[s]->get_item_id(i_sel) : 0;
		dr["prob"] = drop_prob_spin[s] ? int(drop_prob_spin[s]->get_value()) : 100;
		drops[s] = dr;
	}
	d["drops"] = drops;
	arr[idx] = d;
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_refresh_action_patterns_tree(const Array &p_actions) {
	if (!enemy_actions_tree) {
		return;
	}
	enemy_actions_tree->clear();
	TreeItem *root = enemy_actions_tree->create_item();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	Array ai_defs = cat ? cat->get_ai_condition_definitions() : Array();
	const Array &skills = category_data[CAT_SKILLS];

	for (int i = 0; i < p_actions.size(); i++) {
		if (p_actions[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary act = p_actions[i];
		int skill_id = int(act.get("skill_id", 0));
		int cond_type = int(act.get("cond_type", 0));
		int cond_a = int(act.get("cond_a", 0));
		int cond_b = int(act.get("cond_b", 100));
		int rating = int(act.get("rating", 5));

		String skill_name = CG_DB_TR("(ninguna)");
		for (int k = 0; k < skills.size(); k++) {
			Dictionary sk = skills[k];
			if (int(sk.get("id", k + 1)) == skill_id) {
				skill_name = vformat("%03d: %s", skill_id, String(sk.get("name", "")));
				break;
			}
		}

		String cond_name = CG_DB_TR("Siempre");
		for (int c = 0; c < ai_defs.size(); c++) {
			Dictionary ad = ai_defs[c];
			if (int(ad.get("code", c)) == cond_type) {
				cond_name = ad.get("name", "");
				break;
			}
		}

		String param_str = "-";
		if (cond_type == CG_AI_COND_TURN) {
			param_str = vformat("Turno %d", MAX(1, cond_a));
		} else if (cond_type == CG_AI_COND_HP_RANGE) {
			int max_pct = (cond_b > 0 && cond_b <= 100 && cond_a == 0) ? cond_b : (cond_a > 0 ? cond_a : cond_b);
			param_str = vformat("< %d%%", max_pct);
		} else if (cond_type == CG_AI_COND_MP_RANGE) {
			param_str = vformat("%d%% ~ %d%%", cond_a, cond_b);
		} else if (cond_type == CG_AI_COND_STATE) {
			param_str = vformat("Estado #%d", cond_a);
		} else if (cond_type == CG_AI_COND_SWITCH) {
			param_str = vformat("Interruptor #%d", cond_a);
		} else if (cond_type != CG_AI_COND_ALWAYS) {
			param_str = vformat("%d", cond_a);
		}

		TreeItem *item = enemy_actions_tree->create_item(root);
		item->set_text(0, skill_name);
		item->set_text(1, cond_name);
		item->set_text(2, param_str);
		item->set_text(3, String::num_int64(rating));
		item->set_metadata(0, i);
	}
}

void ClockGearsDatabaseDialog::_on_add_action_pattern_pressed() {
	if (!action_pattern_dialog || current_category != CAT_ENEMIES) {
		return;
	}
	_populate_db_records_option(ap_skill_opt, CAT_SKILLS, CG_DB_TR("(ninguna)"), false);
	if (ap_cond_opt) {
		ap_cond_opt->clear();
		ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
		if (cat) {
			Array ai_defs = cat->get_ai_condition_definitions();
			for (int i = 0; i < ai_defs.size(); i++) {
				Dictionary ad = ai_defs[i];
				ap_cond_opt->add_item(ad.get("name", ""), int(ad.get("code", i)));
			}
		}
	}
	if (ap_cond_a_spin) {
		ap_cond_a_spin->set_value(0);
	}
	if (ap_cond_b_spin) {
		ap_cond_b_spin->set_value(100);
	}
	if (ap_rating_spin) {
		ap_rating_spin->set_value(5);
	}
	action_pattern_dialog->popup_centered(Size2(420, 260) * EDSCALE);
}

void ClockGearsDatabaseDialog::_on_remove_action_pattern_pressed() {
	if (current_category != CAT_ENEMIES || !enemy_actions_tree) {
		return;
	}
	TreeItem *sel_item = enemy_actions_tree->get_selected();
	if (!sel_item) {
		return;
	}
	int act_idx = int(sel_item->get_metadata(0));
	Array &arr = category_data[CAT_ENEMIES];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) {
		return;
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	Array actions = d.get("actions", Array());
	if (act_idx >= 0 && act_idx < actions.size()) {
		actions.remove_at(act_idx);
		d["actions"] = actions;
		arr[rec_idx] = d;
		_refresh_action_patterns_tree(actions);
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_action_pattern_confirmed() {
	if (current_category != CAT_ENEMIES) {
		return;
	}
	Array &arr = category_data[CAT_ENEMIES];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		if (arr.is_empty()) {
			arr.push_back(_create_empty_record(CAT_ENEMIES, 1));
			_refresh_record_list();
		}
		record_list->select(0);
		sel = record_list->get_selected_items();
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	Array actions = d.get("actions", Array());

	Dictionary act;
	int sk_sel = ap_skill_opt ? ap_skill_opt->get_selected() : -1;
	int cd_sel = ap_cond_opt ? ap_cond_opt->get_selected() : -1;
	act["skill_id"] = (sk_sel >= 0 && ap_skill_opt) ? ap_skill_opt->get_item_id(sk_sel) : 0;
	act["cond_type"] = (cd_sel >= 0 && ap_cond_opt) ? ap_cond_opt->get_item_id(cd_sel) : 0;
	act["cond_a"] = ap_cond_a_spin ? int(ap_cond_a_spin->get_value()) : 0;
	act["cond_b"] = ap_cond_b_spin ? int(ap_cond_b_spin->get_value()) : 100;
	act["rating"] = ap_rating_spin ? int(ap_rating_spin->get_value()) : 5;

	actions.push_back(act);
	d["actions"] = actions;
	arr[rec_idx] = d;
	_refresh_action_patterns_tree(actions);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_populate_trait_data_option(int p_trait_code) {
	if (!trait_data_opt) {
		return;
	}
	trait_data_opt->clear();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	if (!cat) {
		trait_data_opt->add_item("-", 0);
		return;
	}
	switch (p_trait_code) {
		case CG_TRAIT_ELEMENT_RATE:
		case CG_TRAIT_ATTACK_ELEMENT:
			_populate_catalog_option(trait_data_opt, cat->get_elements());
			break;
		case CG_TRAIT_DEBUFF_RATE:
		case CG_TRAIT_PARAM:
			_populate_catalog_option(trait_data_opt, cat->get_base_param_names());
			break;
		case CG_TRAIT_XPARAM:
			_populate_catalog_option(trait_data_opt, cat->get_ex_param_names());
			break;
		case CG_TRAIT_SPARAM:
			_populate_catalog_option(trait_data_opt, cat->get_sp_param_names());
			break;
		case CG_TRAIT_STATE_RATE:
		case CG_TRAIT_STATE_RESIST:
		case CG_TRAIT_ATTACK_STATE:
			_populate_db_records_option(trait_data_opt, CAT_STATES, CG_DB_TR("(ninguno)"), false);
			break;
		case CG_TRAIT_SKILL_ADD:
		case CG_TRAIT_SKILL_SEAL:
			_populate_db_records_option(trait_data_opt, CAT_SKILLS, CG_DB_TR("(ninguna)"), false);
			break;
		case CG_TRAIT_EQUIP_WTYPE:
			_populate_catalog_option(trait_data_opt, cat->get_weapon_types());
			break;
		case CG_TRAIT_EQUIP_ATYPE:
			_populate_catalog_option(trait_data_opt, cat->get_armor_types());
			break;
		case CG_TRAIT_EQUIP_LOCK:
		case CG_TRAIT_EQUIP_SEAL:
			_populate_catalog_option(trait_data_opt, cat->get_equip_slots());
			break;
		default:
			trait_data_opt->add_item(CG_DB_TR("Estándar (0)"), 0);
			break;
	}
}

void ClockGearsDatabaseDialog::_on_trait_type_changed(int p_idx) {
	if (!trait_type_opt) {
		return;
	}
	int code = trait_type_opt->get_item_id(p_idx);
	_populate_trait_data_option(code);
}

void ClockGearsDatabaseDialog::_refresh_traits_tree(const Array &p_traits) {
	if (!traits_tree) {
		return;
	}
	traits_tree->clear();
	TreeItem *root = traits_tree->create_item();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	Array t_defs = cat ? cat->get_trait_definitions() : Array();

	for (int i = 0; i < p_traits.size(); i++) {
		if (p_traits[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary tr = p_traits[i];
		int code = int(tr.get("code", CG_TRAIT_PARAM));
		int data_id = int(tr.get("data_id", 0));
		double val = double(tr.get("value", 0.0));

		String type_name = vformat("Rasgo #%d", code);
		for (int j = 0; j < t_defs.size(); j++) {
			Dictionary td = t_defs[j];
			if (int(td.get("code", 0)) == code) {
				type_name = td.get("name", type_name);
				break;
			}
		}

		TreeItem *item = traits_tree->create_item(root);
		item->set_text(0, type_name);
		item->set_text(1, vformat("ID %d", data_id));
		item->set_text(2, String::num(val, 1));
		item->set_metadata(0, i);
	}
}

void ClockGearsDatabaseDialog::_on_add_trait_pressed() {
	if (!trait_dialog || !trait_type_opt) {
		return;
	}
	trait_type_opt->clear();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	if (cat) {
		Array t_defs = cat->get_trait_definitions();
		for (int i = 0; i < t_defs.size(); i++) {
			Dictionary td = t_defs[i];
			trait_type_opt->add_item(td.get("name", ""), int(td.get("code", i)));
		}
	}
	if (trait_type_opt->get_item_count() > 0) {
		trait_type_opt->select(0);
		_populate_trait_data_option(trait_type_opt->get_item_id(0));
	}
	if (trait_val_spin) {
		trait_val_spin->set_value(100);
	}
	trait_dialog->popup_centered(Size2(420, 220) * EDSCALE);
}

void ClockGearsDatabaseDialog::_on_remove_trait_pressed() {
	if (!traits_tree || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	TreeItem *sel_item = traits_tree->get_selected();
	if (!sel_item) {
		return;
	}
	int t_idx = int(sel_item->get_metadata(0));
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) {
		return;
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	Array traits = d.get("traits", Array());
	if (t_idx >= 0 && t_idx < traits.size()) {
		traits.remove_at(t_idx);
		d["traits"] = traits;
		arr[rec_idx] = d;
		_refresh_traits_tree(traits);
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_trait_confirmed() {
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		if (arr.is_empty()) {
			arr.push_back(s_finalize_new_db_record(current_category, _create_empty_record(current_category, 1)));
			_refresh_record_list();
		}
		record_list->select(0);
		sel = record_list->get_selected_items();
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	Array traits = d.get("traits", Array());

	Dictionary tr;
	int t_sel = trait_type_opt ? trait_type_opt->get_selected() : -1;
	int d_sel = trait_data_opt ? trait_data_opt->get_selected() : -1;
	tr["code"] = (t_sel >= 0 && trait_type_opt) ? trait_type_opt->get_item_id(t_sel) : int(CG_TRAIT_PARAM);
	tr["data_id"] = (d_sel >= 0 && trait_data_opt) ? trait_data_opt->get_item_id(d_sel) : 0;
	tr["value"] = trait_val_spin ? trait_val_spin->get_value() : 0.0;

	traits.push_back(tr);
	d["traits"] = traits;
	arr[rec_idx] = d;
	_refresh_traits_tree(traits);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_populate_effect_data_option(int p_effect_code) {
	if (!effect_data_opt) {
		return;
	}
	effect_data_opt->clear();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	switch (p_effect_code) {
		case CG_EFFECT_ADD_STATE:
		case CG_EFFECT_REMOVE_STATE:
			_populate_db_records_option(effect_data_opt, CAT_STATES, CG_DB_TR("(ninguno)"), false);
			break;
		case CG_EFFECT_ADD_BUFF:
		case CG_EFFECT_ADD_DEBUFF:
		case CG_EFFECT_REMOVE_BUFF:
		case CG_EFFECT_REMOVE_DEBUFF:
		case CG_EFFECT_GROW_PARAM:
			if (cat) {
				_populate_catalog_option(effect_data_opt, cat->get_base_param_names());
			}
			break;
		case CG_EFFECT_LEARN_SKILL:
			_populate_db_records_option(effect_data_opt, CAT_SKILLS, CG_DB_TR("(ninguna)"), false);
			break;
		case CG_EFFECT_COMMON_EVENT:
			_populate_db_records_option(effect_data_opt, CAT_COMMON_EVENTS, CG_DB_TR("(ninguno)"), false);
			break;
		default:
			effect_data_opt->add_item(CG_DB_TR("Directo (0)"), 0);
			break;
	}
}

void ClockGearsDatabaseDialog::_on_effect_type_changed(int p_idx) {
	if (!effect_type_opt) {
		return;
	}
	int code = effect_type_opt->get_item_id(p_idx);
	_populate_effect_data_option(code);
}

void ClockGearsDatabaseDialog::_refresh_effects_tree(const Array &p_effects) {
	if (!effects_tree) {
		return;
	}
	effects_tree->clear();
	TreeItem *root = effects_tree->create_item();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	Array e_defs = cat ? cat->get_effect_definitions() : Array();

	for (int i = 0; i < p_effects.size(); i++) {
		if (p_effects[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary ef = p_effects[i];
		int code = int(ef.get("code", CG_EFFECT_RECOVER_HP));
		int data_id = int(ef.get("data_id", 0));
		double val1 = double(ef.get("value1", 0.0));
		double val2 = double(ef.get("value2", 0.0));

		String type_name = vformat("Efecto #%d", code);
		for (int j = 0; j < e_defs.size(); j++) {
			Dictionary ed = e_defs[j];
			if (int(ed.get("code", 0)) == code) {
				type_name = ed.get("name", type_name);
				break;
			}
		}

		TreeItem *item = effects_tree->create_item(root);
		item->set_text(0, type_name);
		item->set_text(1, vformat("ID %d", data_id));
		item->set_text(2, vformat("%s / %s", String::num(val1, 0), String::num(val2, 0)));
		item->set_metadata(0, i);
	}
}

void ClockGearsDatabaseDialog::_on_add_effect_pressed() {
	if (!effect_dialog || !effect_type_opt) {
		return;
	}
	effect_type_opt->clear();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	if (cat) {
		Array e_defs = cat->get_effect_definitions();
		for (int i = 0; i < e_defs.size(); i++) {
			Dictionary ed = e_defs[i];
			effect_type_opt->add_item(ed.get("name", ""), int(ed.get("code", i)));
		}
	}
	if (effect_type_opt->get_item_count() > 0) {
		effect_type_opt->select(0);
		_populate_effect_data_option(effect_type_opt->get_item_id(0));
	}
	if (effect_val1_spin) {
		effect_val1_spin->set_value(0);
	}
	if (effect_val2_spin) {
		effect_val2_spin->set_value(50);
	}
	effect_dialog->popup_centered(Size2(420, 240) * EDSCALE);
}

void ClockGearsDatabaseDialog::_on_remove_effect_pressed() {
	if (!effects_tree || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	TreeItem *sel_item = effects_tree->get_selected();
	if (!sel_item) {
		return;
	}
	int e_idx = int(sel_item->get_metadata(0));
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) {
		return;
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	Array effects = d.get("effects", Array());
	if (e_idx >= 0 && e_idx < effects.size()) {
		effects.remove_at(e_idx);
		d["effects"] = effects;
		arr[rec_idx] = d;
		_refresh_effects_tree(effects);
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_effect_confirmed() {
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		if (arr.is_empty()) {
			arr.push_back(s_finalize_new_db_record(current_category, _create_empty_record(current_category, 1)));
			_refresh_record_list();
		}
		record_list->select(0);
		sel = record_list->get_selected_items();
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	Array effects = d.get("effects", Array());

	Dictionary ef;
	int e_sel = effect_type_opt ? effect_type_opt->get_selected() : -1;
	int d_sel = effect_data_opt ? effect_data_opt->get_selected() : -1;
	ef["code"] = (e_sel >= 0 && effect_type_opt) ? effect_type_opt->get_item_id(e_sel) : int(CG_EFFECT_RECOVER_HP);
	ef["data_id"] = (d_sel >= 0 && effect_data_opt) ? effect_data_opt->get_item_id(d_sel) : 0;
	ef["value1"] = effect_val1_spin ? effect_val1_spin->get_value() : 0.0;
	ef["value2"] = effect_val2_spin ? effect_val2_spin->get_value() : 0.0;

	effects.push_back(ef);
	d["effects"] = effects;
	arr[rec_idx] = d;
	_refresh_effects_tree(effects);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::sync_system_player_start(const String &p_map_path, int p_x, int p_y) {
	system_config["start_map"] = p_map_path;
	system_config["start_x"] = p_x;
	system_config["start_y"] = p_y;
	system_config["has_start_pos"] = !p_map_path.is_empty();
	if (current_category == CAT_SYSTEM) {
		updating_ui = true;
		if (sys_start_x_spin) {
			sys_start_x_spin->set_value(p_x);
		}
		if (sys_start_y_spin) {
			sys_start_y_spin->set_value(p_y);
		}
		_refresh_system_maps_and_party();
		updating_ui = false;
	}
}

void ClockGearsDatabaseDialog::_refresh_system_maps_and_party() {
	if (sys_start_map_option) {
		sys_start_map_option->clear();
		String cur_map = system_config.get("start_map", "");
		Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
		String active_scene_path = map ? map->get_scene_file_path() : "";

		Vector<String> map_paths;
		Vector<String> map_display_names;

		// 1. Escanear res://maps
		Ref<DirAccess> dir = DirAccess::open("res://maps");
		if (dir.is_valid()) {
			dir->list_dir_begin();
			String fn = dir->get_next();
			while (!fn.is_empty()) {
				if (!dir->current_is_dir() && fn.ends_with(".tscn")) {
					String full_path = "res://maps/" + fn;
					String map_title;
					Ref<FileAccess> fa = FileAccess::open(full_path, FileAccess::READ);
					if (fa.is_valid()) {
						for (int l = 0; l < 20 && !fa->eof_reached(); l++) {
							String line = fa->get_line().strip_edges();
							if (line.begins_with("map_name = \"") && line.ends_with("\"")) {
								map_title = line.substr(12, line.length() - 13);
								break;
							}
						}
					}
					map_paths.push_back(full_path);
					if (!map_title.is_empty()) {
						map_display_names.push_back(vformat("%s (%s)", map_title, fn));
					} else {
						map_display_names.push_back(fn);
					}
				}
				fn = dir->get_next();
			}
			dir->list_dir_end();
		}

		// 2. Si la escena activa es un ClockGearsMap y no está en res://maps
		if (!active_scene_path.is_empty() && !map_paths.has(active_scene_path)) {
			map_paths.push_back(active_scene_path);
			String t = map ? map->get_map_name() : "";
			if (!t.is_empty()) {
				map_display_names.push_back(vformat("%s (%s)", t, active_scene_path.get_file()));
			} else {
				map_display_names.push_back(active_scene_path.get_file());
			}
		}

		// 3. Si cur_map existe y no estaba en la lista
		if (!cur_map.is_empty() && !map_paths.has(cur_map) && FileAccess::exists(cur_map)) {
			map_paths.push_back(cur_map);
			map_display_names.push_back(cur_map.get_file());
		}

		int sel_idx = 0;
		if (map_paths.is_empty()) {
			sys_start_map_option->add_item(CG_DB_TR("(No hay mapas creados en el proyecto)"), 0);
			sys_start_map_option->set_item_metadata(0, String(""));
		} else {
			for (int i = 0; i < map_paths.size(); i++) {
				String item_text = vformat("%03d: %s", i + 1, map_display_names[i]);
				sys_start_map_option->add_item(item_text, i);
				sys_start_map_option->set_item_metadata(i, map_paths[i]);
				if (map_paths[i] == cur_map || (cur_map.is_empty() && map_paths[i] == active_scene_path)) {
					sel_idx = i;
				}
			}
			if (cur_map.is_empty() && sel_idx < map_paths.size()) {
				cur_map = map_paths[sel_idx];
				system_config["start_map"] = cur_map;
			}
		}
		sys_start_map_option->select(sel_idx);
	}

	if (sys_party_actor_opt) {
		_populate_db_records_option(sys_party_actor_opt, CAT_ACTORS, CG_DB_TR("(ninguno)"), true);
	}

	if (sys_party_list) {
		sys_party_list->clear();
		Array party = system_config.get("party", Array());
		const Array &actors = category_data[CAT_ACTORS];
		for (int i = 0; i < party.size(); i++) {
			int aid = int(party[i]);
			String aname = CG_DB_TR("(Actor eliminado)");
			for (int a = 0; a < actors.size(); a++) {
				Dictionary ad = actors[a];
				if (int(ad.get("id", a + 1)) == aid) {
					aname = ad.get("name", "");
					break;
				}
			}
			String role = (i == 0) ? CG_DB_TR("[Líder]") : vformat(CG_DB_TR("[Seguidor %d]"), i);
			sys_party_list->add_item(vformat("%s %03d: %s", role, aid, aname));
			sys_party_list->set_item_metadata(i, aid);
		}
	}
}

void ClockGearsDatabaseDialog::_on_add_party_member_pressed() {
	if (!sys_party_actor_opt || category_data[CAT_ACTORS].is_empty()) {
		return;
	}
	int sel = sys_party_actor_opt->get_selected();
	if (sel < 0) {
		return;
	}
	int actor_id = sys_party_actor_opt->get_item_id(sel);
	if (actor_id <= 0) {
		return;
	}
	Array party = system_config.get("party", Array());
	if (!party.has(actor_id) && party.size() < 9) {
		party.push_back(actor_id);
		system_config["party"] = party;
		_refresh_system_maps_and_party();
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_remove_party_member_pressed() {
	if (!sys_party_list) {
		return;
	}
	Vector<int> sel = sys_party_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	Array party = system_config.get("party", Array());
	if (sel[0] >= 0 && sel[0] < party.size()) {
		party.remove_at(sel[0]);
		system_config["party"] = party;
		_refresh_system_maps_and_party();
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_start_map_selected(int p_idx) {
	if (updating_ui || !sys_start_map_option) {
		return;
	}
	String map_path = sys_start_map_option->get_item_metadata(p_idx);
	system_config["start_map"] = map_path;
	system_config["has_start_pos"] = !map_path.is_empty();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_graphic_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
		_on_select_graphic_pressed();
	}
}

void ClockGearsDatabaseDialog::_on_select_graphic_pressed() {
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	selecting_tileset_sheet = false;
	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->clear_filters();

	String rel_folder = "img/system";
	String filter_desc = CG_DB_TR("Iconos y Gráficos de Sistema (*.png, *.webp)");
	switch (current_category) {
		case CAT_ACTORS:
		case CAT_NPCS:
		case CAT_ARPG_NPCS:
		case CAT_ARPG_ENEMIES:
		case CAT_FLORA:
		case CAT_FAUNA:
			rel_folder = "img/characters";
			filter_desc = CG_DB_TR("Sprites de Personajes 32x32 (*.png, *.webp)");
			break;
		case CAT_ENEMIES:
		case CAT_TROOPS:
			rel_folder = "img/battlers";
			filter_desc = CG_DB_TR("Battlers de Combate (*.png, *.webp)");
			break;
		case CAT_ANIMATIONS:
			rel_folder = "img/animations";
			filter_desc = CG_DB_TR("Hojas de Animación (*.png, *.webp)");
			break;
		case CAT_TILESETS:
			rel_folder = "img/tilesets";
			filter_desc = CG_DB_TR("Hojas de Tileset 32x32 (*.png, *.webp)");
			break;
		default:
			rel_folder = "img/system";
			filter_desc = CG_DB_TR("Iconos y Gráficos (*.png, *.webp)");
			break;
	}

	file_dialog->add_filter("*.png, *.webp ; " + filter_desc);
	String default_dir = ClockGearsProjectStructure::resolve_project_folder_global(rel_folder);
	if (!default_dir.is_empty()) {
		file_dialog->set_current_dir(default_dir);
	}
	file_dialog->set_title(vformat(CG_DB_TR("Seleccionar Gráfico (res://%s)"), rel_folder));
	file_dialog->popup_centered(Size2(680, 460));
}

void ClockGearsDatabaseDialog::_set_pending_changes(bool p_pending) {
	has_pending_changes = p_pending;
	if (btn_apply_changes) {
		btn_apply_changes->set_disabled(!has_pending_changes);
		btn_apply_changes->set_modulate(has_pending_changes ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(1.0f, 1.0f, 1.0f, 0.40f));
	}
}

void ClockGearsDatabaseDialog::_mark_database_dirty() {
	_set_pending_changes(true);
}

void ClockGearsDatabaseDialog::_on_db_apply_pressed() {
	int keep_cat = current_category;
	int keep_sel = -1;
	if (record_list) {
		Vector<int> sel = record_list->get_selected_items();
		if (!sel.is_empty()) {
			keep_sel = sel[0];
		}
	}
	double keep_scroll = (record_list && record_list->get_v_scroll_bar()) ? record_list->get_v_scroll_bar()->get_value() : 0.0;

	_save_database_to_disk();
	_set_pending_changes(false);

	if (ClockGearsMapPanel::get_singleton()) {
		ClockGearsMapPanel::get_singleton()->load_persisted_tilesets();
	}

	if (record_list && current_category == keep_cat && keep_sel >= 0 && keep_sel < record_list->get_item_count()) {
		record_list->select(keep_sel);
		if (record_list->get_v_scroll_bar()) {
			record_list->get_v_scroll_bar()->set_value(keep_scroll);
		}
	}
}

void ClockGearsDatabaseDialog::_on_db_ok_pressed() {
	_on_db_apply_pressed();
	hide();
}

bool ClockGearsDatabaseDialog::_has_visible_child_dialog() const {
	const Window *children[] = {
		tile_picker_dialog,
		file_dialog,
		asset_warning_dialog,
		change_max_dialog,
		action_pattern_dialog,
		trait_dialog,
		effect_dialog,
		troop_bg_dialog,
		troop_bg_file_dialog,
		troop_adjust_ui[0].dialog,
		troop_adjust_ui[1].dialog,
		battle_test_dialog,
		new_anim_dialog,
		rename_anim_dialog,
		sheet_importer_dialog,
		troop_ctx_menu,
		troop_scale_submenu,
		troop_hp_bar_submenu,
		record_event_editor,
		menu_asset_file_dialog,
		menu_test_window,
		ws_image_file_dialog,
	};
	for (const Window *w : children) {
		if (w && w->is_visible()) {
			return true;
		}
	}
	return false;
}

void ClockGearsDatabaseDialog::_input_from_window(const Ref<InputEvent> &p_event) {
	if (p_event.is_valid() && p_event->is_action_pressed(SNAME("ui_close_dialog"), false, true) && _has_visible_child_dialog()) {
		return;
	}
	AcceptDialog::_input_from_window(p_event);
}

void ClockGearsDatabaseDialog::cancel_pressed() {
	if (_has_visible_child_dialog()) {
		return;
	}
	AcceptDialog::cancel_pressed();
}

void ClockGearsDatabaseDialog::_restore_focus_after_subdialog() {
	if (!is_inside_tree() || !is_visible()) {
		return;
	}
	grab_focus();
	if (btn_pick_tile_graphic && btn_pick_tile_graphic->is_visible_in_tree()) {
		btn_pick_tile_graphic->grab_focus();
	} else if (btn_select_graphic && btn_select_graphic->is_visible_in_tree()) {
		btn_select_graphic->grab_focus();
	}
}

void ClockGearsDatabaseDialog::_on_tile_picker_closed() {
	if (tile_picker_dialog && tile_picker_dialog->is_visible()) {
		tile_picker_dialog->hide();
	}
	if (is_inside_tree() && is_visible()) {
		callable_mp(this, &ClockGearsDatabaseDialog::_restore_focus_after_subdialog).call_deferred();
	}
}

void ClockGearsDatabaseDialog::_clear_record_list_icon_cache() const {
	record_list_icon_cache.clear();
}

void ClockGearsDatabaseDialog::_invalidate_record_list_icon(int p_category, int p_index) const {
	// Keyed by content hash; clear only entries belonging to this category/record if needed.
	String prefix = vformat("%d|%d|", p_category, p_index);
	Vector<String> to_erase;
	for (const KeyValue<String, Ref<Texture2D>> &E : record_list_icon_cache) {
		if (E.key.begins_with(prefix)) {
			to_erase.push_back(E.key);
		}
	}
	for (int i = 0; i < to_erase.size(); i++) {
		record_list_icon_cache.erase(to_erase[i]);
	}
}

Ref<Texture2D> ClockGearsDatabaseDialog::_build_record_list_icon_texture(int p_category, const Dictionary &p_rec, int p_icon_px, const Ref<Texture2D> &p_fallback) const {
	if (!s_category_has_record_graphic(p_category) || p_icon_px <= 0) {
		return p_fallback;
	}

	Ref<Image> src_crop;
	String cache_key;

	if (p_category == CAT_ACTORS || p_category == CAT_ENEMIES) {
		Ref<Texture2D> sp_tex;
		Rect2 sp_reg;
		String sp_src;
		ClockGearsBattleScene::resolve_enemy_preview_sprite(p_rec, sp_tex, sp_reg, sp_src);
		if (sp_src == "placeholder" || sp_tex.is_null() || sp_tex->get_width() <= 0 || sp_tex->get_height() <= 0) {
			return p_fallback;
		}
		String sp_path = sp_tex->get_path().is_empty() ? String(p_rec.get("graphic_path", "")) : sp_tex->get_path();
		Rect2i full_r(0, 0, sp_tex->get_width(), sp_tex->get_height());
		Rect2i r = (sp_reg.size.x > 0 && sp_reg.size.y > 0)
				? Rect2i(int(sp_reg.position.x), int(sp_reg.position.y), int(sp_reg.size.x), int(sp_reg.size.y)).intersection(full_r)
				: full_r;
		if (r.size.x <= 0 || r.size.y <= 0) {
			return p_fallback;
		}
		cache_key = vformat("%d|%d|%s|%d,%d,%d,%d", p_category, p_icon_px, sp_path, r.position.x, r.position.y, r.size.x, r.size.y);
		if (record_list_icon_cache.has(cache_key)) {
			return record_list_icon_cache[cache_key];
		}
		Ref<Image> img = sp_tex->get_image();
		if (img.is_null() || img->is_empty()) {
			return p_fallback;
		}
		r = r.intersection(Rect2i(0, 0, img->get_width(), img->get_height()));
		if (r.size.x <= 0 || r.size.y <= 0) {
			return p_fallback;
		}
		src_crop = img->get_region(r);
	} else if (s_is_anim_slot_category(p_category)) {
		Dictionary sp_dict = p_rec.get("sprite_data", Dictionary());
		ClockGearsSpriteSheetData sd;
		sd.from_dict(sp_dict);
		if (!sd.animations.has("mover_abajo") || sd.animations["mover_abajo"].get_frame_count() <= 0) {
			return p_fallback;
		}
		const ClockGearsSpriteAnimation &mv = sd.animations["mover_abajo"];
		ClockGearsAnimationFrame raw_f0 = mv.get_frame(0, 0, 0);
		String sp_path = raw_f0.sheet_path.is_empty() ? mv.spritesheet_path : raw_f0.sheet_path;
		if (sp_path.is_empty()) {
			sp_path = String(p_rec.get("graphic_path", ""));
		}
		if (sp_path.is_empty()) {
			return p_fallback;
		}
		Ref<Texture2D> sp_tex = _load_texture_from_path(sp_path);
		if (sp_tex.is_null() || sp_tex->get_width() <= 0 || sp_tex->get_height() <= 0) {
			return p_fallback;
		}
		ClockGearsAnimationFrame f0 = mv.get_frame(0, sp_tex->get_width(), sp_tex->get_height());
		Rect2i full_r(0, 0, sp_tex->get_width(), sp_tex->get_height());
		Rect2i r = (f0.region.size.x > 0 && f0.region.size.y > 0)
				? f0.region.intersection(full_r)
				: full_r;
		if (r.size.x <= 0 || r.size.y <= 0) {
			return p_fallback;
		}
		cache_key = vformat("%d|%d|%s|%d,%d,%d,%d", p_category, p_icon_px, sp_path, r.position.x, r.position.y, r.size.x, r.size.y);
		if (record_list_icon_cache.has(cache_key)) {
			return record_list_icon_cache[cache_key];
		}
		Ref<Image> img = sp_tex->get_image();
		if (img.is_null() || img->is_empty()) {
			return p_fallback;
		}
		r = r.intersection(Rect2i(0, 0, img->get_width(), img->get_height()));
		if (r.size.x <= 0 || r.size.y <= 0) {
			return p_fallback;
		}
		src_crop = img->get_region(r);
	} else {
		String gpath = String(p_rec.get("graphic_path", "")).strip_edges();
		if (gpath.is_empty()) {
			return p_fallback;
		}
		Vector2i tc;
		bool has_tc = s_parse_tile_coords_variant(p_rec.get("tile_coords", Variant()), tc);
		Vector2i blk = has_tc ? s_parse_tile_block_variant(p_rec.get("tile_block_size", Variant())) : Vector2i(1, 1);
		cache_key = has_tc
				? vformat("%d|%d|%s|%d,%d|%dx%d", p_category, p_icon_px, gpath, tc.x, tc.y, blk.x, blk.y)
				: vformat("%d|%d|%s|full", p_category, p_icon_px, gpath);
		if (record_list_icon_cache.has(cache_key)) {
			return record_list_icon_cache[cache_key];
		}
		Ref<Texture2D> tex = _load_texture_from_path(gpath);
		if (tex.is_null()) {
			return p_fallback;
		}
		Ref<Image> img = tex->get_image();
		if (img.is_null() || img->is_empty()) {
			return p_fallback;
		}
		if (has_tc) {
			const int tsz = 32;
			Rect2i r(tc.x * tsz, tc.y * tsz, blk.x * tsz, blk.y * tsz);
			r = r.intersection(Rect2i(0, 0, img->get_width(), img->get_height()));
			if (r.size.x <= 0 || r.size.y <= 0) {
				return p_fallback;
			}
			src_crop = img->get_region(r);
		} else {
			src_crop = img->duplicate();
		}
	}

	if (src_crop.is_null() || src_crop->is_empty()) {
		return p_fallback;
	}
	int sw = src_crop->get_width();
	int sh = src_crop->get_height();
	if (sw <= 0 || sh <= 0) {
		return p_fallback;
	}

	int dw = sw;
	int dh = sh;
	if (sw <= p_icon_px && sh <= p_icon_px) {
		int fit_mult = MAX(1, MIN(p_icon_px / sw, p_icon_px / sh));
		int desired_mult = MAX(1, p_icon_px / 32);
		int mult = CLAMP(desired_mult, 1, fit_mult);
		dw = sw * mult;
		dh = sh * mult;
	} else {
		int div = 2;
		while ((sw / div) > p_icon_px || (sh / div) > p_icon_px) {
			div++;
		}
		int cand_w = sw / div;
		int cand_h = sh / div;
		if ((cand_w >= 1 && cand_h >= 1 && sw % div == 0 && sh % div == 0) || (cand_w >= 8 && cand_h >= 8)) {
			dw = cand_w;
			dh = cand_h;
		} else {
			float s = MIN(float(p_icon_px) / float(sw), float(p_icon_px) / float(sh));
			dw = MAX(1, int(Math::round(sw * s)));
			dh = MAX(1, int(Math::round(sh * s)));
		}
	}

	if (src_crop->get_format() != Image::FORMAT_RGBA8) {
		src_crop->convert(Image::FORMAT_RGBA8);
	}
	if (dw != sw || dh != sh) {
		src_crop->resize(dw, dh, Image::INTERPOLATE_NEAREST);
	}

	Ref<Image> canvas = Image::create_empty(p_icon_px, p_icon_px, false, Image::FORMAT_RGBA8);
	canvas->fill(Color(0, 0, 0, 0));
	canvas->blit_rect(src_crop, Rect2i(0, 0, dw, dh), Point2i((p_icon_px - dw) / 2, (p_icon_px - dh) / 2));

	Ref<ImageTexture> out = ImageTexture::create_from_image(canvas);
	if (out.is_null()) {
		return p_fallback;
	}
	record_list_icon_cache[cache_key] = out;
	return out;
}

void ClockGearsDatabaseDialog::_update_record_list_item_icon(int p_index) {
	if (!record_list || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	const Array &arr = category_data[current_category];
	if (p_index < 0 || p_index >= arr.size() || p_index >= record_list->get_item_count()) {
		return;
	}
	const int icon_scale = ClockGearsIcons::get_editor_icon_scale(this);
	const int icon_px = 32 * icon_scale;
	Ref<Texture2D> cat_icon;
	if (ClockGearsIcons::get_singleton()) {
		cat_icon = ClockGearsIcons::get_singleton()->get_db_category_icon(current_category, float(icon_scale));
	}
	Ref<Texture2D> row_icon = _build_record_list_icon_texture(current_category, Dictionary(arr[p_index]), icon_px, cat_icon);
	record_list->set_item_icon(p_index, row_icon);
}

void ClockGearsDatabaseDialog::_refit_graphic_path_label() {
	if (!graphic_path_label) {
		return;
	}
	String full = graphic_path_full_text;
	if (full.is_empty()) {
		graphic_path_label->set_text("");
		return;
	}
	Ref<Font> font = graphic_path_label->get_theme_font("font");
	int font_size = graphic_path_label->get_theme_font_size("font_size");
	float avail_w = graphic_path_label->get_size().x;
	if (avail_w <= 16.0f && standard_graphic_box) {
		avail_w = MAX(148.0f * EDSCALE, standard_graphic_box->get_custom_minimum_size().x - 8.0f * EDSCALE);
	}
	if (avail_w <= 16.0f) {
		avail_w = 160.0f * EDSCALE;
	}
	graphic_path_label->set_text(s_middle_ellipsize_for_2lines(full, font, font_size, avail_w));
}

void ClockGearsDatabaseDialog::_set_graphic_path_label_text(const String &p_full_text) {
	graphic_path_full_text = p_full_text;
	if (!graphic_path_label) {
		return;
	}
	graphic_path_label->set_tooltip_text(p_full_text.is_empty() ? CG_DB_TR("Ruta del gráfico asignado. Doble clic para asignar.") : p_full_text);
	_refit_graphic_path_label();
}

void ClockGearsDatabaseDialog::_update_graphic_preview_ui(const Dictionary &p_rec) {
	String gpath = String(p_rec.get("graphic_path", "")).strip_edges();
	Ref<Texture2D> tex = gpath.is_empty() ? Ref<Texture2D>() : _load_texture_from_path(gpath);
	Vector2i tc;
	bool has_tc = s_parse_tile_coords_variant(p_rec.get("tile_coords", Variant()), tc);

	if (has_tc && tex.is_valid()) {
		Vector2i blk = s_parse_tile_block_variant(p_rec.get("tile_block_size", Variant()));
		Ref<Image> src_img = tex->get_image();
		bool cropped_ok = false;
		if (src_img.is_valid() && !src_img->is_empty()) {
			const int tsz = 32;
			Rect2i reg(tc.x * tsz, tc.y * tsz, blk.x * tsz, blk.y * tsz);
			reg = reg.intersection(Rect2i(0, 0, src_img->get_width(), src_img->get_height()));
			if (reg.size.x > 0 && reg.size.y > 0) {
				Ref<Image> sub = src_img->get_region(reg);
				if (graphic_preview) {
					graphic_preview->set_texture(ImageTexture::create_from_image(sub));
				}
				cropped_ok = true;
			}
		}
		if (!cropped_ok && graphic_preview) {
			graphic_preview->set_texture(tex);
		}
		_set_graphic_path_label_text(vformat("%s [%d,%d] (%dx%d)", gpath.get_file(), tc.x, tc.y, blk.x, blk.y));
	} else {
		if (graphic_preview) {
			graphic_preview->set_texture(tex);
		}
		_set_graphic_path_label_text(gpath.is_empty() ? CG_DB_TR("(Sin gráfico)") : gpath.get_file());
	}
}

void ClockGearsDatabaseDialog::_apply_record_graphic(int p_index, const String &p_path, bool p_has_tile, const Vector2i &p_tile_coords, const Vector2i &p_tile_block) {
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	if (p_index < 0 || p_index >= arr.size()) {
		return;
	}
	Dictionary d = arr[p_index];
	d["graphic_path"] = p_path;
	if (p_has_tile) {
		d["tile_coords"] = s_tile_vec_to_json_array(p_tile_coords);
		d["tile_block_size"] = s_tile_vec_to_json_array(Vector2i(MAX(1, p_tile_block.x), MAX(1, p_tile_block.y)));
	} else {
		d.erase("tile_coords");
		d.erase("tile_block_size");
	}
	arr[p_index] = d;

	_update_graphic_preview_ui(d);
	_update_record_list_item_icon(p_index);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_pick_tile_graphic_pressed() {
	if (!tile_picker_dialog) {
		return;
	}
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	int idx = sel[0];
	const Array &arr = category_data[current_category];
	String atlas_path;
	Vector2i init_coords(-1, -1);
	Vector2i init_block(1, 1);
	Ref<Texture2D> tex;
	if (idx >= 0 && idx < arr.size()) {
		Dictionary d = arr[idx];
		atlas_path = String(d.get("graphic_path", "")).strip_edges();
		if (!atlas_path.is_empty()) {
			tex = _load_texture_from_path(atlas_path);
		}
		Vector2i parsed_tc;
		if (s_parse_tile_coords_variant(d.get("tile_coords", Variant()), parsed_tc)) {
			init_coords = parsed_tc;
			init_block = s_parse_tile_block_variant(d.get("tile_block_size", Variant()));
		}
	}
	if (tex.is_null() && !category_data[CAT_TILESETS].is_empty()) {
		Dictionary ts0 = category_data[CAT_TILESETS][0];
		String fb_path = String(ts0.get("graphic_path", "")).strip_edges();
		if (fb_path.is_empty() && ts0.has("slots") && ts0["slots"].get_type() == Variant::DICTIONARY) {
			Dictionary slots = ts0["slots"];
			const char *slot_order[] = { "B", "C", "D", "E", "A5", "A1", "A2", "A3", "A4" };
			for (size_t s = 0; s < 9; s++) {
				String sp = String(slots.get(slot_order[s], "")).strip_edges();
				if (!sp.is_empty()) {
					fb_path = sp;
					break;
				}
			}
		}
		if (!fb_path.is_empty()) {
			atlas_path = fb_path;
			tex = _load_texture_from_path(atlas_path);
		}
	}
	tile_picker_dialog->open_picker(tex, callable_mp(this, &ClockGearsDatabaseDialog::_on_tile_graphic_picked), atlas_path, init_coords, init_block);
}

void ClockGearsDatabaseDialog::_on_tile_graphic_picked(const Array &p_sel, const Vector2i &p_block) {
	if (!tile_picker_dialog || !tile_picker_dialog->get_picker()) {
		return;
	}
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	int idx = sel[0];
	Array &arr = category_data[current_category];
	if (idx < 0 || idx >= arr.size()) {
		return;
	}
	ClockGearsTilePicker *pk = tile_picker_dialog->get_picker();
	Ref<Texture2D> atlas_tex = pk->get_atlas_texture();
	if (atlas_tex.is_null()) {
		return;
	}
	Vector2i primary = pk->get_primary_selected_tile();
	if (!p_sel.is_empty() && p_sel[0].get_type() == Variant::DICTIONARY) {
		Dictionary first_sel = p_sel[0];
		if (first_sel.has("atlas_coords")) {
			Vector2i parsed;
			if (s_parse_tile_coords_variant(first_sel["atlas_coords"], parsed)) {
				primary = parsed;
			}
		}
	}
	String atlas_path = pk->get_atlas_path().strip_edges();
	if (atlas_path.is_empty()) {
		atlas_path = atlas_tex->get_path().strip_edges();
	}
	if (atlas_path.is_empty()) {
		Dictionary cur_d = arr[idx];
		atlas_path = String(cur_d.get("graphic_path", "")).strip_edges();
	}
	_apply_record_graphic(idx, atlas_path, true, primary, p_block);
}

int ClockGearsDatabaseDialog::_ensure_active_tileset_record() {
	Array &arr = category_data[CAT_TILESETS];
	if (arr.is_empty()) {
		Dictionary rec = _create_empty_record(CAT_TILESETS, 1);
		rec["name"] = "Tileset 001";
		arr.push_back(rec);
		print_line(String::utf8("[ClockGears DB Tilesets] Lista de Tilesets vacía -> auto-creando registro '001: Tileset 001'."));
		bool prev_upd = updating_ui;
		updating_ui = true;
		_refresh_record_list();
		if (record_list && record_list->get_item_count() > 0) {
			record_list->select(0);
		}
		if (name_edit && current_category == CAT_TILESETS) {
			name_edit->set_text("Tileset 001");
		}
		if (btn_remove_record) {
			btn_remove_record->set_disabled(false);
		}
		if (btn_copy_record) {
			btn_copy_record->set_disabled(false);
		}
		updating_ui = prev_upd;
		return 0;
	}
	if (record_list) {
		Vector<int> sel = record_list->get_selected_items();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < arr.size()) {
			return sel[0];
		}
		if (record_list->get_item_count() > 0) {
			record_list->select(0);
		}
	}
	return 0;
}

void ClockGearsDatabaseDialog::_refresh_tileset_slots_ui() {
	int group_idx = (active_tileset_slot_index <= 4) ? 0 : (active_tileset_slot_index - 4);
	for (int g = 0; g < 5; g++) {
		if (btn_ts_group_tab[g]) {
			btn_ts_group_tab[g]->set_pressed_no_signal(g == group_idx);
		}
	}
	if (tileset_a_subtabs_box) {
		tileset_a_subtabs_box->set_visible(group_idx == 0);
	}
	for (int a = 0; a < 5; a++) {
		if (btn_ts_a_subtab[a]) {
			btn_ts_a_subtab[a]->set_pressed_no_signal(a == active_a_subtab_index);
		}
	}

	String active_key = _get_slot_key(active_tileset_slot_index);
	if (active_slot_info_label) {
		active_slot_info_label->set_text(vformat(CG_DB_TR("Ranura activa: %s"), active_key));
		active_slot_info_label->set_tooltip_text(vformat(CG_DB_TR("Ranura Activa: %s (Rejilla 32x32 Anexo A4)"), active_key));
	}
	if (btn_load_tileset_sheet) {
		btn_load_tileset_sheet->set_text(vformat(CG_DB_TR("Cargar Hoja en Ranura %s..."), active_key));
		btn_load_tileset_sheet->set_disabled(false);
	}

	Array &arr = category_data[CAT_TILESETS];
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	int idx = (!sel.is_empty() && sel[0] >= 0 && sel[0] < arr.size()) ? sel[0] : -1;

	if (idx < 0) {
		for (int s = 0; s < TILESET_SLOT_COUNT; s++) {
			if (tileset_slot_edits[s]) {
				tileset_slot_edits[s]->set_text(CG_DB_TR("(Vacío)"));
			}
			if (btn_load_slot[s]) {
				btn_load_slot[s]->set_disabled(false);
			}
			if (btn_clear_slot[s]) {
				btn_clear_slot[s]->set_disabled(true);
			}
		}
		if (db_tile_picker) {
			db_tile_picker->set_atlas_texture(Ref<Texture2D>());
			db_tile_picker->set_tile_metadata_map(Dictionary());
		}
		if (tileset_collision_editor) {
			tileset_collision_editor->set_tileset_texture(Ref<Texture2D>());
			tileset_collision_editor->set_collision_map(Dictionary());
		}
		return;
	}

	Dictionary d = arr[idx];
	Dictionary slots = d.get("slots", Dictionary());
	Dictionary slot_cols = d.get("slot_collisions", Dictionary());

	// Compatibilidad con registros que solo tenían graphic_path
	String legacy_path = d.get("graphic_path", "");
	if (slots.is_empty() && !legacy_path.is_empty()) {
		slots["A5"] = legacy_path;
		slot_cols["A5"] = d.get("collisions", Dictionary());
		d["slots"] = slots;
		d["slot_collisions"] = slot_cols;
		arr[idx] = d;
	}

	for (int s = 0; s < TILESET_SLOT_COUNT; s++) {
		String sk = _get_slot_key(s);
		String spath = slots.get(sk, "");
		if (tileset_slot_edits[s]) {
			tileset_slot_edits[s]->set_text(spath.is_empty() ? CG_DB_TR("(Vacío)") : spath.get_file());
			tileset_slot_edits[s]->set_tooltip_text(spath);
		}
		if (btn_load_slot[s]) {
			btn_load_slot[s]->set_disabled(false);
		}
		if (btn_clear_slot[s]) {
			btn_clear_slot[s]->set_disabled(spath.is_empty());
		}
	}

	String active_path = slots.get(active_key, "");
	Dictionary active_cols = slot_cols.get(active_key, Dictionary());
	Ref<Texture2D> tex = _load_texture_from_path(active_path);

	print_line(vformat(String::utf8("[ClockGears DB Tilesets] Refrescando visor: Ranura '%s' | Ruta='%s' | Metadatos=%d entradas"),
			active_key, active_path.is_empty() ? CG_DB_TR("(Vacío)") : active_path, active_cols.size()));

	if (db_tile_picker) {
		db_tile_picker->set_slot_key(active_key);
		db_tile_picker->set_atlas_texture(tex, active_path);
		db_tile_picker->set_tile_metadata_map(active_cols);
	}
	if (tileset_collision_editor) {
		tileset_collision_editor->set_tileset_texture(tex);
		tileset_collision_editor->set_collision_map(active_cols);
	}
	callable_mp(this, &ClockGearsDatabaseDialog::_update_tileset_split_layout).call_deferred();
}

void ClockGearsDatabaseDialog::_update_tileset_split_layout() {
	if (current_category != CAT_TILESETS || !tileset_main_hbox || !tileset_left_col || !tileset_collision_panel || !db_tile_picker) {
		return;
	}
	float total_w = tileset_main_hbox->get_size().x;
	if (total_w <= 10.0f) {
		return;
	}
	float sep = float(tileset_main_hbox->get_theme_constant("separation"));
	float left_min = tileset_left_col->get_combined_minimum_size().x;
	float max_right = MAX(320.0f * EDSCALE, total_w - left_min - sep);
	float sheet_w = db_tile_picker->get_atlas_draw_width();
	// 8+8 px panel padding + 2 px border + ~12 px v_scroll + 4 px holgura
	float needed_right = (sheet_w > 0.0f) ? (sheet_w + 34.0f * EDSCALE) : (340.0f * EDSCALE);
	float default_right = (total_w - sep) * 0.64f;

	if (sheet_w > 0.0f && needed_right > default_right) {
		float right_min = CLAMP(needed_right, 340.0f * EDSCALE, max_right);
		tileset_collision_panel->set_custom_minimum_size(Size2(right_min, 0));
		float left_extra = MAX(0.0f, total_w - sep - right_min - left_min);
		tileset_left_col->set_h_size_flags(left_extra > 4.0f ? Control::SIZE_EXPAND_FILL : Control::SIZE_FILL);
		tileset_left_col->set_stretch_ratio(0.15f);
		tileset_collision_panel->set_stretch_ratio(0.85f);
	} else {
		tileset_collision_panel->set_custom_minimum_size(Size2(MIN(340.0f * EDSCALE, max_right), 0));
		tileset_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		tileset_left_col->set_stretch_ratio(0.36f);
		tileset_collision_panel->set_stretch_ratio(0.64f);
	}
}

void ClockGearsDatabaseDialog::_on_tileset_group_tab_pressed(int p_group_idx) {
	if (p_group_idx == 0) {
		active_tileset_slot_index = CLAMP(active_a_subtab_index, 0, 4);
	} else {
		active_tileset_slot_index = CLAMP(p_group_idx + 4, 5, 8);
	}
	_refresh_tileset_slots_ui();
}

void ClockGearsDatabaseDialog::_on_tileset_a_subtab_pressed(int p_sub_idx) {
	active_a_subtab_index = CLAMP(p_sub_idx, 0, 4);
	active_tileset_slot_index = active_a_subtab_index;
	_refresh_tileset_slots_ui();
}

void ClockGearsDatabaseDialog::_on_load_slot_button_pressed(int p_slot_idx) {
	active_tileset_slot_index = CLAMP(p_slot_idx, 0, TILESET_SLOT_COUNT - 1);
	if (active_tileset_slot_index <= 4) {
		active_a_subtab_index = active_tileset_slot_index;
	}
	_refresh_tileset_slots_ui();
	_on_load_tileset_sheet_pressed();
}

void ClockGearsDatabaseDialog::_on_clear_slot_button_pressed(int p_slot_idx) {
	clear_tileset_slot(_get_slot_key(p_slot_idx));
}

void ClockGearsDatabaseDialog::_on_load_tileset_sheet_pressed() {
	int rec_idx = _ensure_active_tileset_record();
	pending_load_slot_index = active_tileset_slot_index;
	String slot_key = _get_slot_key(pending_load_slot_index);
	print_line(vformat(String::utf8("[ClockGears DB Tilesets] Abriendo selector de archivos para Tileset #%03d, Ranura '%s'..."), rec_idx + 1, slot_key));

	selecting_tileset_sheet = true;
	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->clear_filters();
	file_dialog->add_filter("*.png, *.webp, *.tres, *.res ; Hojas de Tileset 32x32 (*.png, *.webp, *.tres)");
	String ts_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/tilesets");
	if (!ts_dir.is_empty()) {
		file_dialog->set_current_dir(ts_dir);
	}
	file_dialog->set_title(vformat(CG_DB_TR("Cargar Hoja de Tileset en Ranura %s (res://img/tilesets)"), slot_key));
	file_dialog->popup_centered(Size2(680, 460));
}

void ClockGearsDatabaseDialog::_on_file_selected(const String &p_path) {
	if (selecting_tileset_sheet || current_category == CAT_TILESETS) {
		int slot_idx = (pending_load_slot_index >= 0) ? pending_load_slot_index : active_tileset_slot_index;
		pending_load_slot_index = -1;
		import_tileset_slot(_get_slot_key(slot_idx), p_path);
		return;
	}

	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	int idx = sel[0];
	Array &arr = category_data[current_category];
	if (idx < 0 || idx >= arr.size()) {
		return;
	}

	Dictionary d = arr[idx];
	ClockGearsProjectStructure::write_pixel_art_import_preset(p_path);

	String target_folder = (s_is_anim_slot_category(current_category) || current_category == CAT_FLORA || current_category == CAT_FAUNA)
			? "img/characters"
			: "img/system";
	String warn_32_msg;
	if (!ClockGearsProjectStructure::validate_folder_asset(p_path, target_folder, warn_32_msg)) {
		if (asset_warning_dialog) {
			asset_warning_dialog->set_text(warn_32_msg);
			asset_warning_dialog->popup_centered();
		}
	}

	Ref<Texture2D> tex = _load_texture_from_path(p_path);
	if (tex.is_valid() && ClockGearsPixelArt::get_singleton()) {
		Dictionary val = ClockGearsPixelArt::get_singleton()->validate_texture_dimensions(tex, false);
		if (!bool(val.get("valid", true))) {
			PackedStringArray warns = val.get("warnings", PackedStringArray());
			for (int w = 0; w < warns.size(); w++) {
				WARN_PRINT(vformat("ClockGearsPixelArt [%s]: %s", p_path.get_file(), warns[w]));
			}
		}
	}

	if (s_is_anim_slot_category(current_category) || current_category == CAT_ENEMIES) {
		String cur_anim_name = _get_active_anim_name();
		if (cur_anim_name.is_empty()) {
			cur_anim_name = "idle";
		}
		if (!current_sprite_data.animations.has(cur_anim_name)) {
			ClockGearsSpriteAnimation a;
			a.name = cur_anim_name;
			current_sprite_data.animations[cur_anim_name] = a;
		}
		ClockGearsSpriteAnimation &cur_a = current_sprite_data.animations[cur_anim_name];
		cur_a.spritesheet_path = p_path;
		if (cur_anim_name == "idle" || String(d.get("graphic_path", "")).is_empty()) {
			d["graphic_path"] = p_path;
		}

		_refresh_frames_timeline();
		_refresh_anim_preview();
		d["sprite_data"] = current_sprite_data.to_dict();
		d["animations"] = current_sprite_data.to_dict().get("animations", Dictionary());
		if (s_is_arpg_equip_category(current_category)) {
			if (arpg_draw_mode_opt) {
				d["draw_mode"] = (arpg_draw_mode_opt->get_selected() == 1) ? "simple" : "completo";
			}
			if (arpg_layer_order_spin) {
				d["layer_order"] = int(arpg_layer_order_spin->get_value());
			}
		}
		d.erase("tile_coords");
		d.erase("tile_block_size");
		arr[idx] = d;
		category_data[current_category] = arr;
		_update_graphic_preview_ui(d);
		_update_record_list_item_icon(idx);
		_mark_database_dirty();
	} else {
		_apply_record_graphic(idx, p_path, false, Vector2i(), Vector2i(1, 1));
	}
}

void ClockGearsDatabaseDialog::_on_tileset_collision_changed() {
	if (current_category != CAT_TILESETS) {
		return;
	}
	int idx = _ensure_active_tileset_record();
	Array &arr = category_data[CAT_TILESETS];
	if (idx < 0 || idx >= arr.size()) {
		return;
	}
	Dictionary d = arr[idx];
	Dictionary slot_cols = d.get("slot_collisions", Dictionary());
	String slot_key = _get_slot_key(active_tileset_slot_index);
	Dictionary cur_meta;
	if (db_tile_picker) {
		cur_meta = db_tile_picker->get_tile_metadata_map();
	} else if (tileset_collision_editor) {
		cur_meta = tileset_collision_editor->get_collision_map();
	}
	slot_cols[slot_key] = cur_meta.duplicate(true);
	d["slot_collisions"] = slot_cols;
	d["collisions"] = cur_meta.duplicate(true);
	arr[idx] = d;
	_save_database_to_disk();
	if (ClockGearsMapPanel::get_singleton()) {
		ClockGearsMapPanel::get_singleton()->load_persisted_tilesets();
	}
	Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
	if (map) {
		map->setup_tileset_from_database(map->get_tileset_id() > 0 ? map->get_tileset_id() : 1);
	}
}

void ClockGearsDatabaseDialog::_on_window_size_changed() {
	Size2i cur_sz = get_size();
	Size2i min_sz = get_min_size();
	if (cur_sz.x >= min_sz.x && cur_sz.y >= min_sz.y) {
		_save_editor_ui_state();
	}
	adapt_to_window_size(cur_sz);
	if (current_category == CAT_TILESETS) {
		callable_mp(this, &ClockGearsDatabaseDialog::_update_tileset_split_layout).call_deferred();
	}
}

void ClockGearsDatabaseDialog::select_category(int p_category) {
	_on_category_selected(p_category);
}

int ClockGearsDatabaseDialog::add_tileset_record(const String &p_name) {
	select_category(CAT_TILESETS);
	_on_add_record_pressed();
	Array &arr = category_data[CAT_TILESETS];
	int idx = arr.size() - 1;
	if (idx >= 0 && !p_name.is_empty()) {
		Dictionary d = arr[idx];
		d["name"] = p_name;
		arr[idx] = d;
		if (name_edit) {
			name_edit->set_text(p_name);
		}
		_refresh_record_list();
		if (record_list) {
			record_list->select(idx);
		}
		_save_database_to_disk();
	}
	return idx;
}

void ClockGearsDatabaseDialog::remove_tileset_record(int p_index) {
	select_category(CAT_TILESETS);
	if (p_index >= 0 && record_list && p_index < record_list->get_item_count()) {
		record_list->select(p_index);
	}
	_on_remove_record_pressed();
}

int ClockGearsDatabaseDialog::get_tileset_count() const {
	return category_data[CAT_TILESETS].size();
}

void ClockGearsDatabaseDialog::select_tileset_slot(const String &p_slot_key) {
	int s_idx = _find_slot_index(p_slot_key);
	active_tileset_slot_index = s_idx;
	if (s_idx <= 4) {
		active_a_subtab_index = s_idx;
	}
	_refresh_tileset_slots_ui();
}

String ClockGearsDatabaseDialog::get_active_tileset_slot() const {
	return _get_slot_key(active_tileset_slot_index);
}

bool ClockGearsDatabaseDialog::import_tileset_slot(const String &p_slot_key, const String &p_file_path) {
	int slot_idx = _find_slot_index(p_slot_key);
	String slot_key = _get_slot_key(slot_idx);
	active_tileset_slot_index = slot_idx;
	if (slot_idx <= 4) {
		active_a_subtab_index = slot_idx;
	}

	print_line(vformat(String::utf8("[ClockGears DB Tilesets] Importando hoja '%s' en ranura '%s'..."), p_file_path, slot_key));

	// Compartir el código de importación con la paleta del mapa (ClockGearsTilePicker::import_tileset_resource)
	Ref<Texture2D> tex;
	Ref<TileSet> ts;
	String res_path;
	String err_msg;
	if (!ClockGearsTilePicker::import_tileset_resource(p_file_path, tex, ts, res_path, err_msg, 32)) {
		print_line(vformat(String::utf8("[ClockGears DB Tilesets] ERROR al importar en ranura '%s': %s"), slot_key, err_msg));
		if (asset_warning_dialog) {
			asset_warning_dialog->set_title(CG_DB_TR("Error al Cargar Hoja de Tileset - ClockGears Engine"));
			asset_warning_dialog->set_text(err_msg);
			if (is_inside_tree()) {
				asset_warning_dialog->popup_centered(Size2(440, 160));
			}
		}
		return false;
	}

	int rec_idx = _ensure_active_tileset_record();
	Array &arr = category_data[CAT_TILESETS];
	Dictionary d = arr[rec_idx];
	Dictionary slots = d.get("slots", Dictionary());
	Dictionary slot_cols = d.get("slot_collisions", Dictionary());
	slots[slot_key] = res_path;
	if (!slot_cols.has(slot_key)) {
		slot_cols[slot_key] = Dictionary();
	}
	d["slots"] = slots;
	d["slot_collisions"] = slot_cols;
	d["graphic_path"] = res_path;
	arr[rec_idx] = d;

	_refresh_tileset_slots_ui();
	_save_database_to_disk();

	if (ClockGearsMapPanel::get_singleton()) {
		ClockGearsMapPanel::get_singleton()->load_persisted_tilesets();
	}

	print_line(vformat(String::utf8("[ClockGears DB Tilesets] ÉXITO: Ranura '%s' del Tileset #%03d actualizada con '%s'."), slot_key, rec_idx + 1, res_path));
	return true;
}

void ClockGearsDatabaseDialog::clear_tileset_slot(const String &p_slot_key) {
	int slot_idx = _find_slot_index(p_slot_key);
	String slot_key = _get_slot_key(slot_idx);
	Array &arr = category_data[CAT_TILESETS];
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	int rec_idx = (!sel.is_empty() && sel[0] >= 0 && sel[0] < arr.size()) ? sel[0] : (arr.is_empty() ? -1 : 0);
	if (rec_idx < 0) {
		return;
	}
	Dictionary d = arr[rec_idx];
	Dictionary slots = d.get("slots", Dictionary());
	Dictionary slot_cols = d.get("slot_collisions", Dictionary());
	slots[slot_key] = "";
	slot_cols[slot_key] = Dictionary();
	d["slots"] = slots;
	d["slot_collisions"] = slot_cols;
	if (String(d.get("graphic_path", "")) != "" && active_tileset_slot_index == slot_idx) {
		d["graphic_path"] = "";
	}
	arr[rec_idx] = d;
	print_line(vformat(String::utf8("[ClockGears DB Tilesets] Ranura '%s' vaciada en Tileset #%03d."), slot_key, rec_idx + 1));
	_refresh_tileset_slots_ui();
	_save_database_to_disk();
	if (ClockGearsMapPanel::get_singleton()) {
		ClockGearsMapPanel::get_singleton()->load_persisted_tilesets();
	}
	Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
	if (map) {
		map->setup_tileset_from_database(map->get_tileset_id() > 0 ? map->get_tileset_id() : 1);
	}
}

String ClockGearsDatabaseDialog::get_tileset_slot_path(int p_record_idx, const String &p_slot_key) const {
	const Array &arr = category_data[CAT_TILESETS];
	if (p_record_idx < 0 || p_record_idx >= arr.size()) {
		return "";
	}
	Dictionary d = arr[p_record_idx];
	Dictionary slots = d.get("slots", Dictionary());
	String sk = _get_slot_key(_find_slot_index(p_slot_key));
	return slots.get(sk, "");
}

void ClockGearsDatabaseDialog::set_tileset_slot_tile_meta(const String &p_slot_key, const Vector2i &p_coords, const Dictionary &p_meta) {
	select_tileset_slot(p_slot_key);
	_ensure_active_tileset_record();
	if (db_tile_picker) {
		db_tile_picker->set_tile_metadata(p_coords, p_meta);
	}
}

void ClockGearsDatabaseDialog::drag_paint_tiles_meta(const String &p_slot_key, const Vector2i &p_from, const Vector2i &p_to, const Dictionary &p_meta) {
	select_tileset_slot(p_slot_key);
	_ensure_active_tileset_record();
	int min_x = MIN(p_from.x, p_to.x);
	int max_x = MAX(p_from.x, p_to.x);
	int min_y = MIN(p_from.y, p_to.y);
	int max_y = MAX(p_from.y, p_to.y);
	for (int y = min_y; y <= max_y; y++) {
		for (int x = min_x; x <= max_x; x++) {
			set_tileset_slot_tile_meta(p_slot_key, Vector2i(x, y), p_meta);
		}
	}
}

Dictionary ClockGearsDatabaseDialog::get_tileset_slot_tile_meta(const String &p_slot_key, const Vector2i &p_coords) const {
	const Array &arr = category_data[CAT_TILESETS];
	if (arr.is_empty()) {
		return Dictionary();
	}
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	int rec_idx = (!sel.is_empty() && sel[0] >= 0 && sel[0] < arr.size()) ? sel[0] : 0;
	Dictionary d = arr[rec_idx];
	Dictionary slot_cols = d.get("slot_collisions", Dictionary());
	String sk = _get_slot_key(_find_slot_index(p_slot_key));
	Dictionary cols = slot_cols.get(sk, Dictionary());
	String key = vformat("%d,%d", p_coords.x, p_coords.y);
	return cols.get(key, Dictionary());
}

void ClockGearsDatabaseDialog::save_to_disk() {
	_save_database_to_disk();
	_set_pending_changes(false);
}

void ClockGearsDatabaseDialog::load_from_disk() {
	_clear_record_list_icon_cache();
	_load_database_from_disk();
	_set_pending_changes(false);
	_refresh_category_view();
}

void ClockGearsDatabaseDialog::set_remembered_window_size(const Vector2i &p_size) {
	Size2i clamped = p_size.max(get_min_size());
	set_size(clamped);
	_save_editor_ui_state();
}

Vector2i ClockGearsDatabaseDialog::get_remembered_window_size() const {
	Dictionary st = _load_editor_ui_state();
	if (st.has("window_w") && st.has("window_h")) {
		int sw = int(st.get("window_w", 0));
		int sh = int(st.get("window_h", 0));
		if (sw >= get_min_size().x && sh >= get_min_size().y) {
			return Vector2i(sw, sh);
		}
	}
	return get_size().max(get_min_size());
}

Dictionary ClockGearsDatabaseDialog::check_layout_fits_window(const Vector2i &p_window_size) const {
	Dictionary res;
	int win_w = MAX(p_window_size.x, get_min_size().x);
	int win_h = MAX(p_window_size.y, get_min_size().y);
	int col1_min_w = col1_box ? int(col1_box->get_custom_minimum_size().x) : 200;
	int col2_min_w = col2_scroll_container ? int(col2_scroll_container->get_custom_minimum_size().x) : 320;
	int picker_min_w = db_tile_picker ? int(db_tile_picker->get_combined_minimum_size().x) : 240;
	int total_min_w = col1_min_w + 16 + MAX(col2_min_w, picker_min_w) + 24;

	int alloc_col1 = col1_min_w;
	int alloc_col2 = MAX(col2_min_w, win_w - alloc_col1 - 40);

	res["window_size"] = Vector2i(win_w, win_h);
	res["left_min_w"] = 0;
	res["col1_min_w"] = col1_min_w;
	res["col2_min_w"] = col2_min_w;
	res["col3_min_w"] = 0;
	res["picker_min_w"] = picker_min_w;
	res["total_min_w"] = total_min_w;
	res["alloc_col1_w"] = alloc_col1;
	res["alloc_col2_w"] = alloc_col2;
	res["alloc_col3_w"] = alloc_col2;
	res["help_wraps"] = true;
	res["picker_has_scrollbars"] = (db_tile_picker != nullptr);
	res["fits_horizontally"] = (total_min_w <= win_w);
	return res;
}

void ClockGearsDatabaseDialog::_on_system_field_changed() {
	if (updating_ui) {
		return;
	}
	if (sys_game_title_edit) {
		system_config["game_title"] = sys_game_title_edit->get_text();
	}
	if (sys_currency_edit) {
		system_config["currency"] = sys_currency_edit->get_text();
	}
	if (sys_start_x_spin) {
		system_config["start_x"] = (int)sys_start_x_spin->get_value();
	}
	if (sys_start_y_spin) {
		system_config["start_y"] = (int)sys_start_y_spin->get_value();
	}
	if (sys_camera_zoom_opt) {
		system_config["camera_zoom"] = sys_camera_zoom_opt->get_selected() + 1;
	}
	if (sys_camera_smooth_check) {
		system_config["camera_smooth"] = sys_camera_smooth_check->is_pressed();
	}
	if (sys_camera_clamp_check) {
		system_config["camera_clamp"] = sys_camera_clamp_check->is_pressed();
	}
	if (sys_movement_dirs_opt) {
		int sel_id = sys_movement_dirs_opt->get_selected_id();
		system_config["movement_directions"] = (sel_id == 8) ? 8 : 4;
	}
	if (sys_mannequin_size_opt) {
		int sel_id = sys_mannequin_size_opt->get_selected_id();
		system_config["mannequin_size"] = ClockGearsDataManager::normalize_mannequin_size(sel_id);
	}
	if (sys_arpg_move_atk_chk) {
		system_config["arpg_move_while_attacking"] = sys_arpg_move_atk_chk->is_pressed();
	}
	if (sys_arpg_atk_spd_spin) {
		system_config["arpg_attack_move_speed_pct"] = CLAMP(int(sys_arpg_atk_spd_spin->get_value()), 0, 200);
	}
	if (sys_arpg_move_blk_chk) {
		system_config["arpg_move_while_blocking"] = sys_arpg_move_blk_chk->is_pressed();
	}
	if (sys_arpg_blk_spd_spin) {
		system_config["arpg_block_move_speed_pct"] = CLAMP(int(sys_arpg_blk_spd_spin->get_value()), 0, 200);
	}
	if (sys_arpg_invuln_spin) {
		system_config["arpg_invuln_seconds"] = CLAMP(double(sys_arpg_invuln_spin->get_value()), 0.0, 3.0);
	}
	if (sys_arpg_block_red_spin) {
		system_config["arpg_block_reduction_pct"] = CLAMP(int(Math::round(sys_arpg_block_red_spin->get_value())), 0, 100);
	}
	if (sys_arpg_dodge_type_opt) {
		system_config["arpg_dodge_type"] = (sys_arpg_dodge_type_opt->get_selected() == 1) ? String("dash") : String("voltereta");
	}
	if (sys_arpg_dodge_dur_spin) {
		system_config["arpg_dodge_duration"] = CLAMP(double(sys_arpg_dodge_dur_spin->get_value()), 0.05, 2.0);
	}
	if (sys_arpg_dodge_dist_spin) {
		system_config["arpg_dodge_distance"] = CLAMP(double(sys_arpg_dodge_dist_spin->get_value()), 16.0, 320.0);
	}
	if (sys_arpg_dodge_cd_spin) {
		system_config["arpg_dodge_cooldown"] = CLAMP(double(sys_arpg_dodge_cd_spin->get_value()), 0.0, 5.0);
	}
	if (sys_arpg_show_dmg_chk) {
		system_config["arpg_show_damage_numbers"] = sys_arpg_show_dmg_chk->is_pressed();
	}
	if (ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager::get_singleton()->set_system_config(system_config);
	}
	if (!String(system_config.get("start_map", "")).is_empty()) {
		system_config["has_start_pos"] = true;
	}
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_update_system_arpg_controls_visibility() {
	bool is_arpg = (int(system_config.get("combat_mode", COMBAT_MODE_TURN_BASED)) == COMBAT_MODE_ACTION_RPG);
	bool allow_atk_move = sys_arpg_move_atk_chk ? sys_arpg_move_atk_chk->is_pressed() : true;
	bool allow_blk_move = sys_arpg_move_blk_chk ? sys_arpg_move_blk_chk->is_pressed() : false;

	if (lbl_sys_arpg_atk_move) {
		lbl_sys_arpg_atk_move->set_visible(is_arpg);
	}
	if (sys_arpg_move_atk_chk) {
		sys_arpg_move_atk_chk->set_visible(is_arpg);
	}
	if (lbl_sys_arpg_atk_spd) {
		lbl_sys_arpg_atk_spd->set_visible(is_arpg && allow_atk_move);
	}
	if (sys_arpg_atk_spd_spin) {
		sys_arpg_atk_spd_spin->set_visible(is_arpg && allow_atk_move);
	}
	if (lbl_sys_arpg_blk_move) {
		lbl_sys_arpg_blk_move->set_visible(is_arpg);
	}
	if (sys_arpg_move_blk_chk) {
		sys_arpg_move_blk_chk->set_visible(is_arpg);
	}
	if (lbl_sys_arpg_blk_spd) {
		lbl_sys_arpg_blk_spd->set_visible(is_arpg && allow_blk_move);
	}
	if (sys_arpg_blk_spd_spin) {
		sys_arpg_blk_spd_spin->set_visible(is_arpg && allow_blk_move);
	}
	if (sys_arpg_realtime_panel) {
		sys_arpg_realtime_panel->set_visible(is_arpg);
	}
}

void ClockGearsDatabaseDialog::_on_system_arpg_controls_changed() {
	if (updating_ui) {
		return;
	}
	_update_system_arpg_controls_visibility();
	_on_system_field_changed();
}

bool ClockGearsDatabaseDialog::_has_valid_selected_record() const {
	if (current_category < 0 || current_category >= CAT_MAX || !record_list) {
		return false;
	}
	const Array &arr = category_data[current_category];
	if (arr.is_empty()) {
		return false;
	}
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty()) {
		return false;
	}
	int idx = sel[0];
	return idx >= 0 && idx < arr.size();
}

void ClockGearsDatabaseDialog::_set_anim_editor_controls_enabled(bool p_enabled) {
	if (actor_dir_idle_chk) {
		actor_dir_idle_chk->set_disabled(!p_enabled);
	}
	if (anim_track_opt) {
		anim_track_opt->set_disabled(!p_enabled);
	}
	if (btn_add_anim) {
		btn_add_anim->set_disabled(!p_enabled);
	}
	if (btn_rename_anim) {
		btn_rename_anim->set_disabled(!p_enabled);
	}
	if (btn_remove_anim) {
		btn_remove_anim->set_disabled(!p_enabled);
	}
	if (btn_open_sheet_importer) {
		btn_open_sheet_importer->set_disabled(!p_enabled);
	}
	if (anim_fps_spin) {
		anim_fps_spin->set_editable(p_enabled);
	}
	if (anim_loop_chk) {
		anim_loop_chk->set_disabled(!p_enabled);
	}
	if (anim_random_order_chk) {
		anim_random_order_chk->set_disabled(!p_enabled);
	}
	if (anim_behind_body_chk) {
		anim_behind_body_chk->set_disabled(!p_enabled);
	}
	if (btn_anim_play) {
		btn_anim_play->set_disabled(!p_enabled);
	}
	if (arpg_draw_mode_opt) {
		arpg_draw_mode_opt->set_disabled(!p_enabled);
	}
	if (arpg_layer_order_spin) {
		arpg_layer_order_spin->set_editable(p_enabled);
	}
	if (arpg_simple_group_opt) {
		arpg_simple_group_opt->set_disabled(!p_enabled);
	}
	if (arpg_simple_dir_opt) {
		arpg_simple_dir_opt->set_disabled(!p_enabled);
	}
	if (arpg_simple_browse_btn) {
		arpg_simple_browse_btn->set_disabled(!p_enabled);
	}
	if (arpg_simple_clear_btn) {
		arpg_simple_clear_btn->set_disabled(!p_enabled);
	}
	if (arpg_simple_offset_x_spin) {
		arpg_simple_offset_x_spin->set_editable(p_enabled);
	}
	if (arpg_simple_offset_y_spin) {
		arpg_simple_offset_y_spin->set_editable(p_enabled);
	}
	if (arpg_simple_center_btn) {
		arpg_simple_center_btn->set_disabled(!p_enabled);
	}
	if (arpg_simple_behind_chk) {
		arpg_simple_behind_chk->set_disabled(!p_enabled);
	}
}

void ClockGearsDatabaseDialog::_clear_and_disable_anim_editor() {
	bool prev_upd = updating_ui;
	updating_ui = true;
	current_sprite_data = ClockGearsSpriteSheetData();
	anim_preview_playing = false;
	anim_preview_timer = 0.0f;
	anim_preview_frame_idx = 0;

	if (actor_dir_idle_chk) {
		actor_dir_idle_chk->set_pressed(false);
	}
	if (anim_track_opt) {
		anim_track_opt->clear();
	}
	if (anim_item_list) {
		anim_item_list->clear();
	}
	if (anim_fps_spin) {
		anim_fps_spin->set_value(8.0f);
	}
	if (anim_loop_chk) {
		anim_loop_chk->set_pressed(true);
	}
	if (anim_random_order_chk) {
		anim_random_order_chk->set_pressed(false);
	}
	if (anim_behind_body_chk) {
		anim_behind_body_chk->set_pressed(false);
	}
	if (arpg_simple_path_edit) {
		arpg_simple_path_edit->set_text("");
	}
	if (arpg_simple_offset_x_spin) {
		arpg_simple_offset_x_spin->set_value(0);
	}
	if (arpg_simple_offset_y_spin) {
		arpg_simple_offset_y_spin->set_value(0);
	}
	if (arpg_simple_behind_chk) {
		arpg_simple_behind_chk->set_pressed(false);
	}
	_set_anim_editor_controls_enabled(false);
	if (sprite_anim_editor_box) {
		sprite_anim_editor_box->set_visible(false);
	}
	if (anim_empty_state_label) {
		anim_empty_state_label->set_visible(true);
	}
	_refresh_frames_timeline();
	_refresh_anim_preview();
	_refresh_combat_anims_dropdowns();
	if (arpg_prev_canvas) {
		arpg_prev_canvas->queue_redraw();
	}
	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_on_system_movement_dirs_selected(int p_idx) {
	if (updating_ui || !sys_movement_dirs_opt) {
		return;
	}
	int dirs = (sys_movement_dirs_opt->get_item_id(p_idx) == 8) ? 8 : 4;
	system_config["movement_directions"] = dirs;
	if (ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager::get_singleton()->set_system_config(system_config);
	}
	if (s_is_anim_slot_category(current_category)) {
		_refresh_anim_track_options();
	}
	if (s_is_arpg_equip_category(current_category)) {
		_refresh_arpg_simple_fields();
		_refresh_arpg_preview_selectors();
		_update_arpg_preview_canvas();
	}
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_actor_directional_idle_toggled(bool p_pressed) {
	if (updating_ui || !_has_valid_selected_record()) {
		return;
	}
	_on_param_changed();
	if (s_is_anim_slot_category(current_category)) {
		_refresh_anim_track_options();
	}
	if (s_is_arpg_equip_category(current_category)) {
		_update_arpg_preview_canvas();
	}
}

void ClockGearsDatabaseDialog::_on_system_text_changed(const String &p_new_text) {
	_on_system_field_changed();
}

void ClockGearsDatabaseDialog::_on_system_spin_changed(double p_val) {
	_on_system_field_changed();
}

void ClockGearsDatabaseDialog::_on_combat_mode_selected(int p_idx) {
	current_combat_mode = p_idx;
	system_config["combat_mode"] = p_idx;
	if (ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager::get_singleton()->set_system_config(system_config);
	}
	_update_system_arpg_controls_visibility();
	_update_chapter_and_category_visibility();
	if (s_is_anim_slot_category(current_category)) {
		if (_has_valid_selected_record()) {
			_refresh_anim_track_options();
		} else {
			_clear_and_disable_anim_editor();
		}
	}
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_bind_numeric_spinbox(SpinBox *p_spin) {
	if (!p_spin) {
		return;
	}
	p_spin->set_meta("cg_last_valid_val", p_spin->get_value());
	p_spin->connect("value_changed", callable_mp_static(+[](double p_val, SpinBox *p_sb) {
		if (p_sb) {
			p_sb->set_meta("cg_last_valid_val", p_val);
		}
	}).bind(p_spin));

	LineEdit *le = p_spin->get_line_edit();
	if (le) {
		le->set_editable(true);
		le->set_focus_mode(Control::FOCUS_ALL);
		le->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		le->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_spinbox_line_edit_text_changed).bind(p_spin));
		le->connect("text_submitted", callable_mp(this, &ClockGearsDatabaseDialog::_on_spinbox_line_edit_submitted).bind(p_spin));
		le->connect("focus_exited", callable_mp(this, &ClockGearsDatabaseDialog::_on_spinbox_line_edit_focus_exited).bind(p_spin));
	}
}

void ClockGearsDatabaseDialog::_on_spinbox_line_edit_text_changed(const String &p_new_text, SpinBox *p_spin) {
	if (!p_spin) {
		return;
	}
	LineEdit *le = p_spin->get_line_edit();
	if (!le) {
		return;
	}
	// Retoque 1: Filtrar la entrada exclusivamente a dígitos (0-9), signo inicial (+/-) y un punto decimal (.)
	String filtered;
	bool has_dot = false;
	for (int i = 0; i < p_new_text.length(); i++) {
		char32_t c = p_new_text[i];
		if (c >= '0' && c <= '9') {
			filtered += c;
		} else if ((c == '-' || c == '+') && filtered.is_empty()) {
			filtered += c;
		} else if (c == '.' && !has_dot) {
			has_dot = true;
			filtered += c;
		}
	}
	if (filtered != p_new_text) {
		int col = le->get_caret_column();
		int diff = p_new_text.length() - filtered.length();
		le->set_text(filtered);
		le->set_caret_column(MAX(0, col - diff));
	}
	if (!filtered.is_empty() && filtered.is_valid_float()) {
		double cur_v = filtered.to_float();
		double last_v = p_spin->has_meta("cg_last_valid_val") ? (double)p_spin->get_meta("cg_last_valid_val") : p_spin->get_value();
		if (Math::abs(cur_v - last_v) > 0.0001) {
			_mark_database_dirty();
		}
	}
}

void ClockGearsDatabaseDialog::_on_spinbox_line_edit_submitted(const String &p_text, SpinBox *p_spin) {
	_on_spinbox_line_edit_focus_exited(p_spin);
}

void ClockGearsDatabaseDialog::_on_spinbox_line_edit_focus_exited(SpinBox *p_spin) {
	if (!p_spin) {
		return;
	}
	LineEdit *le = p_spin->get_line_edit();
	if (!le) {
		return;
	}
	String txt = le->get_text().strip_edges();
	double prev_val = p_spin->has_meta("cg_last_valid_val") ? (double)p_spin->get_meta("cg_last_valid_val") : p_spin->get_value();
	if (txt.is_empty() || !txt.is_valid_float()) {
		p_spin->set_value(prev_val);
		le->set_text(String::num(p_spin->get_value(), Math::range_step_decimals(p_spin->get_step())));
	} else {
		double parsed = txt.to_float();
		bool changed = (Math::abs(parsed - prev_val) > 0.0001);
		p_spin->set_value(parsed);
		p_spin->set_meta("cg_last_valid_val", p_spin->get_value());
		le->set_text(String::num(p_spin->get_value(), Math::range_step_decimals(p_spin->get_step())));
		if (changed) {
			_mark_database_dirty();
		}
	}
}

ClockGearsDatabaseDialog::ClockGearsDatabaseDialog() {
	singleton = this;
	set_process(true);
	set_title(CG_DB_TR("Base de Datos - ClockGears Engine"));
	set_wrap_controls(false);
	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_min_size(Size2i(1000, 640));
	connect("size_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_window_size_changed));

	_init_chapters();
	_init_empty_database();
	if (EditorNode::get_singleton()) {
		_load_database_from_disk();
	}

	left_sidebar_panel = nullptr;
	col3_scroll_container = nullptr;
	col3_box = nullptr;
	col3_title = nullptr;
	anim_item_list = nullptr;

	const int icon_scale = ClockGearsIcons::get_editor_icon_scale(this);
	last_icon_scale = icon_scale;
	const int icon_px = 32 * icon_scale;
	const float row_h = 32.0f * EDSCALE;

	MarginContainer *root_margin = memnew(MarginContainer);
	root_margin->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	root_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_margin->add_theme_constant_override("margin_left", int(12 * EDSCALE));
	root_margin->add_theme_constant_override("margin_top", int(12 * EDSCALE));
	root_margin->add_theme_constant_override("margin_right", int(12 * EDSCALE));
	root_margin->add_theme_constant_override("margin_bottom", int(12 * EDSCALE));
	add_child(root_margin);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	main_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_theme_constant_override("separation", int(8 * EDSCALE));
	root_margin->add_child(main_vbox);

	// ========================================================================
	// TOP BAR 1: CAPÍTULOS (Personajes, Equipo, Objetos, Combate, Animaciones, Mundo, Sistema)
	// ========================================================================
	PanelContainer *chapters_panel = memnew(PanelContainer);
	chapters_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	Ref<StyleBoxFlat> chap_bar_style = memnew(StyleBoxFlat);
	chap_bar_style->set_anti_aliased(true);
	chap_bar_style->set_bg_color(Color(0.11f, 0.12f, 0.15f, 1.0f));
	chap_bar_style->set_border_width_all(1);
	chap_bar_style->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.10f));
	chap_bar_style->set_content_margin_individual(int(8 * EDSCALE), int(6 * EDSCALE), int(8 * EDSCALE), int(6 * EDSCALE));
	chap_bar_style->set_corner_radius_all(int(6 * EDSCALE));
	chapters_panel->add_theme_style_override("panel", chap_bar_style);
	main_vbox->add_child(chapters_panel);

	HBoxContainer *chapters_bar = memnew(HBoxContainer);
	chapters_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	chapters_bar->add_theme_constant_override("separation", int(6 * EDSCALE));
	chapters_panel->add_child(chapters_bar);

	for (int c = 0; c < chapters.size(); c++) {
		Button *cbtn = memnew(Button(chapters[c].name));
		cbtn->set_theme_type_variation("ClockGearsTabButton");
		cbtn->set_toggle_mode(true);
		cbtn->set_pressed(c == 0);
		cbtn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		cbtn->set_custom_minimum_size(Size2(0, row_h));
		cbtn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_chapter_selected).bind(c));
		chapters_bar->add_child(cbtn);
		chapter_buttons.push_back(cbtn);
	}

	// ========================================================================
	// TOP BAR 2: CATEGORÍAS DEL CAPÍTULO ACTIVO (Icono 32x32*entero + Nombre)
	// ========================================================================
	PanelContainer *cats_panel = memnew(PanelContainer);
	cats_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	Ref<StyleBoxFlat> cat_bar_style = memnew(StyleBoxFlat);
	cat_bar_style->set_anti_aliased(true);
	cat_bar_style->set_bg_color(Color(0.14f, 0.15f, 0.19f, 1.0f));
	cat_bar_style->set_border_width_all(1);
	cat_bar_style->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.10f));
	cat_bar_style->set_content_margin_individual(int(8 * EDSCALE), int(6 * EDSCALE), int(8 * EDSCALE), int(6 * EDSCALE));
	cat_bar_style->set_corner_radius_all(int(6 * EDSCALE));
	cats_panel->add_theme_style_override("panel", cat_bar_style);
	main_vbox->add_child(cats_panel);

	HFlowContainer *cats_bar = memnew(HFlowContainer);
	cats_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cats_bar->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	cats_bar->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	cats_panel->add_child(cats_bar);

	const float cat_btn_h = MAX(row_h, float(icon_px + 16));
	for (int i = 0; i <= DB_CAT_UI_DESIGNER; i++) {
		Button *btn = memnew(Button(_get_category_name(i)));
		btn->set_theme_type_variation("ClockGearsTabButton");
		btn->set_toggle_mode(true);
		btn->set_custom_minimum_size(Size2(0, cat_btn_h));
		btn->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		btn->add_theme_constant_override("icon_max_width", icon_px);
		btn->add_theme_color_override("icon_normal_color", Color(1, 1, 1, 1));
		btn->add_theme_color_override("icon_hover_color", Color(1, 1, 1, 1));
		btn->add_theme_color_override("icon_pressed_color", Color(1, 1, 1, 1));
		btn->add_theme_color_override("icon_hover_pressed_color", Color(1, 1, 1, 1));
		btn->add_theme_color_override("icon_focus_color", Color(1, 1, 1, 1));
		btn->add_theme_color_override("icon_disabled_color", Color(1, 1, 1, 0.4f));
		if (ClockGearsIcons::get_singleton()) {
			btn->set_button_icon(ClockGearsIcons::get_singleton()->get_db_category_icon(i, float(icon_scale)));
		}
		if (s_is_arpg_category(i)) {
			btn->set_tooltip_text(CG_DB_TR("Para combate en tiempo real"));
		}
		btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_category_selected).bind(i));
		cats_bar->add_child(btn);
		category_buttons.push_back(btn);
	}

	// ========================================================================
	// MAIN AREA: SOLO DOS COLUMNAS (Columna 1: Lista | Columna 2: Editor)
	// ========================================================================
	HBoxContainer *two_col_hbox = memnew(HBoxContainer);
	two_col_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	two_col_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	two_col_hbox->add_theme_constant_override("separation", int(12 * EDSCALE));
	main_vbox->add_child(two_col_hbox);

	// --- COLUMNA 1: Lista de registros + "+", "-", "Max", "Copiar", "Pegar" ---
	col1_box = memnew(VBoxContainer);
	col1_box->set_custom_minimum_size(Size2(MAX(256.0f * EDSCALE, float(icon_px + 192)), 0));
	col1_box->set_h_size_flags(Control::SIZE_FILL);
	col1_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col1_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	two_col_hbox->add_child(col1_box);

	col1_title = memnew(Label(CG_DB_TR("Lista de Actores")));
	col1_title->set_clip_text(true);
	col1_title->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	col1_box->add_child(col1_title);

	record_list = memnew(ItemList);
	record_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	record_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	record_list->set_custom_minimum_size(Size2(0, 140 * EDSCALE));
	record_list->set_fixed_icon_size(Size2i(icon_px, icon_px));
	record_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	record_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_record_selected));
	col1_box->add_child(record_list);

	HFlowContainer *list_btn_row = memnew(HFlowContainer);
	list_btn_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	list_btn_row->set_v_size_flags(Control::SIZE_SHRINK_END);
	list_btn_row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	list_btn_row->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	col1_box->add_child(list_btn_row);

	btn_add_record = memnew(Button("+"));
	btn_add_record->set_custom_minimum_size(Size2(0, row_h));
	btn_add_record->set_tooltip_text(CG_DB_TR("Añadir nuevo registro"));
	btn_add_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_record_pressed));
	list_btn_row->add_child(btn_add_record);

	btn_remove_record = memnew(Button("-"));
	btn_remove_record->set_custom_minimum_size(Size2(0, row_h));
	btn_remove_record->set_tooltip_text(CG_DB_TR("Eliminar registro seleccionado"));
	btn_remove_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_record_pressed));
	list_btn_row->add_child(btn_remove_record);

	btn_change_max = memnew(Button(CG_DB_TR("Max")));
	btn_change_max->set_custom_minimum_size(Size2(0, row_h));
	btn_change_max->set_tooltip_text(CG_DB_TR("Cambiar cantidad máxima de registros"));
	btn_change_max->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_change_max_pressed));
	list_btn_row->add_child(btn_change_max);

	btn_copy_record = memnew(Button(CG_DB_TR("Copiar")));
	btn_copy_record->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_copy_record->set_custom_minimum_size(Size2(0, row_h));
	btn_copy_record->set_tooltip_text(CG_DB_TR("Copiar el registro seleccionado"));
	btn_copy_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_copy_record_pressed));
	list_btn_row->add_child(btn_copy_record);

	btn_paste_record = memnew(Button(CG_DB_TR("Pegar")));
	btn_paste_record->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_paste_record->set_custom_minimum_size(Size2(0, row_h));
	btn_paste_record->set_tooltip_text(CG_DB_TR("Pegar el registro copiado"));
	btn_paste_record->set_disabled(true);
	btn_paste_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_paste_record_pressed));
	list_btn_row->add_child(btn_paste_record);

	// --- COLUMNA 2: Editor (todo el ancho restante) ---
	main_editor_tabs = memnew(TabContainer);
	main_editor_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_editor_tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_editor_tabs->set_tabs_visible(true);
	{
		Ref<StyleBoxFlat> db_tab_sel = memnew(StyleBoxFlat);
		db_tab_sel->set_bg_color(Color(0.18f, 0.21f, 0.27f, 1.0f));
		db_tab_sel->set_border_color(Color(0.38f, 0.56f, 0.85f, 0.85f));
		db_tab_sel->set_border_width(SIDE_TOP, 2);
		db_tab_sel->set_corner_radius(CORNER_TOP_LEFT, int(7 * EDSCALE));
		db_tab_sel->set_corner_radius(CORNER_TOP_RIGHT, int(7 * EDSCALE));
		db_tab_sel->set_content_margin_individual(int(12 * EDSCALE), int(6 * EDSCALE), int(12 * EDSCALE), int(6 * EDSCALE));

		Ref<StyleBoxFlat> db_tab_unsel = memnew(StyleBoxFlat);
		db_tab_unsel->set_bg_color(Color(0.12f, 0.13f, 0.17f, 0.9f));
		db_tab_unsel->set_corner_radius(CORNER_TOP_LEFT, int(7 * EDSCALE));
		db_tab_unsel->set_corner_radius(CORNER_TOP_RIGHT, int(7 * EDSCALE));
		db_tab_unsel->set_content_margin_individual(int(12 * EDSCALE), int(6 * EDSCALE), int(12 * EDSCALE), int(6 * EDSCALE));

		Ref<StyleBoxFlat> db_tab_panel = memnew(StyleBoxFlat);
		db_tab_panel->set_bg_color(Color(0.14f, 0.15f, 0.19f, 0.96f));
		db_tab_panel->set_border_width_all(1);
		db_tab_panel->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.08f));
		db_tab_panel->set_corner_radius_all(int(8 * EDSCALE));

		main_editor_tabs->add_theme_style_override("tab_selected", db_tab_sel);
		main_editor_tabs->add_theme_style_override("tab_hovered", db_tab_sel);
		main_editor_tabs->add_theme_style_override("tab_unselected", db_tab_unsel);
		main_editor_tabs->add_theme_style_override("panel", db_tab_panel);
	}
	two_col_hbox->add_child(main_editor_tabs);

	// Pestaña 0: "General"
	ScrollContainer *col2_scroll = memnew(ScrollContainer);
	col2_scroll_container = col2_scroll;
	col2_scroll->set_name(CG_DB_TR("General"));
	col2_scroll->set_custom_minimum_size(Size2(320 * EDSCALE, 0));
	col2_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col2_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col2_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	col2_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	main_editor_tabs->add_child(col2_scroll);

	MarginContainer *tab0_margin = memnew(MarginContainer);
	tab0_margin_container = tab0_margin;
	tab0_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab0_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab0_margin->add_theme_constant_override("margin_left", int(12 * EDSCALE));
	tab0_margin->add_theme_constant_override("margin_top", int(12 * EDSCALE));
	tab0_margin->add_theme_constant_override("margin_right", int(12 * EDSCALE));
	tab0_margin->add_theme_constant_override("margin_bottom", int(12 * EDSCALE));
	col2_scroll->add_child(tab0_margin);

	col2_box = memnew(VBoxContainer);
	col2_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col2_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col2_box->add_theme_constant_override("separation", int(16 * EDSCALE));
	tab0_margin->add_child(col2_box);

	// Pestaña 1: "Combate" (Enemigos: Debilidades, Patrones IA y Rasgos)
	ScrollContainer *combat_tab_scroll = memnew(ScrollContainer);
	combat_tab_scroll->set_name(CG_DB_TR("Combate"));
	combat_tab_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	combat_tab_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	combat_tab_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	combat_tab_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	main_editor_tabs->add_child(combat_tab_scroll);

	MarginContainer *tab1_margin = memnew(MarginContainer);
	tab1_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab1_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab1_margin->add_theme_constant_override("margin_left", int(12 * EDSCALE));
	tab1_margin->add_theme_constant_override("margin_top", int(12 * EDSCALE));
	tab1_margin->add_theme_constant_override("margin_right", int(12 * EDSCALE));
	tab1_margin->add_theme_constant_override("margin_bottom", int(12 * EDSCALE));
	combat_tab_scroll->add_child(tab1_margin);

	combat_tab_vbox = memnew(VBoxContainer);
	combat_tab_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	combat_tab_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	combat_tab_vbox->add_theme_constant_override("separation", int(16 * EDSCALE));
	tab1_margin->add_child(combat_tab_vbox);

	// Pestaña 2: "Animaciones" (Actores y Enemigos)
	anim_editor_scroll = memnew(ScrollContainer);
	anim_editor_scroll->set_name(CG_DB_TR("Animaciones"));
	anim_editor_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_editor_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	anim_editor_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	anim_editor_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	main_editor_tabs->add_child(anim_editor_scroll);

	MarginContainer *tab2_margin = memnew(MarginContainer);
	tab2_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab2_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab2_margin->add_theme_constant_override("margin_left", int(12 * EDSCALE));
	tab2_margin->add_theme_constant_override("margin_top", int(12 * EDSCALE));
	tab2_margin->add_theme_constant_override("margin_right", int(12 * EDSCALE));
	tab2_margin->add_theme_constant_override("margin_bottom", int(12 * EDSCALE));
	anim_editor_scroll->add_child(tab2_margin);

	anim_empty_state_label = memnew(Label(CG_DB_TR("Crea una entrada con +")));
	anim_empty_state_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_empty_state_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	anim_empty_state_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	anim_empty_state_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	anim_empty_state_label->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
	anim_empty_state_label->set_visible(false);
	tab2_margin->add_child(anim_empty_state_label);

	sprite_anim_editor_box = memnew(VBoxContainer);
	sprite_anim_editor_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sprite_anim_editor_box->add_theme_constant_override("separation", int(16 * EDSCALE));
	tab2_margin->add_child(sprite_anim_editor_box);

	// Pestaña 3: "Evento" (NPC, NPC ARPG, Enemigos ARPG)
	event_tab_scroll = memnew(ScrollContainer);
	event_tab_scroll->set_name(CG_DB_TR("Evento"));
	event_tab_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	event_tab_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	event_tab_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	event_tab_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	main_editor_tabs->add_child(event_tab_scroll);

	MarginContainer *tab3_margin = memnew(MarginContainer);
	tab3_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab3_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab3_margin->add_theme_constant_override("margin_left", int(12 * EDSCALE));
	tab3_margin->add_theme_constant_override("margin_top", int(12 * EDSCALE));
	tab3_margin->add_theme_constant_override("margin_right", int(12 * EDSCALE));
	tab3_margin->add_theme_constant_override("margin_bottom", int(12 * EDSCALE));
	event_tab_scroll->add_child(tab3_margin);

	event_tab_vbox = memnew(VBoxContainer);
	event_tab_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	event_tab_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	event_tab_vbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	tab3_margin->add_child(event_tab_vbox);

	HBoxContainer *ev_hdr = memnew(HBoxContainer);
	ev_hdr->add_theme_constant_override("separation", int(10 * EDSCALE));
	event_tab_vbox->add_child(ev_hdr);

	event_tab_summary_label = memnew(Label(CG_DB_TR("0 páginas de evento configuradas.")));
	event_tab_summary_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ev_hdr->add_child(event_tab_summary_label);

	btn_open_record_event_editor = memnew(Button(CG_DB_TR("Editar Evento...")));
	btn_open_record_event_editor->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	btn_open_record_event_editor->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_open_record_event_editor_pressed));
	ev_hdr->add_child(btn_open_record_event_editor);

	event_tab_pages_tree = memnew(Tree);
	event_tab_pages_tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	event_tab_pages_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	event_tab_pages_tree->set_custom_minimum_size(Size2(0, 220 * EDSCALE));
	event_tab_pages_tree->set_hide_root(true);
	event_tab_pages_tree->set_columns(3);
	event_tab_pages_tree->set_column_titles_visible(true);
	event_tab_pages_tree->set_column_title(0, CG_DB_TR("Página"));
	event_tab_pages_tree->set_column_title(1, CG_DB_TR("Disparador"));
	event_tab_pages_tree->set_column_title(2, CG_DB_TR("Comandos"));
	event_tab_vbox->add_child(event_tab_pages_tree);

	// `record_event_editor` se crea de forma perezosa la primera vez que se edita un evento en DB.

	_setup_body_zones_tab_ui();
	_setup_movement_tab_ui();

	// ========================================================================
	// SECCIÓN 1 EN TAB GENERAL: DATOS GENERALES + RECUADRO FIJO 96x96 A LA DERECHA
	// ========================================================================
	general_section_box = memnew(VBoxContainer);
	general_section_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	general_section_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	col2_box->add_child(general_section_box);

	col2_title = memnew(Label(CG_DB_TR("General")));
	col2_title->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	general_section_box->add_child(col2_title);
	general_section_box->add_child(memnew(HSeparator));

	HBoxContainer *gen_top_hbox = memnew(HBoxContainer);
	gen_top_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gen_top_hbox->add_theme_constant_override("separation", int(16 * EDSCALE));
	general_section_box->add_child(gen_top_hbox);

	params_grid = memnew(GridContainer);
	params_grid->set_columns(4); // 2 pares de (Etiqueta + Campo) por fila
	params_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	params_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	gen_top_hbox->add_child(params_grid);

	// Recuadro fijo de 96x96 a la derecha cuando la categoría usa un gráfico/icono
	standard_graphic_box = memnew(VBoxContainer);
	standard_graphic_box->set_h_size_flags(Control::SIZE_FILL);
	standard_graphic_box->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	standard_graphic_box->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	standard_graphic_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	gen_top_hbox->add_child(standard_graphic_box);

	CenterContainer *graphic_center = memnew(CenterContainer);
	graphic_center->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	standard_graphic_box->add_child(graphic_center);

	graphic_frame = memnew(PanelContainer);
	graphic_frame->set_custom_minimum_size(Size2(96 * EDSCALE, 96 * EDSCALE));
	Ref<StyleBoxFlat> dark_frame = memnew(StyleBoxFlat);
	dark_frame->set_anti_aliased(true);
	dark_frame->set_bg_color(Color(0.08f, 0.09f, 0.11f, 1.0f));
	dark_frame->set_corner_radius_all(int(6 * EDSCALE));
	dark_frame->set_border_width_all(1);
	dark_frame->set_border_color(Color(0.28f, 0.31f, 0.37f, 1.0f));
	dark_frame->set_content_margin_all(4);
	graphic_frame->add_theme_style_override("panel", dark_frame);
	graphic_center->add_child(graphic_frame);

	graphic_preview = memnew(TextureRect);
	graphic_preview->set_custom_minimum_size(Size2(88 * EDSCALE, 88 * EDSCALE));
	graphic_preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	graphic_preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	graphic_preview->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	graphic_preview->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	graphic_preview->set_tooltip_text(CG_DB_TR("Doble clic para abrir el explorador y asignar Sprite o Icono (96x96)"));
	graphic_preview->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_graphic_gui_input));
	graphic_frame->add_child(graphic_preview);

	graphic_path_label = memnew(Label(CG_DB_TR("(Sin gráfico)")));
	graphic_path_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	graphic_path_label->set_custom_minimum_size(Size2(160 * EDSCALE, 0));
	graphic_path_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	graphic_path_label->set_max_lines_visible(2);
	graphic_path_label->set_clip_text(true);
	graphic_path_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	graphic_path_label->set_tooltip_text(CG_DB_TR("Ruta del gráfico asignado. Doble clic para asignar."));
	graphic_path_label->connect("resized", callable_mp(this, &ClockGearsDatabaseDialog::_refit_graphic_path_label));
	standard_graphic_box->add_child(graphic_path_label);

	btn_select_graphic = memnew(Button(CG_DB_TR("Cambiar gráfico...")));
	btn_select_graphic->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_select_graphic->set_custom_minimum_size(Size2(0, row_h));
	btn_select_graphic->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_select_graphic_pressed));
	standard_graphic_box->add_child(btn_select_graphic);

	btn_pick_tile_graphic = memnew(Button(CG_DB_TR("TilePicker...")));
	btn_pick_tile_graphic->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_pick_tile_graphic->set_custom_minimum_size(Size2(0, row_h));
	btn_pick_tile_graphic->set_tooltip_text(CG_DB_TR("Seleccionar celda 32x32 desde una hoja de tiles"));
	btn_pick_tile_graphic->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_pick_tile_graphic_pressed));
	standard_graphic_box->add_child(btn_pick_tile_graphic);

	tile_picker_dialog = memnew(ClockGearsTilePickerDialog);
	tile_picker_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_tile_picker_closed));
	tile_picker_dialog->connect("canceled", callable_mp(this, &ClockGearsDatabaseDialog::_on_tile_picker_closed));
	tile_picker_dialog->connect("close_requested", callable_mp(this, &ClockGearsDatabaseDialog::_on_tile_picker_closed));
	add_child(tile_picker_dialog);

	Label *lbl_nom = memnew(Label(CG_DB_TR("Nombre:")));
	lbl_nom->set_tooltip_text(CG_DB_TR("Nombre identificador"));
	params_grid->add_child(lbl_nom);
	name_edit = memnew(LineEdit);
	name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	name_edit->set_custom_minimum_size(Size2(150 * EDSCALE, row_h));
	name_edit->set_focus_mode(Control::FOCUS_ALL);
	name_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	name_edit->set_editable(true);
	name_edit->set_placeholder(CG_DB_TR("Nombre..."));
	name_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_text_changed));
	params_grid->add_child(name_edit);

	Label *lbl_desc = memnew(Label(CG_DB_TR("Notas:")));
	lbl_desc->set_tooltip_text(CG_DB_TR("Descripción en el menú o notas del registro"));
	params_grid->add_child(lbl_desc);
	desc_edit = memnew(LineEdit);
	desc_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	desc_edit->set_custom_minimum_size(Size2(150 * EDSCALE, row_h));
	desc_edit->set_focus_mode(Control::FOCUS_ALL);
	desc_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	desc_edit->set_editable(true);
	desc_edit->set_placeholder(CG_DB_TR("Descripción / notas..."));
	desc_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_text_changed));
	params_grid->add_child(desc_edit);

	type_label = memnew(Label(CG_DB_TR("Clase:")));
	type_label->set_tooltip_text(CG_DB_TR("Tipo o clase principal"));
	params_grid->add_child(type_label);
	type_option = memnew(OptionButton);
	type_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	type_option->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	type_option->set_clip_text(true);
	type_option->set_focus_mode(Control::FOCUS_ALL);
	type_option->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	type_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(type_option);

	secondary_label = memnew(Label(CG_DB_TR("Subtipo:")));
	secondary_label->set_tooltip_text(CG_DB_TR("Subtipo o categoría secundaria"));
	params_grid->add_child(secondary_label);
	secondary_option = memnew(OptionButton);
	secondary_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	secondary_option->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	secondary_option->set_clip_text(true);
	secondary_option->set_focus_mode(Control::FOCUS_ALL);
	secondary_option->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	secondary_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(secondary_option);

	tertiary_label = memnew(Label(CG_DB_TR("Ocasión:")));
	tertiary_label->set_tooltip_text(CG_DB_TR("Ocasión de uso / Vínculo"));
	params_grid->add_child(tertiary_label);
	tertiary_option = memnew(OptionButton);
	tertiary_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tertiary_option->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	tertiary_option->set_clip_text(true);
	tertiary_option->set_focus_mode(Control::FOCUS_ALL);
	tertiary_option->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	tertiary_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(tertiary_option);

	// Nivel, EXP y Oro de Enemigos en General (2 columnas)
	enemy_level_label = memnew(Label(CG_DB_TR("Nivel:")));
	enemy_level_label->set_tooltip_text(CG_DB_TR("Nivel informativo del enemigo (mínimo 1, no escala estadísticas)"));
	params_grid->add_child(enemy_level_label);
	enemy_level_spin = memnew(SpinBox);
	enemy_level_spin->set_min(1);
	enemy_level_spin->set_max(999);
	enemy_level_spin->set_step(1);
	enemy_level_spin->set_value(1);
	enemy_level_spin->set_editable(true);
	enemy_level_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_level_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(enemy_level_spin);
	enemy_level_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(enemy_level_spin);

	enemy_exp_label = memnew(Label(CG_DB_TR("EXP:")));
	enemy_exp_label->set_tooltip_text(CG_DB_TR("EXP Otorgada"));
	params_grid->add_child(enemy_exp_label);
	enemy_exp_spin = memnew(SpinBox);
	enemy_exp_spin->set_min(0);
	enemy_exp_spin->set_max(9999999);
	enemy_exp_spin->set_step(1);
	enemy_exp_spin->set_editable(true);
	enemy_exp_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_exp_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(enemy_exp_spin);
	enemy_exp_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(enemy_exp_spin);

	enemy_gold_label = memnew(Label(CG_DB_TR("Oro:")));
	enemy_gold_label->set_tooltip_text(CG_DB_TR("Oro Otorgado"));
	params_grid->add_child(enemy_gold_label);
	enemy_gold_spin = memnew(SpinBox);
	enemy_gold_spin->set_min(0);
	enemy_gold_spin->set_max(9999999);
	enemy_gold_spin->set_step(1);
	enemy_gold_spin->set_editable(true);
	enemy_gold_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_gold_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(enemy_gold_spin);
	enemy_gold_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(enemy_gold_spin);

	battler_frames_label = memnew(Label(CG_DB_TR("Frames:")));
	battler_frames_label->set_tooltip_text(CG_DB_TR("Frames en la hoja de animación"));
	params_grid->add_child(battler_frames_label);
	battler_frames_spin = memnew(SpinBox);
	battler_frames_spin->set_min(1);
	battler_frames_spin->set_max(64);
	battler_frames_spin->set_step(1);
	battler_frames_spin->set_value(1);
	battler_frames_spin->set_editable(true);
	battler_frames_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	battler_frames_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(battler_frames_spin);
	battler_frames_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(battler_frames_spin);

	battler_fps_label = memnew(Label(CG_DB_TR("FPS:")));
	battler_fps_label->set_tooltip_text(CG_DB_TR("Velocidad de reproducción (FPS)"));
	params_grid->add_child(battler_fps_label);
	battler_fps_spin = memnew(SpinBox);
	battler_fps_spin->set_min(1);
	battler_fps_spin->set_max(60);
	battler_fps_spin->set_step(1);
	battler_fps_spin->set_value(8);
	battler_fps_spin->set_editable(true);
	battler_fps_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	battler_fps_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(battler_fps_spin);
	battler_fps_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(battler_fps_spin);

	// ========================================================================
	// SECCIÓN CURVA EXP (Clases - 2 columnas)
	// ========================================================================
	exp_section_box = memnew(VBoxContainer);
	exp_section_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	exp_section_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	exp_section_box->hide();
	col2_box->add_child(exp_section_box);

	Label *lbl_exp_sec = memnew(Label(CG_DB_TR("Curva de experiencia")));
	lbl_exp_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	exp_section_box->add_child(lbl_exp_sec);
	exp_section_box->add_child(memnew(HSeparator));

	GridContainer *exp_grid = memnew(GridContainer);
	exp_grid->set_columns(4);
	exp_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	exp_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	exp_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	exp_section_box->add_child(exp_grid);

	const char *exp_titles[4] = {
		"EXP Base:",
		"EXP Extra:",
		"Aceleración A:",
		"Aceleración B:"
	};
	const char *exp_tips[4] = {
		"Valor base de la fórmula de experiencia (base)",
		"Valor extra de la fórmula de experiencia (extra)",
		"Factor de aceleración A de la fórmula (acc_a)",
		"Factor de aceleración B de la fórmula (acc_b)"
	};
	const int exp_defaults[4] = { 30, 20, 30, 30 };
	const int exp_mins[4] = { 1, 0, 1, 1 };
	const int exp_maxs[4] = { 9999, 9999, 999, 999 };
	for (int i = 0; i < 4; i++) {
		exp_labels[i] = memnew(Label(CG_DB_TR(exp_titles[i])));
		exp_labels[i]->set_tooltip_text(CG_DB_TR(exp_tips[i]));
		exp_grid->add_child(exp_labels[i]);

		exp_spins[i] = memnew(SpinBox);
		exp_spins[i]->set_min(exp_mins[i]);
		exp_spins[i]->set_max(exp_maxs[i]);
		exp_spins[i]->set_step(1);
		exp_spins[i]->set_value(exp_defaults[i]);
		exp_spins[i]->set_editable(true);
		exp_spins[i]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		exp_spins[i]->set_custom_minimum_size(Size2(0, row_h));
		_bind_numeric_spinbox(exp_spins[i]);
		exp_spins[i]->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
		exp_grid->add_child(exp_spins[i]);
	}

	HBoxContainer *exp_prev_hb = memnew(HBoxContainer);
	exp_prev_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	exp_prev_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	exp_section_box->add_child(exp_prev_hb);

	exp_preview_title = memnew(Label(CG_DB_TR("Curva EXP:")));
	exp_preview_title->set_tooltip_text(CG_DB_TR("Muestra Curva EXP (9.3)"));
	exp_prev_hb->add_child(exp_preview_title);
	exp_preview_label = memnew(Label);
	exp_preview_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	exp_preview_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	exp_preview_label->add_theme_color_override("font_color", Color(0.55f, 0.85f, 0.98f));
	exp_prev_hb->add_child(exp_preview_label);

	// ========================================================================
	// SECCIÓN USO Y DAÑO / PRECIO (Objetos, Armas, Armaduras, Habilidades - 2 columnas)
	// ========================================================================
	invocation_section_box = memnew(VBoxContainer);
	invocation_section_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	invocation_section_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	invocation_section_box->hide();
	col2_box->add_child(invocation_section_box);

	Label *lbl_inv_sec = memnew(Label(CG_DB_TR("Propiedades e invocación")));
	lbl_inv_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	invocation_section_box->add_child(lbl_inv_sec);
	invocation_section_box->add_child(memnew(HSeparator));

	GridContainer *inv_grid = memnew(GridContainer);
	inv_grid->set_columns(4);
	inv_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	inv_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	inv_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	invocation_section_box->add_child(inv_grid);

	price_label = memnew(Label(CG_DB_TR("Precio:")));
	price_label->set_tooltip_text(CG_DB_TR("Precio de Compra"));
	inv_grid->add_child(price_label);
	price_spin = memnew(SpinBox);
	price_spin->set_min(0);
	price_spin->set_max(999999);
	price_spin->set_step(1);
	price_spin->set_editable(true);
	price_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	price_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(price_spin);
	price_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	inv_grid->add_child(price_spin);

	sell_price_title = memnew(Label(CG_DB_TR("Venta:")));
	sell_price_title->set_tooltip_text(CG_DB_TR("Precio de Venta (50%)"));
	inv_grid->add_child(sell_price_title);
	sell_price_label = memnew(Label("0 G (50%)"));
	sell_price_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sell_price_label->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	inv_grid->add_child(sell_price_label);

	attack_anim_label = memnew(Label(CG_DB_TR("Animación:")));
	attack_anim_label->set_tooltip_text(CG_DB_TR("Animación de Ataque o Habilidad"));
	inv_grid->add_child(attack_anim_label);
	attack_anim_option = memnew(OptionButton);
	attack_anim_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	attack_anim_option->set_custom_minimum_size(Size2(0, row_h));
	attack_anim_option->set_clip_text(true);
	attack_anim_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	inv_grid->add_child(attack_anim_option);

	consumable_label = memnew(Label(CG_DB_TR("Consumible:")));
	consumable_label->set_tooltip_text(CG_DB_TR("Determina si se consume al usar"));
	inv_grid->add_child(consumable_label);
	consumable_check = memnew(CheckBox(CG_DB_TR("Consumible")));
	consumable_check->set_tooltip_text(CG_DB_TR("Se consume al usar"));
	consumable_check->set_pressed(true);
	consumable_check->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	consumable_check->set_custom_minimum_size(Size2(0, row_h));
	consumable_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_check_toggled));
	inv_grid->add_child(consumable_check);

	item_speed_label = memnew(Label(CG_DB_TR("Velocidad:")));
	item_speed_label->set_tooltip_text(CG_DB_TR("Velocidad de Acción"));
	inv_grid->add_child(item_speed_label);
	item_speed_spin = memnew(SpinBox);
	item_speed_spin->set_min(-2000);
	item_speed_spin->set_max(2000);
	item_speed_spin->set_step(1);
	item_speed_spin->set_value(0);
	item_speed_spin->set_editable(true);
	item_speed_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_speed_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(item_speed_spin);
	item_speed_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	inv_grid->add_child(item_speed_spin);

	item_success_label = memnew(Label(CG_DB_TR("Éxito (%):")));
	item_success_label->set_tooltip_text(CG_DB_TR("Tasa de Éxito (%)"));
	inv_grid->add_child(item_success_label);
	item_success_spin = memnew(SpinBox);
	item_success_spin->set_min(0);
	item_success_spin->set_max(100);
	item_success_spin->set_step(1);
	item_success_spin->set_value(100);
	item_success_spin->set_editable(true);
	item_success_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_success_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(item_success_spin);
	item_success_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	inv_grid->add_child(item_success_spin);

	item_repeats_label = memnew(Label(CG_DB_TR("Repeticiones:")));
	item_repeats_label->set_tooltip_text(CG_DB_TR("Número de Repeticiones"));
	inv_grid->add_child(item_repeats_label);
	item_repeats_spin = memnew(SpinBox);
	item_repeats_spin->set_min(1);
	item_repeats_spin->set_max(9);
	item_repeats_spin->set_step(1);
	item_repeats_spin->set_value(1);
	item_repeats_spin->set_editable(true);
	item_repeats_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_repeats_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(item_repeats_spin);
	item_repeats_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	inv_grid->add_child(item_repeats_spin);

	item_tp_gain_label = memnew(Label(CG_DB_TR("TP:")));
	item_tp_gain_label->set_tooltip_text(CG_DB_TR("Ganancia de PT (TP)"));
	inv_grid->add_child(item_tp_gain_label);
	item_tp_gain_spin = memnew(SpinBox);
	item_tp_gain_spin->set_min(0);
	item_tp_gain_spin->set_max(100);
	item_tp_gain_spin->set_step(1);
	item_tp_gain_spin->set_value(0);
	item_tp_gain_spin->set_editable(true);
	item_tp_gain_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_tp_gain_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(item_tp_gain_spin);
	item_tp_gain_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	inv_grid->add_child(item_tp_gain_spin);

	hit_type_label = memnew(Label(CG_DB_TR("Tipo Golpe:")));
	hit_type_label->set_tooltip_text(CG_DB_TR("Tipo de Golpe"));
	inv_grid->add_child(hit_type_label);
	hit_type_option = memnew(OptionButton);
	hit_type_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hit_type_option->set_custom_minimum_size(Size2(0, row_h));
	hit_type_option->set_clip_text(true);
	hit_type_option->add_item(CG_DB_TR("Golpe Seguro"), 0);
	hit_type_option->add_item(CG_DB_TR("Ataque Físico"), 1);
	hit_type_option->add_item(CG_DB_TR("Ataque Mágico"), 2);
	hit_type_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	inv_grid->add_child(hit_type_option);

	item_element_label = memnew(Label(CG_DB_TR("Elemento:")));
	item_element_label->set_tooltip_text(CG_DB_TR("Elemento de Daño"));
	inv_grid->add_child(item_element_label);
	item_element_option = memnew(OptionButton);
	item_element_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_element_option->set_custom_minimum_size(Size2(0, row_h));
	item_element_option->set_clip_text(true);
	item_element_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	inv_grid->add_child(item_element_option);

	formula_label = memnew(Label(CG_DB_TR("Fórmula Daño:")));
	formula_label->set_tooltip_text(CG_DB_TR("Fórmula de Daño"));
	inv_grid->add_child(formula_label);
	formula_edit = memnew(ClockGearsFormulaLineEdit);
	formula_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	formula_edit->set_custom_minimum_size(Size2(0, row_h));
	formula_edit->set_focus_mode(Control::FOCUS_ALL);
	formula_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	formula_edit->set_editable(true);
	formula_edit->set_placeholder("max(a.atk * 4 - b.def * 2, 0)");
	formula_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_text_changed));
	inv_grid->add_child(formula_edit);

	variance_label = memnew(Label(CG_DB_TR("Varianza (%):")));
	variance_label->set_tooltip_text(CG_DB_TR("Varianza de Daño (%)"));
	inv_grid->add_child(variance_label);
	variance_spin = memnew(SpinBox);
	variance_spin->set_min(0);
	variance_spin->set_max(100);
	variance_spin->set_step(1);
	variance_spin->set_value(20);
	variance_spin->set_editable(true);
	variance_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	variance_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(variance_spin);
	variance_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	inv_grid->add_child(variance_spin);

	critical_label = memnew(Label(CG_DB_TR("Crítico:")));
	critical_label->set_tooltip_text(CG_DB_TR("Golpe Crítico"));
	inv_grid->add_child(critical_label);
	critical_check = memnew(CheckBox(CG_DB_TR("Crítico")));
	critical_check->set_tooltip_text(CG_DB_TR("Permite golpe crítico"));
	critical_check->set_pressed(false);
	critical_check->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	critical_check->set_custom_minimum_size(Size2(0, row_h));
	critical_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_check_toggled));
	inv_grid->add_child(critical_check);

	// ========================================================================
	// SECCIÓN PARÁMETROS (8 Estadísticas en cuadrícula de 2 columnas = 4 cols GridContainer)
	// ========================================================================
	stats_section_box = memnew(VBoxContainer);
	stats_section_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	stats_section_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	col2_box->add_child(stats_section_box);

	Label *lbl_stats_sec = memnew(Label(CG_DB_TR("Parámetros")));
	lbl_stats_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	stats_section_box->add_child(lbl_stats_sec);
	stats_section_box->add_child(memnew(HSeparator));

	GridContainer *stats_grid = memnew(GridContainer);
	stats_grid->set_columns(4);
	stats_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	stats_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	stats_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	stats_section_box->add_child(stats_grid);

	const char *stat_titles[8] = {
		"HP:",
		"MP:",
		"ATK:",
		"DEF:",
		"MAT:",
		"MDF:",
		"AGI:",
		"LUK:"
	};
	const char *stat_tips[8] = {
		"Puntos de Vida Máximos (HP)",
		"Puntos de Magia Máximos (MP)",
		"Ataque Físico (ATK)",
		"Defensa Física (DEF)",
		"Ataque Mágico (MAT)",
		"Defensa Mágica (MDF)",
		"Agilidad (AGI)",
		"Suerte / Valor (LUK)"
	};

	for (int i = 0; i < 8; i++) {
		stat_labels[i] = memnew(Label(CG_DB_TR(stat_titles[i])));
		stat_labels[i]->set_tooltip_text(CG_DB_TR(stat_tips[i]));
		stats_grid->add_child(stat_labels[i]);

		stat_spins[i] = memnew(SpinBox);
		stat_spins[i]->set_min(-999);
		stat_spins[i]->set_max(999999);
		stat_spins[i]->set_step(1);
		stat_spins[i]->set_editable(true);
		stat_spins[i]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		stat_spins[i]->set_custom_minimum_size(Size2(0, row_h));
		_bind_numeric_spinbox(stat_spins[i]);
		stat_spins[i]->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
		stats_grid->add_child(stat_spins[i]);
	}

	// ========================================================================
	// SECCIÓN VELOCIDADES (Actores, NPC, NPC ARPG, Enemigos ARPG)
	// ========================================================================
	speeds_section_box = memnew(VBoxContainer);
	speeds_section_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	speeds_section_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	speeds_section_box->hide();
	col2_box->add_child(speeds_section_box);

	Label *lbl_speeds_sec = memnew(Label(CG_DB_TR("Velocidades")));
	lbl_speeds_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	speeds_section_box->add_child(lbl_speeds_sec);
	speeds_section_box->add_child(memnew(HSeparator));

	GridContainer *speeds_grid = memnew(GridContainer);
	speeds_grid->set_columns(4);
	speeds_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	speeds_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	speeds_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	speeds_section_box->add_child(speeds_grid);

	Label *lbl_walk_spd = memnew(Label(CG_DB_TR("Caminar:")));
	speeds_grid->add_child(lbl_walk_spd);
	speed_walk_spin = memnew(SpinBox);
	speed_walk_spin->set_min(1);
	speed_walk_spin->set_max(500);
	speed_walk_spin->set_step(1);
	speed_walk_spin->set_value(100);
	speed_walk_spin->set_suffix("%");
	speed_walk_spin->set_editable(true);
	speed_walk_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	speed_walk_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(speed_walk_spin);
	speed_walk_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	speeds_grid->add_child(speed_walk_spin);

	Label *lbl_run_spd = memnew(Label(CG_DB_TR("Correr:")));
	speeds_grid->add_child(lbl_run_spd);
	speed_run_spin = memnew(SpinBox);
	speed_run_spin->set_min(1);
	speed_run_spin->set_max(500);
	speed_run_spin->set_step(1);
	speed_run_spin->set_value(100);
	speed_run_spin->set_suffix("%");
	speed_run_spin->set_editable(true);
	speed_run_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	speed_run_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(speed_run_spin);
	speed_run_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	speeds_grid->add_child(speed_run_spin);

	speed_attack_label = memnew(Label(CG_DB_TR("Velocidad al atacar:")));
	speeds_grid->add_child(speed_attack_label);
	speed_attack_spin = memnew(SpinBox);
	speed_attack_spin->set_min(1);
	speed_attack_spin->set_max(500);
	speed_attack_spin->set_step(1);
	speed_attack_spin->set_value(100);
	speed_attack_spin->set_suffix("%");
	speed_attack_spin->set_editable(true);
	speed_attack_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	speed_attack_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(speed_attack_spin);
	speed_attack_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	speeds_grid->add_child(speed_attack_spin);

	speeds_help_label = memnew(Label(CG_DB_TR("100% es la velocidad base del proyecto; valores menores frenan y mayores aceleran.")));
	speeds_help_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	speeds_help_label->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
	speeds_help_label->add_theme_font_size_override("font_size", int(12 * EDSCALE));
	speeds_section_box->add_child(speeds_help_label);

	// ========================================================================
	// SECCIÓN TROPAS (Apilado en vertical: Botones HFlowContainer + Tabla Miembros + Vista Previa 16:9)
	// ========================================================================
	troop_members_box = memnew(VBoxContainer);
	troop_members_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_members_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	troop_members_box->hide();
	col2_box->add_child(troop_members_box);

	HBoxContainer *t_hdr = memnew(HBoxContainer);
	t_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	Label *lbl_t_hdr = memnew(Label(CG_DB_TR("Miembros de tropa")));
	lbl_t_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_t_hdr->set_tooltip_text(CG_DB_TR("Hasta 8 enemigos por tropa"));
	lbl_t_hdr->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	t_hdr->add_child(lbl_t_hdr);

	btn_add_troop_member = memnew(Button(CG_DB_TR("+ Añadir")));
	btn_add_troop_member->set_custom_minimum_size(Size2(0, row_h));
	btn_add_troop_member->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_troop_member_pressed));
	t_hdr->add_child(btn_add_troop_member);
	troop_members_box->add_child(t_hdr);
	troop_members_box->add_child(memnew(HSeparator));

	HFlowContainer *t_actions_flow = memnew(HFlowContainer);
	t_actions_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	t_actions_flow->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	t_actions_flow->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	troop_members_box->add_child(t_actions_flow);

	btn_remove_troop_member_sel = memnew(Button(CG_DB_TR("Eliminar")));
	btn_remove_troop_member_sel->set_custom_minimum_size(Size2(0, row_h));
	btn_remove_troop_member_sel->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_selected_troop_member_pressed));
	t_actions_flow->add_child(btn_remove_troop_member_sel);

	btn_clear_troop_members = memnew(Button(CG_DB_TR("Vaciar")));
	btn_clear_troop_members->set_custom_minimum_size(Size2(0, row_h));
	btn_clear_troop_members->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_clear_troop_members_pressed));
	t_actions_flow->add_child(btn_clear_troop_members);

	btn_align_troop_members = memnew(Button(CG_DB_TR("Alinear")));
	btn_align_troop_members->set_custom_minimum_size(Size2(0, row_h));
	btn_align_troop_members->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_align_troop_members_pressed));
	t_actions_flow->add_child(btn_align_troop_members);

	btn_autoname_troop = memnew(Button(CG_DB_TR("Autonombre")));
	btn_autoname_troop->set_custom_minimum_size(Size2(0, row_h));
	btn_autoname_troop->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_autoname_troop_pressed));
	t_actions_flow->add_child(btn_autoname_troop);

	btn_change_troop_bg = memnew(Button(CG_DB_TR("Cambiar fondo")));
	btn_change_troop_bg->set_custom_minimum_size(Size2(0, row_h));
	btn_change_troop_bg->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_change_troop_bg_pressed));
	t_actions_flow->add_child(btn_change_troop_bg);

	btn_adjust_troop_bg = memnew(Button(CG_DB_TR("Ajustar")));
	btn_adjust_troop_bg->set_custom_minimum_size(Size2(0, row_h));
	btn_adjust_troop_bg->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_troop_bg_pressed));
	t_actions_flow->add_child(btn_adjust_troop_bg);

	troop_bg_adjust_menu = memnew(PopupMenu);
	troop_bg_adjust_menu->add_item(CG_DB_TR("Fondo"), 1);
	troop_bg_adjust_menu->add_item(CG_DB_TR("Suelo"), 2);
	troop_bg_adjust_menu->connect("id_pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_troop_bg_menu_id));
	add_child(troop_bg_adjust_menu);

	troop_ctx_menu = memnew(PopupMenu);
	troop_ctx_menu->set_hide_on_item_selection(true);
	troop_ctx_menu->set_hide_on_checkable_item_selection(true);
	troop_scale_submenu = memnew(PopupMenu);
	troop_scale_submenu->set_hide_on_item_selection(true);
	troop_scale_submenu->set_hide_on_checkable_item_selection(true);
	for (int s = 1; s <= 6; s++) {
		troop_scale_submenu->add_radio_check_item(vformat("x%d", s), s);
	}
	troop_scale_submenu->connect("id_pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_scale_menu_id_pressed));
	troop_ctx_menu->add_child(troop_scale_submenu);
	troop_ctx_menu->add_submenu_node_item(CG_DB_TR("Escala"), troop_scale_submenu, 100);

	troop_hp_bar_submenu = memnew(PopupMenu);
	troop_hp_bar_submenu->set_hide_on_item_selection(true);
	troop_hp_bar_submenu->set_hide_on_checkable_item_selection(true);
	troop_hp_bar_submenu->add_radio_check_item(CG_DB_TR("Arriba"), 1);
	troop_hp_bar_submenu->add_radio_check_item(CG_DB_TR("Abajo"), 2);
	troop_hp_bar_submenu->add_radio_check_item(CG_DB_TR("Mover libremente"), 3);
	troop_hp_bar_submenu->add_radio_check_item(CG_DB_TR("Ocultar barra"), 4);
	troop_hp_bar_submenu->add_separator();
	troop_hp_bar_submenu->add_item(CG_DB_TR("Restablecer"), 5);
	troop_hp_bar_submenu->connect("id_pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_hp_bar_menu_id_pressed));
	troop_ctx_menu->add_child(troop_hp_bar_submenu);
	troop_ctx_menu->add_submenu_node_item(CG_DB_TR("Ajustar barra de vida"), troop_hp_bar_submenu, 101);
	add_child(troop_ctx_menu);

	btn_battle_test_troop = memnew(Button(CG_DB_TR("Prueba de batalla")));
	btn_battle_test_troop->set_custom_minimum_size(Size2(0, row_h));
	btn_battle_test_troop->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_battle_test_troop_pressed));
	t_actions_flow->add_child(btn_battle_test_troop);

	chk_troop_snap_grid = memnew(CheckBox(CG_DB_TR("Ajustar a rejilla (32)")));
	chk_troop_snap_grid->set_custom_minimum_size(Size2(0, row_h));
	chk_troop_snap_grid->set_pressed(false);
	chk_troop_snap_grid->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_queue_troop_preview_redraw).unbind(1));
	t_actions_flow->add_child(chk_troop_snap_grid);

	troop_members_list_vbox = memnew(VBoxContainer);
	troop_members_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_members_list_vbox->add_theme_constant_override("separation", int(6 * EDSCALE));
	troop_members_box->add_child(troop_members_list_vbox);

	// ========================================================================
	// SECCIÓN COMBATE DE ENEMIGOS (En Pestaña Combate: Debilidades 2 cols + Patrones IA)
	// ========================================================================
	enemy_element_rates_box = memnew(VBoxContainer);
	enemy_element_rates_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_element_rates_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	enemy_element_rates_box->hide();
	combat_tab_vbox->add_child(enemy_element_rates_box);

	Label *lbl_elem_rates = memnew(Label(CG_DB_TR("Debilidades y resistencias")));
	lbl_elem_rates->set_tooltip_text(CG_DB_TR("Multiplicador de daño elemental recibido por el enemigo (%)"));
	lbl_elem_rates->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	enemy_element_rates_box->add_child(lbl_elem_rates);
	enemy_element_rates_box->add_child(memnew(HSeparator));

	enemy_element_rates_rows_vbox = memnew(GridContainer);
	enemy_element_rates_rows_vbox->set_columns(2); // Cuadrícula de 2 columnas
	enemy_element_rates_rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_element_rates_rows_vbox->add_theme_constant_override("h_separation", int(16 * EDSCALE));
	enemy_element_rates_rows_vbox->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	enemy_element_rates_box->add_child(enemy_element_rates_rows_vbox);

	// ========================================================================
	// SECCIÓN TILESETS (Izquierda: General + Ranuras | Derecha: Hoja vertical completa)
	// ========================================================================
	tileset_main_hbox = memnew(HBoxContainer);
	tileset_main_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_main_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_main_hbox->add_theme_constant_override("separation", int(12 * EDSCALE));
	tileset_main_hbox->connect("resized", callable_mp(this, &ClockGearsDatabaseDialog::_update_tileset_split_layout));
	tileset_main_hbox->hide();
	col2_box->add_child(tileset_main_hbox);

	tileset_left_col = memnew(VBoxContainer);
	tileset_left_col->set_custom_minimum_size(Size2(240 * EDSCALE, 0));
	tileset_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_left_col->set_stretch_ratio(0.36f);
	tileset_left_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	tileset_left_col->hide();
	tileset_main_hbox->add_child(tileset_left_col);

	tileset_slots_scroll = memnew(ScrollContainer);
	tileset_slots_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_slots_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_slots_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	tileset_slots_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	tileset_left_col->add_child(tileset_slots_scroll);

	tileset_slots_box = memnew(VBoxContainer);
	tileset_slots_box->set_custom_minimum_size(Size2(220 * EDSCALE, 0));
	tileset_slots_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_slots_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	tileset_slots_box->hide();
	tileset_slots_scroll->add_child(tileset_slots_box);

	HBoxContainer *slots_hdr_hb = memnew(HBoxContainer);
	slots_hdr_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slots_hdr_hb->add_theme_constant_override("separation", 4);
	Label *slots_header = memnew(Label(CG_DB_TR("Ranuras de tiles")));
	slots_header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slots_header->set_clip_text(true);
	slots_header->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	slots_hdr_hb->add_child(slots_header);
	Button *btn_slots_help = memnew(Button("?"));
	btn_slots_help->set_custom_minimum_size(Size2(22 * EDSCALE, 22 * EDSCALE));
	btn_slots_help->set_tooltip_text(CG_DB_TR("Ranuras de hojas de tiles (32x32 Anexo A4):\nA1: Agua / Animados\nA2: Suelo / Autotiles\nA3: Edificios / Fachadas\nA4: Muros / Techos\nA5: Suelo y muros normales\nB-E: Objetos, vegetación y decoración"));
	slots_hdr_hb->add_child(btn_slots_help);
	tileset_slots_box->add_child(slots_hdr_hb);
	tileset_slots_box->add_child(memnew(HSeparator));

	for (int s = 0; s < TILESET_SLOT_COUNT; s++) {
		HBoxContainer *sl_row = memnew(HBoxContainer);
		sl_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sl_row->add_theme_constant_override("separation", 4);
		tileset_slots_box->add_child(sl_row);

		Label *sl_lbl = memnew(Label(_get_slot_key(s)));
		sl_lbl->set_custom_minimum_size(Size2(28 * EDSCALE, 0));
		sl_lbl->set_tooltip_text(_get_slot_label(s));
		sl_row->add_child(sl_lbl);

		tileset_slot_edits[s] = memnew(LineEdit);
		tileset_slot_edits[s]->set_editable(false);
		tileset_slot_edits[s]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		tileset_slot_edits[s]->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
		tileset_slot_edits[s]->set_text(CG_DB_TR("(Vacío)"));
		sl_row->add_child(tileset_slot_edits[s]);

		btn_load_slot[s] = memnew(Button);
		if (has_theme_icon(SNAME("Load"), SNAME("EditorIcons"))) {
			btn_load_slot[s]->set_button_icon(get_editor_theme_icon(SNAME("Load")));
		} else {
			btn_load_slot[s]->set_text("+");
		}
		btn_load_slot[s]->set_tooltip_text(vformat(CG_DB_TR("Cargar hoja PNG 32x32 en la ranura %s (%s)"), _get_slot_key(s), _get_slot_label(s)));
		btn_load_slot[s]->set_custom_minimum_size(Size2(28 * EDSCALE, row_h));
		btn_load_slot[s]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_load_slot_button_pressed).bind(s));
		sl_row->add_child(btn_load_slot[s]);

		btn_clear_slot[s] = memnew(Button);
		if (has_theme_icon(SNAME("Close"), SNAME("EditorIcons"))) {
			btn_clear_slot[s]->set_button_icon(get_editor_theme_icon(SNAME("Close")));
		} else {
			btn_clear_slot[s]->set_text("-");
		}
		btn_clear_slot[s]->set_tooltip_text(vformat(CG_DB_TR("Quitar la hoja asignada a la ranura %s"), _get_slot_key(s)));
		btn_clear_slot[s]->set_custom_minimum_size(Size2(28 * EDSCALE, row_h));
		btn_clear_slot[s]->set_disabled(true);
		btn_clear_slot[s]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_clear_slot_button_pressed).bind(s));
		sl_row->add_child(btn_clear_slot[s]);
	}

	// ========================================================================
	// SECCIÓN BOTÍN (Drops de Enemigos en General)
	// ========================================================================
	enemy_drops_box = memnew(VBoxContainer);
	enemy_drops_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_drops_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	enemy_drops_box->hide();
	col2_box->add_child(enemy_drops_box);

	Label *lbl_drops_sec = memnew(Label(CG_DB_TR("Botín del enemigo")));
	lbl_drops_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	enemy_drops_box->add_child(lbl_drops_sec);
	enemy_drops_box->add_child(memnew(HSeparator));

	for (int s = 0; s < 3; s++) {
		HBoxContainer *dr_row = memnew(HBoxContainer);
		dr_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dr_row->add_theme_constant_override("separation", int(8 * EDSCALE));
		enemy_drops_box->add_child(dr_row);

		Label *dr_lbl = memnew(Label(vformat(CG_DB_TR("Drop %d:"), s + 1)));
		dr_lbl->set_custom_minimum_size(Size2(60 * EDSCALE, 0));
		dr_row->add_child(dr_lbl);

		drop_kind_opt[s] = memnew(OptionButton);
		drop_kind_opt[s]->set_custom_minimum_size(Size2(130 * EDSCALE, row_h));
		drop_kind_opt[s]->set_clip_text(true);
		drop_kind_opt[s]->add_item(CG_DB_TR("Ninguno"), 0);
		drop_kind_opt[s]->add_item(CG_DB_TR("Objeto"), CAT_ITEMS);
		drop_kind_opt[s]->add_item(CG_DB_TR("Arma"), CAT_WEAPONS);
		drop_kind_opt[s]->add_item(CG_DB_TR("Armadura"), CAT_ARMORS);
		drop_kind_opt[s]->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_drop_kind_changed).bind(s));
		dr_row->add_child(drop_kind_opt[s]);

		drop_item_opt[s] = memnew(OptionButton);
		drop_item_opt[s]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		drop_item_opt[s]->set_custom_minimum_size(Size2(160 * EDSCALE, row_h));
		drop_item_opt[s]->set_clip_text(true);
		drop_item_opt[s]->add_item(CG_DB_TR("(ninguno)"), 0);
		drop_item_opt[s]->set_disabled(true);
		drop_item_opt[s]->connect("item_selected", callable_mp_static(+[](int p_i, ClockGearsDatabaseDialog *p_self, int p_sl) {
			if (p_self) {
				p_self->_on_drop_field_changed(p_sl);
			}
		}).bind(this, s));
		dr_row->add_child(drop_item_opt[s]);

		drop_prob_spin[s] = memnew(SpinBox);
		drop_prob_spin[s]->set_custom_minimum_size(Size2(85 * EDSCALE, row_h));
		drop_prob_spin[s]->set_min(1);
		drop_prob_spin[s]->set_max(100);
		drop_prob_spin[s]->set_step(1);
		drop_prob_spin[s]->set_value(100);
		drop_prob_spin[s]->set_suffix("%");
		_bind_numeric_spinbox(drop_prob_spin[s]);
		drop_prob_spin[s]->connect("value_changed", callable_mp_static(+[](double p_v, ClockGearsDatabaseDialog *p_self, int p_sl) {
			if (p_self) {
				p_self->_on_drop_field_changed(p_sl);
			}
		}).bind(this, s));
		dr_row->add_child(drop_prob_spin[s]);
	}

	// ========================================================================
	// PATRONES DE ACCIÓN IA (En Pestaña Combate de Enemigos)
	// ========================================================================
	enemy_actions_box = memnew(VBoxContainer);
	enemy_actions_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_actions_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	enemy_actions_box->hide();
	combat_tab_vbox->add_child(enemy_actions_box);

	Label *lbl_act_sec = memnew(Label(CG_DB_TR("Patrones de acción")));
	lbl_act_sec->set_tooltip_text(CG_DB_TR("Habilidades y condiciones de inteligencia artificial del enemigo"));
	lbl_act_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	enemy_actions_box->add_child(lbl_act_sec);
	enemy_actions_box->add_child(memnew(HSeparator));

	enemy_actions_tree = memnew(Tree);
	enemy_actions_tree->set_columns(4);
	enemy_actions_tree->set_column_titles_visible(true);
	enemy_actions_tree->set_column_title(0, CG_DB_TR("Habilidad"));
	enemy_actions_tree->set_column_title(1, CG_DB_TR("Condición"));
	enemy_actions_tree->set_column_title(2, CG_DB_TR("Parám."));
	enemy_actions_tree->set_column_title(3, CG_DB_TR("Prioridad"));
	enemy_actions_tree->set_column_expand(0, true);
	enemy_actions_tree->set_column_expand_ratio(0, 3);
	enemy_actions_tree->set_column_expand(1, true);
	enemy_actions_tree->set_column_expand_ratio(1, 3);
	enemy_actions_tree->set_column_expand(2, true);
	enemy_actions_tree->set_column_expand_ratio(2, 2);
	enemy_actions_tree->set_column_expand(3, true);
	enemy_actions_tree->set_column_expand_ratio(3, 1);
	enemy_actions_tree->set_hide_root(true);
	enemy_actions_tree->set_custom_minimum_size(Size2(0, 140 * EDSCALE));
	enemy_actions_box->add_child(enemy_actions_tree);

	HBoxContainer *ap_btn_row = memnew(HBoxContainer);
	ap_btn_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	enemy_actions_box->add_child(ap_btn_row);

	btn_add_action_pattern = memnew(Button(CG_DB_TR("+ Añadir patrón")));
	btn_add_action_pattern->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_add_action_pattern->set_custom_minimum_size(Size2(0, row_h));
	btn_add_action_pattern->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_action_pattern_pressed));
	ap_btn_row->add_child(btn_add_action_pattern);

	btn_remove_action_pattern = memnew(Button(CG_DB_TR("- Eliminar patrón")));
	btn_remove_action_pattern->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_remove_action_pattern->set_custom_minimum_size(Size2(0, row_h));
	btn_remove_action_pattern->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_action_pattern_pressed));
	ap_btn_row->add_child(btn_remove_action_pattern);

	action_pattern_dialog = memnew(AcceptDialog);
	action_pattern_dialog->set_title(CG_DB_TR("Añadir Patrón de Acción (IA)"));
	GridContainer *ap_grid = memnew(GridContainer);
	ap_grid->set_columns(2);
	ap_grid->add_theme_constant_override("h_separation", 10);
	ap_grid->add_theme_constant_override("v_separation", 8);
	ap_grid->add_child(memnew(Label(CG_DB_TR("Habilidad:"))));
	ap_skill_opt = memnew(OptionButton);
	ap_skill_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ap_grid->add_child(ap_skill_opt);
	ap_grid->add_child(memnew(Label(CG_DB_TR("Condición:"))));
	ap_cond_opt = memnew(OptionButton);
	ap_cond_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ap_grid->add_child(ap_cond_opt);
	ap_grid->add_child(memnew(Label(CG_DB_TR("Parámetro A (Turno/Mín%/ID):"))));
	ap_cond_a_spin = memnew(SpinBox);
	ap_cond_a_spin->set_min(0);
	ap_cond_a_spin->set_max(9999);
	_bind_numeric_spinbox(ap_cond_a_spin);
	ap_grid->add_child(ap_cond_a_spin);
	ap_grid->add_child(memnew(Label(CG_DB_TR("Parámetro B (Paso/Máx%):"))));
	ap_cond_b_spin = memnew(SpinBox);
	ap_cond_b_spin->set_min(0);
	ap_cond_b_spin->set_max(9999);
	ap_cond_b_spin->set_value(100);
	_bind_numeric_spinbox(ap_cond_b_spin);
	ap_grid->add_child(ap_cond_b_spin);
	ap_grid->add_child(memnew(Label(CG_DB_TR("Prioridad (1..10):"))));
	ap_rating_spin = memnew(SpinBox);
	ap_rating_spin->set_min(1);
	ap_rating_spin->set_max(10);
	ap_rating_spin->set_value(5);
	_bind_numeric_spinbox(ap_rating_spin);
	ap_grid->add_child(ap_rating_spin);
	action_pattern_dialog->add_child(ap_grid);
	action_pattern_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_action_pattern_confirmed));
	add_child(action_pattern_dialog);

	// ========================================================================
	// SECCIÓN IA DE ENEMIGOS ARPG (En Pestaña Combate, bajo Debilidades)
	// ========================================================================
	arpg_ai_box = memnew(VBoxContainer);
	arpg_ai_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_ai_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	arpg_ai_box->hide();
	combat_tab_vbox->add_child(arpg_ai_box);

	Label *lbl_arpg_ai = memnew(Label(CG_DB_TR("IA")));
	lbl_arpg_ai->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	arpg_ai_box->add_child(lbl_arpg_ai);
	arpg_ai_box->add_child(memnew(HSeparator));

	GridContainer *arpg_ai_grid = memnew(GridContainer);
	arpg_ai_grid->set_columns(4);
	arpg_ai_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_ai_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	arpg_ai_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	arpg_ai_box->add_child(arpg_ai_grid);

	arpg_ai_grid->add_child(memnew(Label(CG_DB_TR("Tipo de IA:"))));
	arpg_ai_type_opt = memnew(OptionButton);
	arpg_ai_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_ai_type_opt->set_custom_minimum_size(Size2(0, row_h));
	arpg_ai_type_opt->set_clip_text(true);
	arpg_ai_type_opt->add_item(CG_DB_TR("Quieto"), 0);
	arpg_ai_type_opt->set_item_metadata(0, "quieto");
	arpg_ai_type_opt->add_item(CG_DB_TR("Patrulla"), 1);
	arpg_ai_type_opt->set_item_metadata(1, "patrulla");
	arpg_ai_type_opt->add_item(CG_DB_TR("Persigue y ataca"), 2);
	arpg_ai_type_opt->set_item_metadata(2, "persigue_y_ataca");
	arpg_ai_type_opt->add_item(CG_DB_TR("Huye"), 3);
	arpg_ai_type_opt->set_item_metadata(3, "huye");
	arpg_ai_type_opt->add_item(CG_DB_TR("A distancia"), 4);
	arpg_ai_type_opt->set_item_metadata(4, "a_distancia");
	arpg_ai_type_opt->select(2);
	arpg_ai_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_ai_field_changed).unbind(1));
	arpg_ai_grid->add_child(arpg_ai_type_opt);

	arpg_ai_grid->add_child(memnew(Label(CG_DB_TR("Radio de visión:"))));
	arpg_ai_vision_spin = memnew(SpinBox);
	arpg_ai_vision_spin->set_min(0.0);
	arpg_ai_vision_spin->set_max(100.0);
	arpg_ai_vision_spin->set_step(0.5);
	arpg_ai_vision_spin->set_value(6.0);
	arpg_ai_vision_spin->set_suffix(" tiles");
	arpg_ai_vision_spin->set_editable(true);
	arpg_ai_vision_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_ai_vision_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_ai_vision_spin);
	arpg_ai_vision_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_ai_field_changed).unbind(1));
	arpg_ai_grid->add_child(arpg_ai_vision_spin);

	arpg_ai_grid->add_child(memnew(Label(CG_DB_TR("Resistencia al retroceso:"))));
	arpg_ai_kb_resist_spin = memnew(SpinBox);
	arpg_ai_kb_resist_spin->set_min(0);
	arpg_ai_kb_resist_spin->set_max(100);
	arpg_ai_kb_resist_spin->set_step(1);
	arpg_ai_kb_resist_spin->set_value(0);
	arpg_ai_kb_resist_spin->set_suffix("%");
	arpg_ai_kb_resist_spin->set_editable(true);
	arpg_ai_kb_resist_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_ai_kb_resist_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_ai_kb_resist_spin);
	arpg_ai_kb_resist_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_ai_field_changed).unbind(1));
	arpg_ai_grid->add_child(arpg_ai_kb_resist_spin);

	// ========================================================================
	// SECCIÓN ATAQUE DE ENEMIGOS ARPG (En Pestaña Combate, sección "Ataque")
	// ========================================================================
	arpg_attack_box = memnew(VBoxContainer);
	arpg_attack_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_attack_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	arpg_attack_box->hide();
	combat_tab_vbox->add_child(arpg_attack_box);

	Label *lbl_arpg_attack = memnew(Label(CG_DB_TR("Ataque")));
	lbl_arpg_attack->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	arpg_attack_box->add_child(lbl_arpg_attack);
	arpg_attack_box->add_child(memnew(HSeparator));

	GridContainer *arpg_attack_grid = memnew(GridContainer);
	arpg_attack_grid->set_columns(4);
	arpg_attack_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_attack_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	arpg_attack_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	arpg_attack_box->add_child(arpg_attack_grid);

	// 1. Frecuencia de ataque (tiempo entre ataques)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Tiempo entre ataques:"))));
	arpg_atk_interval_spin = memnew(SpinBox);
	arpg_atk_interval_spin->set_min(0.0);
	arpg_atk_interval_spin->set_max(60.0);
	arpg_atk_interval_spin->set_step(0.1);
	arpg_atk_interval_spin->set_value(1.5);
	arpg_atk_interval_spin->set_suffix(" s");
	arpg_atk_interval_spin->set_editable(true);
	arpg_atk_interval_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_interval_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_interval_spin);
	arpg_atk_interval_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_interval_spin);
	arpg_ai_interval_spin = arpg_atk_interval_spin;

	// 1b. Variación aleatoria (±%)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Variación aleatoria:"))));
	arpg_atk_interval_var_spin = memnew(SpinBox);
	arpg_atk_interval_var_spin->set_min(0.0);
	arpg_atk_interval_var_spin->set_max(100.0);
	arpg_atk_interval_var_spin->set_step(1.0);
	arpg_atk_interval_var_spin->set_value(0.0);
	arpg_atk_interval_var_spin->set_prefix(U"±");
	arpg_atk_interval_var_spin->set_suffix("%");
	arpg_atk_interval_var_spin->set_editable(true);
	arpg_atk_interval_var_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_interval_var_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_interval_var_spin);
	arpg_atk_interval_var_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_interval_var_spin);

	// 2. Velocidad de ataque (% que acelera o frena la animación)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Velocidad de ataque:"))));
	arpg_atk_speed_spin = memnew(SpinBox);
	arpg_atk_speed_spin->set_min(1.0);
	arpg_atk_speed_spin->set_max(1000.0);
	arpg_atk_speed_spin->set_step(5.0);
	arpg_atk_speed_spin->set_value(100.0);
	arpg_atk_speed_spin->set_suffix("%");
	arpg_atk_speed_spin->set_editable(true);
	arpg_atk_speed_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_speed_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_speed_spin);
	arpg_atk_speed_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_speed_spin);

	// 3. Anticipación (ms antes de golpear)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Anticipación:"))));
	arpg_atk_windup_spin = memnew(SpinBox);
	arpg_atk_windup_spin->set_min(0);
	arpg_atk_windup_spin->set_max(5000);
	arpg_atk_windup_spin->set_step(50);
	arpg_atk_windup_spin->set_value(0);
	arpg_atk_windup_spin->set_suffix(" ms");
	arpg_atk_windup_spin->set_editable(true);
	arpg_atk_windup_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_windup_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_windup_spin);
	arpg_atk_windup_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_windup_spin);

	// 4. Recuperación (ms tras atacar antes de volver a moverse)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Recuperación:"))));
	arpg_atk_recovery_spin = memnew(SpinBox);
	arpg_atk_recovery_spin->set_min(0);
	arpg_atk_recovery_spin->set_max(5000);
	arpg_atk_recovery_spin->set_step(50);
	arpg_atk_recovery_spin->set_value(0);
	arpg_atk_recovery_spin->set_suffix(" ms");
	arpg_atk_recovery_spin->set_editable(true);
	arpg_atk_recovery_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_recovery_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_recovery_spin);
	arpg_atk_recovery_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_recovery_spin);

	// 5. Multiplicador de daño (%)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Multiplicador daño:"))));
	arpg_atk_dmg_mult_spin = memnew(SpinBox);
	arpg_atk_dmg_mult_spin->set_min(0.0);
	arpg_atk_dmg_mult_spin->set_max(1000.0);
	arpg_atk_dmg_mult_spin->set_step(5.0);
	arpg_atk_dmg_mult_spin->set_value(100.0);
	arpg_atk_dmg_mult_spin->set_suffix("%");
	arpg_atk_dmg_mult_spin->set_editable(true);
	arpg_atk_dmg_mult_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_dmg_mult_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_dmg_mult_spin);
	arpg_atk_dmg_mult_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_dmg_mult_spin);

	// 6. Alcance (ya existe)
	arpg_attack_grid->add_child(memnew(Label(CG_DB_TR("Alcance:"))));
	arpg_atk_range_spin = memnew(SpinBox);
	arpg_atk_range_spin->set_min(0.0);
	arpg_atk_range_spin->set_max(100.0);
	arpg_atk_range_spin->set_step(0.5);
	arpg_atk_range_spin->set_value(1.0);
	arpg_atk_range_spin->set_suffix(" tiles");
	arpg_atk_range_spin->set_editable(true);
	arpg_atk_range_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_atk_range_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(arpg_atk_range_spin);
	arpg_atk_range_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_attack_field_changed));
	arpg_attack_grid->add_child(arpg_atk_range_spin);
	arpg_ai_range_spin = arpg_atk_range_spin;

	// ========================================================================
	// SYSTEM CONFIG PANEL (CAT_SYSTEM)
	// ========================================================================
	system_panel = memnew(VBoxContainer);
	system_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	system_panel->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	system_panel->add_theme_constant_override("separation", int(16 * EDSCALE));
	system_panel->hide();
	col2_box->add_child(system_panel);

	GridContainer *sys_grid = memnew(GridContainer);
	sys_grid->set_columns(2);
	sys_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_grid->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	sys_grid->add_theme_constant_override("h_separation", int(16 * EDSCALE));
	sys_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	system_panel->add_child(sys_grid);

	Label *lbl_sys_title = memnew(Label(CG_DB_TR("Título del Proyecto RPG:")));
	lbl_sys_title->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_title);
	sys_game_title_edit = memnew(LineEdit);
	sys_game_title_edit->set_custom_minimum_size(Size2(280 * EDSCALE, row_h));
	sys_game_title_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_game_title_edit->set_editable(true);
	sys_game_title_edit->set_focus_mode(Control::FOCUS_ALL);
	sys_game_title_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	sys_game_title_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_text_changed));
	sys_grid->add_child(sys_game_title_edit);

	Label *lbl_sys_curr = memnew(Label(CG_DB_TR("Unidad Monetaria:")));
	lbl_sys_curr->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_curr);
	sys_currency_edit = memnew(LineEdit);
	sys_currency_edit->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	sys_currency_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_currency_edit->set_text("G");
	sys_currency_edit->set_editable(true);
	sys_currency_edit->set_focus_mode(Control::FOCUS_ALL);
	sys_currency_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	sys_currency_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_text_changed));
	sys_grid->add_child(sys_currency_edit);

	Label *lbl_sys_combat = memnew(Label(CG_DB_TR("Sistema de Combate:")));
	lbl_sys_combat->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_combat);
	combat_mode_selector = memnew(OptionButton);
	combat_mode_selector->set_custom_minimum_size(Size2(280 * EDSCALE, row_h));
	combat_mode_selector->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	combat_mode_selector->add_item(CG_DB_TR("Combate por Turnos Clásico (Turn-Based)"), 0);
	combat_mode_selector->add_item(CG_DB_TR("Combate en Tiempo Real (Action RPG)"), 1);
	combat_mode_selector->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_mode_selected));
	sys_grid->add_child(combat_mode_selector);

	lbl_sys_arpg_atk_move = memnew(Label(CG_DB_TR("Ataque (Action RPG):")));
	lbl_sys_arpg_atk_move->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_arpg_atk_move->set_visible(false);
	sys_grid->add_child(lbl_sys_arpg_atk_move);
	sys_arpg_move_atk_chk = memnew(CheckBox(CG_DB_TR("Moverse mientras atacas")));
	sys_arpg_move_atk_chk->set_custom_minimum_size(Size2(220 * EDSCALE, row_h));
	sys_arpg_move_atk_chk->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_move_atk_chk->set_pressed(true);
	sys_arpg_move_atk_chk->set_visible(false);
	sys_arpg_move_atk_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_arpg_controls_changed).unbind(1));
	sys_grid->add_child(sys_arpg_move_atk_chk);

	lbl_sys_arpg_atk_spd = memnew(Label(CG_DB_TR("Velocidad al atacar (%):")));
	lbl_sys_arpg_atk_spd->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_arpg_atk_spd->set_visible(false);
	sys_grid->add_child(lbl_sys_arpg_atk_spd);
	sys_arpg_atk_spd_spin = memnew(SpinBox);
	sys_arpg_atk_spd_spin->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	sys_arpg_atk_spd_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_atk_spd_spin->set_min(0);
	sys_arpg_atk_spd_spin->set_max(200);
	sys_arpg_atk_spd_spin->set_step(5);
	sys_arpg_atk_spd_spin->set_value(100);
	sys_arpg_atk_spd_spin->set_suffix("%");
	sys_arpg_atk_spd_spin->set_visible(false);
	_bind_numeric_spinbox(sys_arpg_atk_spd_spin);
	sys_arpg_atk_spd_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_arpg_controls_changed).unbind(1));
	sys_grid->add_child(sys_arpg_atk_spd_spin);

	lbl_sys_arpg_blk_move = memnew(Label(CG_DB_TR("Bloqueo (Action RPG):")));
	lbl_sys_arpg_blk_move->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_arpg_blk_move->set_visible(false);
	sys_grid->add_child(lbl_sys_arpg_blk_move);
	sys_arpg_move_blk_chk = memnew(CheckBox(CG_DB_TR("Moverse mientras bloqueas")));
	sys_arpg_move_blk_chk->set_custom_minimum_size(Size2(220 * EDSCALE, row_h));
	sys_arpg_move_blk_chk->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_move_blk_chk->set_pressed(false);
	sys_arpg_move_blk_chk->set_visible(false);
	sys_arpg_move_blk_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_arpg_controls_changed).unbind(1));
	sys_grid->add_child(sys_arpg_move_blk_chk);

	lbl_sys_arpg_blk_spd = memnew(Label(CG_DB_TR("Velocidad al bloquear (%):")));
	lbl_sys_arpg_blk_spd->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_arpg_blk_spd->set_visible(false);
	sys_grid->add_child(lbl_sys_arpg_blk_spd);
	sys_arpg_blk_spd_spin = memnew(SpinBox);
	sys_arpg_blk_spd_spin->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	sys_arpg_blk_spd_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_blk_spd_spin->set_min(0);
	sys_arpg_blk_spd_spin->set_max(200);
	sys_arpg_blk_spd_spin->set_step(5);
	sys_arpg_blk_spd_spin->set_value(50);
	sys_arpg_blk_spd_spin->set_suffix("%");
	sys_arpg_blk_spd_spin->set_visible(false);
	_bind_numeric_spinbox(sys_arpg_blk_spd_spin);
	sys_arpg_blk_spd_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_arpg_controls_changed).unbind(1));
	sys_grid->add_child(sys_arpg_blk_spd_spin);

	Label *lbl_sys_dirs = memnew(Label(CG_DB_TR("Direcciones de movimiento:")));
	lbl_sys_dirs->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_dirs);
	sys_movement_dirs_opt = memnew(OptionButton);
	sys_movement_dirs_opt->set_custom_minimum_size(Size2(280 * EDSCALE, row_h));
	sys_movement_dirs_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_movement_dirs_opt->add_item(CG_DB_TR("4 direcciones (Clásico)"), 4);
	sys_movement_dirs_opt->add_item(CG_DB_TR("8 direcciones (Cardinales + Diagonales)"), 8);
	sys_movement_dirs_opt->select(0);
	sys_movement_dirs_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_movement_dirs_selected));
	sys_grid->add_child(sys_movement_dirs_opt);

	Label *lbl_sys_mannequin = memnew(Label(CG_DB_TR("Tamaño del maniquí:")));
	lbl_sys_mannequin->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_mannequin);
	sys_mannequin_size_opt = memnew(OptionButton);
	sys_mannequin_size_opt->set_custom_minimum_size(Size2(280 * EDSCALE, row_h));
	sys_mannequin_size_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_mannequin_size_opt->add_item(CG_DB_TR("32x32 px"), 32);
	sys_mannequin_size_opt->add_item(CG_DB_TR("48x48 px"), 48);
	sys_mannequin_size_opt->add_item(CG_DB_TR("64x64 px"), 64);
	sys_mannequin_size_opt->select(0);
	sys_mannequin_size_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_mannequin_size_selected));
	sys_grid->add_child(sys_mannequin_size_opt);

	Label *lbl_sys_map = memnew(Label(CG_DB_TR("Mapa Inicial:")));
	lbl_sys_map->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_map);
	sys_start_map_option = memnew(OptionButton);
	sys_start_map_option->set_custom_minimum_size(Size2(280 * EDSCALE, row_h));
	sys_start_map_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_start_map_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_start_map_selected));
	sys_grid->add_child(sys_start_map_option);

	Label *lbl_sys_x = memnew(Label(CG_DB_TR("Posición Inicial X (Celda):")));
	lbl_sys_x->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_x);
	sys_start_x_spin = memnew(SpinBox);
	sys_start_x_spin->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	sys_start_x_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_start_x_spin->set_max(999);
	sys_start_x_spin->set_editable(true);
	_bind_numeric_spinbox(sys_start_x_spin);
	sys_start_x_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_spin_changed));
	sys_grid->add_child(sys_start_x_spin);

	Label *lbl_sys_y = memnew(Label(CG_DB_TR("Posición Inicial Y (Celda):")));
	lbl_sys_y->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_y);
	sys_start_y_spin = memnew(SpinBox);
	sys_start_y_spin->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	sys_start_y_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_start_y_spin->set_max(999);
	sys_start_y_spin->set_editable(true);
	_bind_numeric_spinbox(sys_start_y_spin);
	sys_start_y_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_spin_changed));
	sys_grid->add_child(sys_start_y_spin);

	Label *lbl_sys_zoom = memnew(Label(CG_DB_TR("Zoom de Cámara (Pixel Art):")));
	lbl_sys_zoom->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_zoom);
	sys_camera_zoom_opt = memnew(OptionButton);
	sys_camera_zoom_opt->set_custom_minimum_size(Size2(280 * EDSCALE, row_h));
	sys_camera_zoom_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_camera_zoom_opt->add_item(CG_DB_TR("1x (Resolución Nativa 1:1)"), 1);
	sys_camera_zoom_opt->add_item(CG_DB_TR("2x (Pixel Art Estándar)"), 2);
	sys_camera_zoom_opt->add_item(CG_DB_TR("3x (Pixel Art Aumentado)"), 3);
	sys_camera_zoom_opt->add_item(CG_DB_TR("4x (Máximo Aumento)"), 4);
	sys_camera_zoom_opt->select(1);
	sys_camera_zoom_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	sys_grid->add_child(sys_camera_zoom_opt);

	Label *lbl_sys_cam_opts = memnew(Label(CG_DB_TR("Comportamiento de Cámara:")));
	lbl_sys_cam_opts->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	sys_grid->add_child(lbl_sys_cam_opts);

	VBoxContainer *cam_flags_vb = memnew(VBoxContainer);
	cam_flags_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_camera_smooth_check = memnew(CheckBox(CG_DB_TR("Seguimiento Suave (Interpolación fluida al jugador)")));
	sys_camera_smooth_check->set_pressed(true);
	sys_camera_smooth_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	cam_flags_vb->add_child(sys_camera_smooth_check);

	sys_camera_clamp_check = memnew(CheckBox(CG_DB_TR("Restringir Cámara a los Límites del Mapa")));
	sys_camera_clamp_check->set_pressed(true);
	sys_camera_clamp_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	cam_flags_vb->add_child(sys_camera_clamp_check);
	sys_grid->add_child(cam_flags_vb);

	sys_arpg_realtime_panel = memnew(PanelContainer);
	sys_arpg_realtime_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_realtime_panel->set_visible(false);
	{
		Ref<StyleBoxFlat> rt_sb = memnew(StyleBoxFlat);
		rt_sb->set_anti_aliased(true);
		rt_sb->set_bg_color(Color(0.11f, 0.13f, 0.18f, 0.92f));
		rt_sb->set_border_width_all(1);
		rt_sb->set_border_color(Color(0.32f, 0.56f, 0.88f, 0.38f));
		rt_sb->set_corner_radius_all(int(8 * EDSCALE));
		rt_sb->set_content_margin_all(int(14 * EDSCALE));
		sys_arpg_realtime_panel->add_theme_style_override("panel", rt_sb);
	}
	system_panel->add_child(sys_arpg_realtime_panel);

	VBoxContainer *rt_vb = memnew(VBoxContainer);
	rt_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rt_vb->add_theme_constant_override("separation", int(10 * EDSCALE));
	sys_arpg_realtime_panel->add_child(rt_vb);

	Label *lbl_rt_sec = memnew(Label(CG_DB_TR("Combate en tiempo real")));
	lbl_rt_sec->add_theme_color_override("font_color", Color(0.46f, 0.78f, 1.0f));
	rt_vb->add_child(lbl_rt_sec);
	rt_vb->add_child(memnew(HSeparator));

	GridContainer *rt_grid = memnew(GridContainer);
	rt_grid->set_columns(2);
	rt_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rt_grid->add_theme_constant_override("h_separation", int(14 * EDSCALE));
	rt_grid->add_theme_constant_override("v_separation", int(10 * EDSCALE));
	rt_vb->add_child(rt_grid);

	_add_realtime_slider_spin_row(rt_grid, CG_DB_TR("Invulnerabilidad tras daño (s):"), 0.0, 3.0, 0.05, 0.5, " s", sys_arpg_invuln_slider, sys_arpg_invuln_spin);
	_add_realtime_slider_spin_row(rt_grid, CG_DB_TR("Reducción del bloqueo (%):"), 0.0, 100.0, 5.0, 50.0, "%", sys_arpg_block_red_slider, sys_arpg_block_red_spin);

	Label *lbl_dodge_type = memnew(Label(CG_DB_TR("Tipo de esquivar:")));
	lbl_dodge_type->set_custom_minimum_size(Size2(185 * EDSCALE, 0));
	rt_grid->add_child(lbl_dodge_type);
	sys_arpg_dodge_type_opt = memnew(OptionButton);
	sys_arpg_dodge_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_dodge_type_opt->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	sys_arpg_dodge_type_opt->add_item(CG_DB_TR("Voltereta"), 0);
	sys_arpg_dodge_type_opt->add_item(CG_DB_TR("Dash"), 1);
	sys_arpg_dodge_type_opt->select(0);
	sys_arpg_dodge_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	rt_grid->add_child(sys_arpg_dodge_type_opt);

	_add_realtime_slider_spin_row(rt_grid, CG_DB_TR("Duración de esquivar (s):"), 0.05, 2.0, 0.05, 0.25, " s", sys_arpg_dodge_dur_slider, sys_arpg_dodge_dur_spin);
	_add_realtime_slider_spin_row(rt_grid, CG_DB_TR("Distancia de esquivar (px):"), 16.0, 320.0, 8.0, 96.0, " px", sys_arpg_dodge_dist_slider, sys_arpg_dodge_dist_spin);
	_add_realtime_slider_spin_row(rt_grid, CG_DB_TR("Enfriamiento de esquivar (s):"), 0.0, 5.0, 0.05, 0.4, " s", sys_arpg_dodge_cd_slider, sys_arpg_dodge_cd_spin);

	Label *lbl_show_dmg = memnew(Label(CG_DB_TR("Indicadores en pantalla:")));
	lbl_show_dmg->set_custom_minimum_size(Size2(185 * EDSCALE, 0));
	rt_grid->add_child(lbl_show_dmg);
	sys_arpg_show_dmg_chk = memnew(CheckBox(CG_DB_TR("Mostrar números de daño")));
	sys_arpg_show_dmg_chk->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_arpg_show_dmg_chk->set_pressed(false);
	sys_arpg_show_dmg_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	rt_grid->add_child(sys_arpg_show_dmg_chk);

	Label *lbl_party_sec = memnew(Label(CG_DB_TR("Grupo inicial")));
	lbl_party_sec->set_tooltip_text(CG_DB_TR("Grupo inicial (Party - Máx. 1 Líder + 8 Seguidores)"));
	lbl_party_sec->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	system_panel->add_child(lbl_party_sec);
	system_panel->add_child(memnew(HSeparator));

	sys_party_list = memnew(ItemList);
	sys_party_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_party_list->set_custom_minimum_size(Size2(0, 120 * EDSCALE));
	system_panel->add_child(sys_party_list);

	HBoxContainer *party_ctrl_row = memnew(HBoxContainer);
	party_ctrl_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	system_panel->add_child(party_ctrl_row);

	sys_party_actor_opt = memnew(OptionButton);
	sys_party_actor_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sys_party_actor_opt->set_custom_minimum_size(Size2(220 * EDSCALE, row_h));
	party_ctrl_row->add_child(sys_party_actor_opt);

	btn_add_party_member = memnew(Button(CG_DB_TR("+ Añadir al Grupo")));
	btn_add_party_member->set_custom_minimum_size(Size2(0, row_h));
	btn_add_party_member->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_party_member_pressed));
	party_ctrl_row->add_child(btn_add_party_member);

	btn_remove_party_member = memnew(Button(CG_DB_TR("- Quitar")));
	btn_remove_party_member->set_custom_minimum_size(Size2(0, row_h));
	btn_remove_party_member->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_party_member_pressed));
	party_ctrl_row->add_child(btn_remove_party_member);

	// Los editores de Menús se construyen de forma perezosa la primera vez que se abre su sección.

	// Vista previa de colocación de tropas debajo de los miembros (todo el ancho disponible hasta 960px, proporción 16:9, sin hueco arriba)
	troop_placement_col3_box = memnew(VBoxContainer);
	troop_placement_col3_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_placement_col3_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	troop_placement_col3_box->hide();
	col2_box->add_child(troop_placement_col3_box);

	Label *lbl_tp_hdr = memnew(Label(CG_DB_TR("Vista previa (16:9)")));
	lbl_tp_hdr->set_tooltip_text(CG_DB_TR("Arrastra con el ratón cada enemigo para colocarlo en la pantalla de batalla (960x540)"));
	lbl_tp_hdr->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	troop_placement_col3_box->add_child(lbl_tp_hdr);
	troop_placement_col3_box->add_child(memnew(HSeparator));

	troop_placement_canvas = memnew(Control);
	troop_placement_canvas->set_custom_minimum_size(Size2(540 * EDSCALE, 304 * EDSCALE));
	troop_placement_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_placement_canvas->set_clip_contents(true);
	troop_placement_canvas->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	troop_placement_canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	troop_placement_canvas->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_placement_canvas_draw));
	troop_placement_canvas->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_placement_canvas_gui_input));
	troop_placement_col3_box->add_child(troop_placement_canvas);

	troop_bg_layer_ctrl = memnew(Control);
	troop_bg_layer_ctrl->set_anchors_preset(Control::PRESET_FULL_RECT);
	troop_bg_layer_ctrl->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	troop_bg_layer_ctrl->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_bg_layer_draw));
	troop_placement_canvas->add_child(troop_bg_layer_ctrl);

	troop_floor_layer_ctrl = memnew(Control);
	troop_floor_layer_ctrl->set_anchors_preset(Control::PRESET_FULL_RECT);
	troop_floor_layer_ctrl->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	troop_floor_layer_ctrl->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_floor_layer_draw));
	troop_placement_canvas->add_child(troop_floor_layer_ctrl);

	troop_enemies_overlay_ctrl = memnew(Control);
	troop_enemies_overlay_ctrl->set_anchors_preset(Control::PRESET_FULL_RECT);
	troop_enemies_overlay_ctrl->set_clip_contents(true);
	troop_enemies_overlay_ctrl->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	troop_enemies_overlay_ctrl->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	troop_enemies_overlay_ctrl->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_enemies_overlay_draw));
	troop_placement_canvas->add_child(troop_enemies_overlay_ctrl);

	troop_hud_warning_label = memnew(Label);
	troop_hud_warning_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_hud_warning_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	troop_hud_warning_label->add_theme_color_override("font_color", Color(1.0f, 0.78f, 0.30f));
	troop_hud_warning_label->hide();
	troop_placement_col3_box->add_child(troop_hud_warning_label);

	// Ventana "Cambiar fondo" (Fondo + Suelo por tropa)
	troop_bg_dialog = memnew(AcceptDialog);
	troop_bg_dialog->set_title(CG_DB_TR("Cambiar fondo de batalla"));
	VBoxContainer *tbg_vb = memnew(VBoxContainer);
	tbg_vb->add_theme_constant_override("separation", 8);
	troop_bg_dialog->add_child(tbg_vb);

	HBoxContainer *tbg_row1 = memnew(HBoxContainer);
	tbg_row1->add_theme_constant_override("separation", 6);
	tbg_vb->add_child(tbg_row1);
	Label *lbl_bg1 = memnew(Label(CG_DB_TR("Fondo:")));
	lbl_bg1->set_custom_minimum_size(Size2(60 * EDSCALE, 0));
	tbg_row1->add_child(lbl_bg1);
	troop_bg1_edit = memnew(LineEdit);
	troop_bg1_edit->set_editable(false);
	troop_bg1_edit->set_custom_minimum_size(Size2(240 * EDSCALE, 0));
	troop_bg1_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tbg_row1->add_child(troop_bg1_edit);
	Button *btn_bg1_browse = memnew(Button(CG_DB_TR("Seleccionar...")));
	btn_bg1_browse->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_bg_browse_pressed).bind(1));
	tbg_row1->add_child(btn_bg1_browse);
	Button *btn_bg1_clear = memnew(Button(CG_DB_TR("Quitar")));
	btn_bg1_clear->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_bg_clear_pressed).bind(1));
	tbg_row1->add_child(btn_bg1_clear);

	HBoxContainer *tbg_row2 = memnew(HBoxContainer);
	tbg_row2->add_theme_constant_override("separation", 6);
	tbg_vb->add_child(tbg_row2);
	Label *lbl_bg2 = memnew(Label(CG_DB_TR("Suelo:")));
	lbl_bg2->set_custom_minimum_size(Size2(60 * EDSCALE, 0));
	tbg_row2->add_child(lbl_bg2);
	troop_bg2_edit = memnew(LineEdit);
	troop_bg2_edit->set_editable(false);
	troop_bg2_edit->set_custom_minimum_size(Size2(240 * EDSCALE, 0));
	troop_bg2_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tbg_row2->add_child(troop_bg2_edit);
	Button *btn_bg2_browse = memnew(Button(CG_DB_TR("Seleccionar...")));
	btn_bg2_browse->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_bg_browse_pressed).bind(2));
	tbg_row2->add_child(btn_bg2_browse);
	Button *btn_bg2_clear = memnew(Button(CG_DB_TR("Quitar")));
	btn_bg2_clear->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_bg_clear_pressed).bind(2));
	tbg_row2->add_child(btn_bg2_clear);

	add_child(troop_bg_dialog);

	troop_bg_file_dialog = memnew(FileDialog);
	troop_bg_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	troop_bg_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	troop_bg_file_dialog->clear_filters();
	troop_bg_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes de Fondo (*.png, *.webp)"));
	troop_bg_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_bg_file_selected));
	troop_bg_dialog->add_child(troop_bg_file_dialog);

	_build_adjust_dialog(0, 1); // Ventana propia para Ajustar Fondo
	_build_adjust_dialog(1, 2); // Ventana propia para Ajustar Suelo

	// ========================================================================
	// PESTAÑA ANIMACIONES (Enemigos y Actores: UN SOLO selector desplegable + Tira de frames HFlow + Vista previa 128px)
	// ========================================================================
	_setup_arpg_equip_anim_ui();

	anim_sel_sec = memnew(VBoxContainer);
	anim_sel_sec->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_sel_sec->add_theme_constant_override("separation", int(8 * EDSCALE));
	sprite_anim_editor_box->add_child(anim_sel_sec);

	Label *lbl_b1 = memnew(Label(CG_DB_TR("Animación activa")));
	lbl_b1->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	anim_sel_sec->add_child(lbl_b1);
	anim_sel_sec->add_child(memnew(HSeparator));

	actor_dir_idle_chk = memnew(CheckBox(CG_DB_TR("Idle direccional (ranuras quieto por dirección)")));
	actor_dir_idle_chk->set_visible(false);
	actor_dir_idle_chk->set_pressed(false);
	actor_dir_idle_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_actor_directional_idle_toggled));
	anim_sel_sec->add_child(actor_dir_idle_chk);

	HBoxContainer *track_ctrl_hb = memnew(HBoxContainer);
	track_ctrl_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	track_ctrl_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	anim_sel_sec->add_child(track_ctrl_hb);

	anim_track_opt = memnew(OptionButton);
	anim_track_opt->set_meta("cg_anim_track_opt", true);
	anim_track_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_track_opt->set_custom_minimum_size(Size2(180 * EDSCALE, row_h));
	anim_track_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_track_selected));
	track_ctrl_hb->add_child(anim_track_opt);

	btn_add_anim = memnew(Button(CG_DB_TR("Nueva")));
	btn_add_anim->set_custom_minimum_size(Size2(72 * EDSCALE, row_h));
	btn_add_anim->set_tooltip_text(CG_DB_TR("Añadir una nueva animación vacía"));
	btn_add_anim->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_anim_pressed));
	track_ctrl_hb->add_child(btn_add_anim);

	btn_rename_anim = memnew(Button(CG_DB_TR("Renombrar")));
	btn_rename_anim->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	btn_rename_anim->set_tooltip_text(CG_DB_TR("Renombrar animación seleccionada"));
	btn_rename_anim->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_rename_anim_pressed));
	track_ctrl_hb->add_child(btn_rename_anim);

	btn_remove_anim = memnew(Button(CG_DB_TR("Quitar")));
	btn_remove_anim->set_custom_minimum_size(Size2(72 * EDSCALE, row_h));
	btn_remove_anim->set_tooltip_text(CG_DB_TR("Eliminar animación seleccionada"));
	btn_remove_anim->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_anim_pressed));
	track_ctrl_hb->add_child(btn_remove_anim);

	// Diálogo: Nueva Animación
	new_anim_dialog = memnew(ConfirmationDialog);
	new_anim_dialog->set_title(CG_DB_TR("Nueva Animación"));
	VBoxContainer *nad_vb = memnew(VBoxContainer);
	nad_vb->add_child(memnew(Label(CG_DB_TR("Nombre de la animación (ej: idle, attack, hurt, death, walk):"))));
	new_anim_name_edit = memnew(LineEdit);
	new_anim_name_edit->set_custom_minimum_size(Size2(220 * EDSCALE, 0));
	nad_vb->add_child(new_anim_name_edit);
	new_anim_dialog->add_child(nad_vb);
	new_anim_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_new_anim_dialog_confirmed));
	add_child(new_anim_dialog);

	// Diálogo: Renombrar Animación
	rename_anim_dialog = memnew(ConfirmationDialog);
	rename_anim_dialog->set_title(CG_DB_TR("Renombrar Animación"));
	VBoxContainer *rad_vb = memnew(VBoxContainer);
	rad_vb->add_child(memnew(Label(CG_DB_TR("Nuevo nombre para la animación:"))));
	rename_anim_name_edit = memnew(LineEdit);
	rename_anim_name_edit->set_custom_minimum_size(Size2(220 * EDSCALE, 0));
	rad_vb->add_child(rename_anim_name_edit);
	rename_anim_dialog->add_child(rad_vb);
	rename_anim_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_rename_anim_dialog_confirmed));
	add_child(rename_anim_dialog);

	// Debajo, sin cortarse: FPS, Bucle, Orden aleatorio y Probar/Detener
	HFlowContainer *settings_row = memnew(HFlowContainer);
	settings_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	settings_row->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	settings_row->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	anim_sel_sec->add_child(settings_row);

	HBoxContainer *fps_pair = memnew(HBoxContainer);
	fps_pair->add_theme_constant_override("separation", int(6 * EDSCALE));
	settings_row->add_child(fps_pair);

	Label *lbl_fps = memnew(Label(CG_DB_TR("FPS:")));
	fps_pair->add_child(lbl_fps);

	anim_fps_spin = memnew(SpinBox);
	anim_fps_spin->set_custom_minimum_size(Size2(76 * EDSCALE, row_h));
	anim_fps_spin->set_min(1);
	anim_fps_spin->set_max(60);
	anim_fps_spin->set_value(6);
	_bind_numeric_spinbox(anim_fps_spin);
	anim_fps_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_fps_value_changed));
	if (LineEdit *fps_le = anim_fps_spin->get_line_edit()) {
		fps_le->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_fps_text_changed));
		fps_le->connect("text_submitted", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_fps_submitted));
		fps_le->connect("focus_exited", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_fps_focus_exited));
	}
	fps_pair->add_child(anim_fps_spin);

	anim_loop_chk = memnew(CheckBox(CG_DB_TR("Bucle")));
	anim_loop_chk->set_custom_minimum_size(Size2(0, row_h));
	anim_loop_chk->set_pressed(true);
	anim_loop_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_playback_settings_changed).unbind(1));
	settings_row->add_child(anim_loop_chk);

	anim_behind_body_chk = memnew(CheckBox(CG_DB_TR("Dibujar detrás del cuerpo")));
	anim_behind_body_chk->set_custom_minimum_size(Size2(0, row_h));
	anim_behind_body_chk->set_pressed(false);
	anim_behind_body_chk->set_visible(false);
	anim_behind_body_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_playback_settings_changed).unbind(1));
	settings_row->add_child(anim_behind_body_chk);

	anim_random_order_chk = memnew(CheckBox(CG_DB_TR("Orden aleatorio")));
	anim_random_order_chk->set_custom_minimum_size(Size2(0, row_h));
	anim_random_order_chk->set_pressed(false);
	anim_random_order_chk->set_tooltip_text(CG_DB_TR("Elige el siguiente frame al azar en vez de seguir la tira"));
	anim_random_order_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_playback_settings_changed).unbind(1));
	settings_row->add_child(anim_random_order_chk);

	btn_anim_play = memnew(Button(CG_DB_TR("▶ Probar")));
	btn_anim_play->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	btn_anim_play->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_play_pressed));
	settings_row->add_child(btn_anim_play);

	// Submodos por ranura para ARPG y Equipo ARPG: "Frames" (intacto) vs "Formas" (Hitbox/Hurtbox/Ancla/Maniquí)
	anim_submode_bar = memnew(HFlowContainer);
	anim_submode_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_submode_bar->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	anim_submode_bar->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	anim_submode_bar->set_visible(false);
	anim_sel_sec->add_child(anim_submode_bar);

	Label *lbl_submode = memnew(Label(CG_DB_TR("Modo de ranura:")));
	anim_submode_bar->add_child(lbl_submode);

	btn_anim_mode_frames = memnew(Button(CG_DB_TR("Frames")));
	btn_anim_mode_frames->set_toggle_mode(true);
	btn_anim_mode_frames->set_pressed(true);
	btn_anim_mode_frames->set_theme_type_variation("ClockGearsTabButton");
	btn_anim_mode_frames->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	btn_anim_mode_frames->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_submode_selected).bind(0));
	anim_submode_bar->add_child(btn_anim_mode_frames);

	btn_anim_mode_shapes = memnew(Button(CG_DB_TR("Formas")));
	btn_anim_mode_shapes->set_toggle_mode(true);
	btn_anim_mode_shapes->set_pressed(false);
	btn_anim_mode_shapes->set_theme_type_variation("ClockGearsTabButton");
	btn_anim_mode_shapes->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	btn_anim_mode_shapes->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_submode_selected).bind(1));
	anim_submode_bar->add_child(btn_anim_mode_shapes);

	// Tira de frames (con salto de línea HFlowContainer, NUNCA scroll horizontal) + Vista previa 128px a un lado
	anim_frames_sec = memnew(VBoxContainer);
	anim_frames_sec->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_frames_sec->add_theme_constant_override("separation", int(8 * EDSCALE));
	sprite_anim_editor_box->add_child(anim_frames_sec);

	HBoxContainer *frames_bar = memnew(HBoxContainer);
	frames_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frames_bar->add_theme_constant_override("separation", int(12 * EDSCALE));
	anim_frames_sec->add_child(frames_bar);

	Label *lbl_b3 = memnew(Label(CG_DB_TR("Frames y vista previa")));
	lbl_b3->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_b3->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	frames_bar->add_child(lbl_b3);

	btn_open_sheet_importer = memnew(Button(CG_DB_TR("🎞 Añadir frames desde spritesheet...")));
	btn_open_sheet_importer->set_custom_minimum_size(Size2(240 * EDSCALE, row_h));
	btn_open_sheet_importer->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_open_sheet_importer_pressed));
	frames_bar->add_child(btn_open_sheet_importer);

	anim_frames_sec->add_child(memnew(HSeparator));

	HBoxContainer *frames_and_preview_hb = memnew(HBoxContainer);
	frames_and_preview_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frames_and_preview_hb->add_theme_constant_override("separation", int(16 * EDSCALE));
	anim_frames_sec->add_child(frames_and_preview_hb);

	frames_strip_scroll = nullptr;
	frames_strip_hbox = memnew(HFlowContainer);
	frames_strip_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frames_strip_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	frames_strip_hbox->set_custom_minimum_size(Size2(0, 170 * EDSCALE));
	frames_strip_hbox->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	frames_strip_hbox->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	frames_and_preview_hb->add_child(frames_strip_hbox);

	VBoxContainer *prev_col = memnew(VBoxContainer);
	prev_col->set_h_size_flags(Control::SIZE_FILL);
	prev_col->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	prev_col->add_theme_constant_override("separation", int(6 * EDSCALE));
	frames_and_preview_hb->add_child(prev_col);

	Label *lbl_prev = memnew(Label(CG_DB_TR("Vista previa")));
	lbl_prev->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	lbl_prev->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	prev_col->add_child(lbl_prev);

	PanelContainer *prev_panel = memnew(PanelContainer);
	prev_panel->set_custom_minimum_size(Size2(136 * EDSCALE, 136 * EDSCALE));
	Ref<StyleBoxFlat> prev_sb;
	prev_sb.instantiate();
	prev_sb->set_anti_aliased(true);
	prev_sb->set_bg_color(Color(0.12f, 0.14f, 0.18f, 0.95f));
	prev_sb->set_border_width_all(1);
	prev_sb->set_border_color(Color(0.3f, 0.35f, 0.45f, 0.8f));
	prev_sb->set_corner_radius_all(6);
	prev_sb->set_content_margin_all(int(4 * EDSCALE));
	prev_panel->add_theme_style_override("panel", prev_sb);
	prev_col->add_child(prev_panel);

	anim_preview_rect = memnew(TextureRect);
	anim_preview_rect->set_custom_minimum_size(Size2(128 * EDSCALE, 128 * EDSCALE));
	anim_preview_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	anim_preview_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	anim_preview_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	anim_preview_atlas.instantiate();
	anim_preview_rect->set_texture(anim_preview_atlas);
	prev_panel->add_child(anim_preview_rect);

	_setup_shape_editor_ui();

	// ==========================================
	// BLOQUE 5: ANIMACIONES DE COMBATE (Actores) / LANZAMIENTOS CON NOMBRE Y ELEMENTO (Enemigos)
	// ==========================================
	_setup_combat_anims_ui();

	enemy_casts_box = memnew(VBoxContainer);
	enemy_casts_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_casts_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	enemy_casts_box->hide();
	sprite_anim_editor_box->add_child(enemy_casts_box);

	HBoxContainer *ec_hdr = memnew(HBoxContainer);
	ec_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	Label *lbl_ec_hdr = memnew(Label(CG_DB_TR("Lanzamientos elementales")));
	lbl_ec_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_ec_hdr->set_tooltip_text(CG_DB_TR("Ranuras de animación para habilidades elementales del enemigo"));
	lbl_ec_hdr->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	ec_hdr->add_child(lbl_ec_hdr);

	btn_add_enemy_cast = memnew(Button(CG_DB_TR("+ Añadir Lanzamiento")));
	btn_add_enemy_cast->set_custom_minimum_size(Size2(0, row_h));
	btn_add_enemy_cast->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_enemy_cast_pressed));
	ec_hdr->add_child(btn_add_enemy_cast);
	enemy_casts_box->add_child(ec_hdr);
	enemy_casts_box->add_child(memnew(HSeparator));

	enemy_casts_rows_vbox = memnew(VBoxContainer);
	enemy_casts_rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_casts_rows_vbox->add_theme_constant_override("separation", int(6 * EDSCALE));
	enemy_casts_box->add_child(enemy_casts_rows_vbox);

	enemy_others_box = memnew(VBoxContainer);
	enemy_others_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_others_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	enemy_others_box->hide();
	sprite_anim_editor_box->add_child(enemy_others_box);

	Label *lbl_eo_hdr = memnew(Label(CG_DB_TR("Otras animaciones")));
	lbl_eo_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_eo_hdr->set_tooltip_text(CG_DB_TR("Animaciones migradas para reasignar a una ranura fija o borrar"));
	lbl_eo_hdr->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	enemy_others_box->add_child(lbl_eo_hdr);
	enemy_others_box->add_child(memnew(HSeparator));

	enemy_others_rows_vbox = memnew(VBoxContainer);
	enemy_others_rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_others_rows_vbox->add_theme_constant_override("separation", int(6 * EDSCALE));
	enemy_others_box->add_child(enemy_others_rows_vbox);

	// ==========================================
	// VENTANA: "AÑADIR FRAMES DESDE SPRITESHEET" (Modal estilo Godot)
	// ==========================================
	sheet_importer_dialog = memnew(ConfirmationDialog);
	sheet_importer_dialog->set_title(CG_DB_TR("Añadir frames desde spritesheet"));
	sheet_importer_dialog->set_min_size(Size2(680, 500) * EDSCALE);
	sheet_importer_dialog->set_ok_button_text(CG_DB_TR("Añadir frames"));

	VBoxContainer *imp_vb = memnew(VBoxContainer);
	imp_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	imp_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	imp_vb->add_theme_constant_override("separation", 6);
	sheet_importer_dialog->add_child(imp_vb);

	// Fila 1: Selector de PNG
	HBoxContainer *imp_file_hb = memnew(HBoxContainer);
	imp_file_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	imp_file_hb->add_theme_constant_override("separation", 4);
	imp_vb->add_child(imp_file_hb);

	Label *lbl_imp_png = memnew(Label(CG_DB_TR("Spritesheet PNG:")));
	imp_file_hb->add_child(lbl_imp_png);

	sheet_importer_path_edit = memnew(LineEdit);
	sheet_importer_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sheet_importer_path_edit->set_editable(false);
	imp_file_hb->add_child(sheet_importer_path_edit);

	sheet_importer_browse_btn = memnew(Button(CG_DB_TR("Explorar...")));
	sheet_importer_browse_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_browse_pressed));
	imp_file_hb->add_child(sheet_importer_browse_btn);

	// Fila 2: Frames horizontales y verticales + acciones de selección
	HBoxContainer *imp_grid_hb = memnew(HBoxContainer);
	imp_grid_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	imp_grid_hb->add_theme_constant_override("separation", 6);
	imp_vb->add_child(imp_grid_hb);

	Label *lbl_imp_cols = memnew(Label(CG_DB_TR("Frames horizontales:")));
	imp_grid_hb->add_child(lbl_imp_cols);

	sheet_importer_cols_spin = memnew(SpinBox);
	sheet_importer_cols_spin->set_min(1);
	sheet_importer_cols_spin->set_max(64);
	sheet_importer_cols_spin->set_value(4);
	_bind_numeric_spinbox(sheet_importer_cols_spin);
	sheet_importer_cols_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_grid_changed));
	imp_grid_hb->add_child(sheet_importer_cols_spin);

	Label *lbl_imp_rows = memnew(Label(CG_DB_TR("Frames verticales:")));
	imp_grid_hb->add_child(lbl_imp_rows);

	sheet_importer_rows_spin = memnew(SpinBox);
	sheet_importer_rows_spin->set_min(1);
	sheet_importer_rows_spin->set_max(64);
	sheet_importer_rows_spin->set_value(1);
	_bind_numeric_spinbox(sheet_importer_rows_spin);
	sheet_importer_rows_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_grid_changed));
	imp_grid_hb->add_child(sheet_importer_rows_spin);

	imp_grid_hb->add_child(memnew(VSeparator));

	sheet_importer_sel_all_btn = memnew(Button(CG_DB_TR("Seleccionar todos")));
	sheet_importer_sel_all_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_sel_all_pressed));
	imp_grid_hb->add_child(sheet_importer_sel_all_btn);

	sheet_importer_clear_sel_btn = memnew(Button(CG_DB_TR("Deseleccionar")));
	sheet_importer_clear_sel_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_clear_sel_pressed));
	imp_grid_hb->add_child(sheet_importer_clear_sel_btn);

	sheet_importer_info_lbl = memnew(Label(CG_DB_TR("Tamaño de frame: - | Imagen: -")));
	sheet_importer_info_lbl->add_theme_color_override("font_color", Color(0.75f, 0.8f, 0.9f));
	imp_vb->add_child(sheet_importer_info_lbl);

	// Rejilla interactiva con scroll
	sheet_importer_scroll = memnew(ScrollContainer);
	sheet_importer_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sheet_importer_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	sheet_importer_scroll->set_custom_minimum_size(Size2(0, 220 * EDSCALE));
	sheet_importer_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	sheet_importer_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	imp_vb->add_child(sheet_importer_scroll);

	sheet_importer_canvas = memnew(Control);
	sheet_importer_canvas->set_custom_minimum_size(Size2(128 * EDSCALE, 128 * EDSCALE));
	sheet_importer_canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	sheet_importer_canvas->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_canvas_draw));
	sheet_importer_canvas->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_canvas_gui_input));
	sheet_importer_scroll->add_child(sheet_importer_canvas);

	// Estado inferior del diálogo
	HBoxContainer *imp_status_hb = memnew(HBoxContainer);
	imp_status_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	imp_vb->add_child(imp_status_hb);

	sheet_importer_status_lbl = memnew(Label(CG_DB_TR("0 frame(s) seleccionado(s) (Arrastra o pulsa Ctrl+Clic para elegir frames)")));
	sheet_importer_status_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sheet_importer_status_lbl->add_theme_color_override("font_color", Color(0.7f, 0.7f, 0.7f));
	imp_status_hb->add_child(sheet_importer_status_lbl);

	sheet_importer_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_add_confirmed));
	add_child(sheet_importer_dialog);

	sheet_importer_file_dialog = memnew(FileDialog);
	sheet_importer_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	sheet_importer_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	sheet_importer_file_dialog->clear_filters();
	sheet_importer_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes Spritesheet (*.png, *.webp)"));
	sheet_importer_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_sheet_importer_file_selected));
	add_child(sheet_importer_file_dialog);

	// Sección: Editor de Rasgos (Traits) en el panel Editor
	traits_box = memnew(VBoxContainer);
	traits_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	traits_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	col2_box->add_child(traits_box);

	traits_box->add_child(memnew(HSeparator));
	traits_box->add_child(memnew(Label(CG_DB_TR("Rasgos"))));

	traits_tree = memnew(Tree);
	traits_tree->set_columns(3);
	traits_tree->set_column_titles_visible(true);
	traits_tree->set_column_title(0, CG_DB_TR("Tipo"));
	traits_tree->set_column_title(1, CG_DB_TR("Dato"));
	traits_tree->set_column_title(2, CG_DB_TR("Valor"));
	traits_tree->set_column_expand(0, true);
	traits_tree->set_column_expand_ratio(0, 2);
	traits_tree->set_column_expand(1, true);
	traits_tree->set_column_expand_ratio(1, 3);
	traits_tree->set_column_expand(2, true);
	traits_tree->set_column_expand_ratio(2, 2);
	traits_tree->set_hide_root(true);
	traits_tree->set_custom_minimum_size(Size2(0, 140 * EDSCALE));
	traits_box->add_child(traits_tree);

	HBoxContainer *tr_btn_row = memnew(HBoxContainer);
	tr_btn_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	traits_box->add_child(tr_btn_row);

	btn_add_trait = memnew(Button(CG_DB_TR("+ Rasgo")));
	btn_add_trait->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_add_trait->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	btn_add_trait->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_trait_pressed));
	tr_btn_row->add_child(btn_add_trait);

	btn_remove_trait = memnew(Button(CG_DB_TR("- Quitar")));
	btn_remove_trait->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_remove_trait->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	btn_remove_trait->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_trait_pressed));
	tr_btn_row->add_child(btn_remove_trait);

	trait_dialog = memnew(AcceptDialog);
	trait_dialog->set_title(CG_DB_TR("Añadir Rasgo"));
	GridContainer *tr_grid = memnew(GridContainer);
	tr_grid->set_columns(2);
	tr_grid->add_theme_constant_override("h_separation", 10);
	tr_grid->add_theme_constant_override("v_separation", 8);
	tr_grid->add_child(memnew(Label(CG_DB_TR("Tipo de Rasgo:"))));
	trait_type_opt = memnew(OptionButton);
	trait_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	trait_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_trait_type_changed));
	tr_grid->add_child(trait_type_opt);
	tr_grid->add_child(memnew(Label(CG_DB_TR("Objetivo / Subtipo:"))));
	trait_data_opt = memnew(OptionButton);
	trait_data_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tr_grid->add_child(trait_data_opt);
	tr_grid->add_child(memnew(Label(CG_DB_TR("Valor (% o Cantidad):"))));
	trait_val_spin = memnew(SpinBox);
	trait_val_spin->set_min(-1000);
	trait_val_spin->set_max(1000);
	trait_val_spin->set_step(1);
	trait_val_spin->set_value(100);
	_bind_numeric_spinbox(trait_val_spin);
	tr_grid->add_child(trait_val_spin);
	trait_dialog->add_child(tr_grid);
	trait_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_trait_confirmed));
	add_child(trait_dialog);

	// Sección: Editor de Efectos de uso (Effects) en el panel Editor
	effects_box = memnew(VBoxContainer);
	effects_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	effects_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	effects_box->hide();
	col2_box->add_child(effects_box);

	effects_box->add_child(memnew(HSeparator));
	effects_box->add_child(memnew(Label(CG_DB_TR("Efectos de uso"))));

	effects_tree = memnew(Tree);
	effects_tree->set_columns(3);
	effects_tree->set_column_titles_visible(true);
	effects_tree->set_column_title(0, CG_DB_TR("Efecto"));
	effects_tree->set_column_title(1, CG_DB_TR("Dato"));
	effects_tree->set_column_title(2, CG_DB_TR("Val 1 / Val 2"));
	effects_tree->set_column_expand(0, true);
	effects_tree->set_column_expand_ratio(0, 2);
	effects_tree->set_column_expand(1, true);
	effects_tree->set_column_expand_ratio(1, 3);
	effects_tree->set_column_expand(2, true);
	effects_tree->set_column_expand_ratio(2, 2);
	effects_tree->set_hide_root(true);
	effects_tree->set_custom_minimum_size(Size2(0, 140 * EDSCALE));
	effects_box->add_child(effects_tree);

	HBoxContainer *ef_btn_row = memnew(HBoxContainer);
	ef_btn_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	effects_box->add_child(ef_btn_row);

	btn_add_effect = memnew(Button(CG_DB_TR("+ Efecto")));
	btn_add_effect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_add_effect->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	btn_add_effect->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_effect_pressed));
	ef_btn_row->add_child(btn_add_effect);

	btn_remove_effect = memnew(Button(CG_DB_TR("- Quitar")));
	btn_remove_effect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_remove_effect->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
	btn_remove_effect->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_effect_pressed));
	ef_btn_row->add_child(btn_remove_effect);

	effect_dialog = memnew(AcceptDialog);
	effect_dialog->set_title(CG_DB_TR("Añadir Efecto"));
	GridContainer *ef_grid = memnew(GridContainer);
	ef_grid->set_columns(2);
	ef_grid->add_theme_constant_override("h_separation", 10);
	ef_grid->add_theme_constant_override("v_separation", 8);
	ef_grid->add_child(memnew(Label(CG_DB_TR("Tipo de Efecto:"))));
	effect_type_opt = memnew(OptionButton);
	effect_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	effect_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_effect_type_changed));
	ef_grid->add_child(effect_type_opt);
	ef_grid->add_child(memnew(Label(CG_DB_TR("Objetivo / Dato:"))));
	effect_data_opt = memnew(OptionButton);
	effect_data_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ef_grid->add_child(effect_data_opt);
	ef_grid->add_child(memnew(Label(CG_DB_TR("Valor 1 (% / Turnos):"))));
	effect_val1_spin = memnew(SpinBox);
	effect_val1_spin->set_min(-1000);
	effect_val1_spin->set_max(1000);
	effect_val1_spin->set_step(1);
	effect_val1_spin->set_value(0);
	_bind_numeric_spinbox(effect_val1_spin);
	ef_grid->add_child(effect_val1_spin);
	ef_grid->add_child(memnew(Label(CG_DB_TR("Valor 2 (Fijo / Extra):"))));
	effect_val2_spin = memnew(SpinBox);
	effect_val2_spin->set_min(-999999);
	effect_val2_spin->set_max(999999);
	effect_val2_spin->set_step(1);
	effect_val2_spin->set_value(50);
	_bind_numeric_spinbox(effect_val2_spin);
	ef_grid->add_child(effect_val2_spin);
	effect_dialog->add_child(ef_grid);
	effect_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_effect_confirmed));
	add_child(effect_dialog);

	// Tileset Collision Panel (shown on the right when CAT_TILESETS is active)
	tileset_collision_panel = memnew(PanelContainer);
	tileset_collision_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_collision_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_collision_panel->set_stretch_ratio(0.64f);
	tileset_collision_panel->set_custom_minimum_size(Size2(340 * EDSCALE, 0));
	Ref<StyleBoxFlat> ts_panel_sb = memnew(StyleBoxFlat);
	ts_panel_sb->set_anti_aliased(true);
	ts_panel_sb->set_bg_color(Color::html("#1a1d24"));
	ts_panel_sb->set_corner_radius_all(int(6 * EDSCALE));
	ts_panel_sb->set_border_width_all(1);
	ts_panel_sb->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.12f));
	ts_panel_sb->set_content_margin_individual(int(8 * EDSCALE), int(4 * EDSCALE), int(8 * EDSCALE), int(4 * EDSCALE));
	tileset_collision_panel->add_theme_style_override("panel", ts_panel_sb);
	tileset_collision_panel->hide();
	tileset_main_hbox->add_child(tileset_collision_panel);

	tileset_collision_box = memnew(VBoxContainer);
	tileset_collision_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_collision_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_collision_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	tileset_collision_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	tileset_collision_box->hide();
	tileset_collision_panel->add_child(tileset_collision_box);

	// a) Fila superior, b) Lienzo, c) Estado, d) Modos (ordenado dentro de db_tile_picker en modo panel)
	db_tile_picker = memnew(ClockGearsTilePicker);
	db_tile_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	db_tile_picker->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	db_tile_picker->set_custom_minimum_size(Size2(220 * EDSCALE, 200 * EDSCALE));
	db_tile_picker->set_metadata_editing_enabled(true);
	db_tile_picker->set_tileset_panel_mode(true, ClockGearsIcons::get_editor_icon_scale(this));
	db_tile_picker->set_edit_mode(ClockGearsTilePicker::MODE_PASSAGE_BASIC);
	db_tile_picker->set_on_metadata_changed(callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_collision_changed));
	db_tile_picker->set_on_request_load(callable_mp(this, &ClockGearsDatabaseDialog::_on_load_tileset_sheet_pressed));
	tileset_collision_box->add_child(db_tile_picker);

	// Estilo compacto redondeado (esquinas 6px, borde 1px) para botones del pie de Tilesets
	const Color ts_accent = ClockGearsTheme::color_accent_amber();
	const int ts_r6 = int(6 * EDSCALE);
	Ref<StyleBoxFlat> ts_tab_norm = memnew(StyleBoxFlat);
	ts_tab_norm->set_anti_aliased(true);
	ts_tab_norm->set_bg_color(Color::html("#2a2e38"));
	ts_tab_norm->set_corner_radius_all(ts_r6);
	ts_tab_norm->set_border_width_all(1);
	ts_tab_norm->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.14f));
	ts_tab_norm->set_content_margin_all(int(2 * EDSCALE));

	Ref<StyleBoxFlat> ts_tab_hov = ts_tab_norm->duplicate();
	ts_tab_hov->set_bg_color(Color::html("#353b48"));
	ts_tab_hov->set_border_color(Color(ts_accent.r, ts_accent.g, ts_accent.b, 0.65f));

	Ref<StyleBoxFlat> ts_tab_prs = ts_tab_norm->duplicate();
	ts_tab_prs->set_bg_color(Color(0.24f, 0.20f, 0.14f, 1.0f));
	ts_tab_prs->set_border_color(ts_accent);

	Ref<StyleBoxFlat> ts_tab_hprs = ts_tab_prs->duplicate();
	ts_tab_hprs->set_bg_color(Color(0.29f, 0.24f, 0.16f, 1.0f));

	auto apply_ts_btn_style = [&](Button *p_btn, int p_pad_lr) {
		Ref<StyleBoxFlat> bn = ts_tab_norm->duplicate();
		Ref<StyleBoxFlat> bh = ts_tab_hov->duplicate();
		Ref<StyleBoxFlat> bp = ts_tab_prs->duplicate();
		Ref<StyleBoxFlat> bhp = ts_tab_hprs->duplicate();
		bn->set_content_margin_individual(p_pad_lr, int(2 * EDSCALE), p_pad_lr, int(2 * EDSCALE));
		bh->set_content_margin_individual(p_pad_lr, int(2 * EDSCALE), p_pad_lr, int(2 * EDSCALE));
		bp->set_content_margin_individual(p_pad_lr, int(2 * EDSCALE), p_pad_lr, int(2 * EDSCALE));
		bhp->set_content_margin_individual(p_pad_lr, int(2 * EDSCALE), p_pad_lr, int(2 * EDSCALE));
		p_btn->add_theme_style_override("normal", bn);
		p_btn->add_theme_style_override("hover", bh);
		p_btn->add_theme_style_override("pressed", bp);
		p_btn->add_theme_style_override("hover_pressed", bhp);
	};

	// e) Pestañas A B C D E + subpestañas A1 A2 A3 A4 A5 en una sola fila alineada a la izquierda (28x28 px)
	tileset_tabs_row = memnew(HBoxContainer);
	tileset_tabs_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_tabs_row->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	tileset_tabs_row->add_theme_constant_override("separation", int(12 * EDSCALE));
	tileset_collision_box->add_child(tileset_tabs_row);

	tileset_group_tabs_box = memnew(HBoxContainer);
	tileset_group_tabs_box->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
	tileset_group_tabs_box->add_theme_constant_override("separation", int(4 * EDSCALE));
	tileset_tabs_row->add_child(tileset_group_tabs_box);

	const char *group_names[5] = { "A", "B", "C", "D", "E" };
	const char *group_tips[5] = {
		"Pestaña A: Hojas A1 (Agua), A2 (Suelo), A3 (Edificios), A4 (Muros), A5 (Normal)",
		"Pestaña B: Objetos y Decoración 1 (32x32)",
		"Pestaña C: Objetos y Decoración 2 (32x32)",
		"Pestaña D: Objetos y Decoración 3 (32x32)",
		"Pestaña E: Objetos y Decoración 4 (32x32)"
	};
	for (int g = 0; g < 5; g++) {
		btn_ts_group_tab[g] = memnew(Button(group_names[g]));
		btn_ts_group_tab[g]->set_toggle_mode(true);
		btn_ts_group_tab[g]->set_pressed(g == 0);
		btn_ts_group_tab[g]->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		btn_ts_group_tab[g]->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		btn_ts_group_tab[g]->set_custom_minimum_size(Size2(int(28 * EDSCALE), int(28 * EDSCALE)));
		btn_ts_group_tab[g]->set_text_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		btn_ts_group_tab[g]->set_tooltip_text(CG_DB_TR(group_tips[g]));
		apply_ts_btn_style(btn_ts_group_tab[g], int(2 * EDSCALE));
		btn_ts_group_tab[g]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_group_tab_pressed).bind(g));
		tileset_group_tabs_box->add_child(btn_ts_group_tab[g]);
	}

	// Sub-pestañas cuando A está activa: A1 | A2 | A3 | A4 | A5 (justo a la derecha de A..E)
	tileset_a_subtabs_box = memnew(HBoxContainer);
	tileset_a_subtabs_box->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
	tileset_a_subtabs_box->add_theme_constant_override("separation", int(4 * EDSCALE));
	tileset_tabs_row->add_child(tileset_a_subtabs_box);

	const char *sub_names[5] = { "A1", "A2", "A3", "A4", "A5" };
	const char *sub_tips[5] = {
		"Ranura A1: Agua / Tiles Animados (32x32)",
		"Ranura A2: Suelo / Autotiles (32x32)",
		"Ranura A3: Edificios / Fachadas (32x32)",
		"Ranura A4: Muros / Techos (32x32)",
		"Ranura A5: Tiles Normales (32x32)"
	};
	for (int a = 0; a < 5; a++) {
		btn_ts_a_subtab[a] = memnew(Button(sub_names[a]));
		btn_ts_a_subtab[a]->set_toggle_mode(true);
		btn_ts_a_subtab[a]->set_pressed(a == 4);
		btn_ts_a_subtab[a]->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		btn_ts_a_subtab[a]->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		btn_ts_a_subtab[a]->set_custom_minimum_size(Size2(int(28 * EDSCALE), int(28 * EDSCALE)));
		btn_ts_a_subtab[a]->set_text_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		btn_ts_a_subtab[a]->set_tooltip_text(CG_DB_TR(sub_tips[a]));
		apply_ts_btn_style(btn_ts_a_subtab[a], int(2 * EDSCALE));
		btn_ts_a_subtab[a]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_a_subtab_pressed).bind(a));
		tileset_a_subtabs_box->add_child(btn_ts_a_subtab[a]);
	}

	// f) Fila inferior de estado compacta ("Ranura activa: ...", botón "Cargar Hoja..." y "?")
	HBoxContainer *slot_status_row = memnew(HBoxContainer);
	slot_status_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slot_status_row->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	slot_status_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	tileset_collision_box->add_child(slot_status_row);

	active_slot_info_label = memnew(Label(CG_DB_TR("Ranura activa: A5")));
	active_slot_info_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	active_slot_info_label->set_clip_text(true);
	active_slot_info_label->add_theme_color_override("font_color", ts_accent);
	slot_status_row->add_child(active_slot_info_label);

	btn_load_tileset_sheet = memnew(Button(CG_DB_TR("Cargar Hoja...")));
	btn_load_tileset_sheet->set_tooltip_text(CG_DB_TR("Cargar archivo PNG 32x32 para la ranura activa"));
	btn_load_tileset_sheet->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	btn_load_tileset_sheet->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	btn_load_tileset_sheet->set_custom_minimum_size(Size2(0, int(28 * EDSCALE)));
	apply_ts_btn_style(btn_load_tileset_sheet, int(8 * EDSCALE));
	btn_load_tileset_sheet->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_load_tileset_sheet_pressed));
	slot_status_row->add_child(btn_load_tileset_sheet);

	Button *btn_picker_help = memnew(Button("?"));
	btn_picker_help->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	btn_picker_help->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	btn_picker_help->set_custom_minimum_size(Size2(int(28 * EDSCALE), int(28 * EDSCALE)));
	btn_picker_help->set_text_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	apply_ts_btn_style(btn_picker_help, int(2 * EDSCALE));
	btn_picker_help->set_tooltip_text(CG_DB_TR("Clic en cada tile para alternar pasabilidad:\n○ (Círculo) = Pasable (Suelo)\n× (Cruz) = Bloqueado (Colisión)\n★ (Estrella) = Sobre personaje (Techo/Pasa detrás)\n4-Dir = Pasabilidad por flechas (↓ ← → ↑)\nTerr = Etiqueta de Terreno (0 a 7)\nArb = Arbusto (~)\nMost = Mostrador (♦)\nEsc = Escalera (H)\nDaño = Daño de suelo (▲)"));
	slot_status_row->add_child(btn_picker_help);

	tileset_scroll = memnew(ScrollContainer);
	tileset_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	tileset_scroll->hide();
	tileset_collision_box->add_child(tileset_scroll);

	tileset_collision_editor = memnew(ClockGearsTilesetCollisionEditor);
	tileset_collision_editor->set_on_collision_changed(callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_collision_changed));
	tileset_scroll->add_child(tileset_collision_editor);

	// Change Max Modal Dialog
	change_max_dialog = memnew(AcceptDialog);
	change_max_dialog->set_title(CG_DB_TR("Cambiar Máximo de Registros"));
	VBoxContainer *cm_vb = memnew(VBoxContainer);
	cm_vb->add_child(memnew(Label(CG_DB_TR("Nueva cantidad máxima de elementos:"))));
	change_max_spin = memnew(SpinBox);
	change_max_spin->set_min(0);
	change_max_spin->set_max(999);
	change_max_spin->set_step(1);
	_bind_numeric_spinbox(change_max_spin);
	cm_vb->add_child(change_max_spin);
	change_max_dialog->add_child(cm_vb);
	change_max_dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_change_max_confirmed));
	add_child(change_max_dialog);

	// File Dialog for Sprites / Icons / Tilesets
	file_dialog = memnew(FileDialog);
	file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_file_selected));
	add_child(file_dialog);

	asset_warning_dialog = memnew(AcceptDialog);
	asset_warning_dialog->set_title(CG_DB_TR("Aviso Pixel Art (32x32)"));
	add_child(asset_warning_dialog);

	const float footer_btn_h = 32.0f * EDSCALE;
	add_theme_constant_override("buttons_min_width", int(112 * EDSCALE));
	add_theme_constant_override("buttons_min_height", int(footer_btn_h));
	set_hide_on_ok(false);
	connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_db_ok_pressed));
	if (get_ok_button()) {
		get_ok_button()->set_text(CG_DB_TR("OK"));
		get_ok_button()->set_custom_minimum_size(Size2(112 * EDSCALE, footer_btn_h));
	}
	btn_apply_changes = add_button(CG_DB_TR("Aplicar"), true);
	if (btn_apply_changes) {
		btn_apply_changes->set_custom_minimum_size(Size2(112 * EDSCALE, footer_btn_h));
		btn_apply_changes->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_db_apply_pressed));
	}
	_set_pending_changes(false);

	Dictionary init_ui_state = _load_editor_ui_state();
	int init_cat = int(init_ui_state.get("last_category", CAT_ACTORS));
	if (init_cat < 0 || init_cat > DB_CAT_UI_DESIGNER) {
		init_cat = CAT_ACTORS;
	}
	_on_category_selected(init_cat);
}

void ClockGearsDatabaseDialog::_refresh_icon_scale() {
	const int icon_scale = ClockGearsIcons::get_editor_icon_scale(this);
	last_icon_scale = icon_scale;
	const int icon_px = 32 * icon_scale;
	const float row_h = 32.0f * EDSCALE;
	const float cat_btn_h = MAX(row_h, float(icon_px + 16));

	for (int i = 0; i < category_buttons.size(); i++) {
		if (!category_buttons[i]) {
			continue;
		}
		category_buttons[i]->add_theme_constant_override("icon_max_width", icon_px);
		category_buttons[i]->set_custom_minimum_size(Size2(0, cat_btn_h));
		if (ClockGearsIcons::get_singleton()) {
			category_buttons[i]->set_button_icon(ClockGearsIcons::get_singleton()->get_db_category_icon(i, float(icon_scale)));
		}
	}
	if (col1_box) {
		col1_box->set_custom_minimum_size(Size2(MAX(256.0f * EDSCALE, float(icon_px + 192)), 0));
	}
	if (record_list) {
		record_list->set_fixed_icon_size(Size2i(icon_px, icon_px));
		record_list->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	}
	add_theme_constant_override("buttons_min_width", int(112 * EDSCALE));
	add_theme_constant_override("buttons_min_height", int(row_h));
	if (get_ok_button()) {
		get_ok_button()->set_custom_minimum_size(Size2(112 * EDSCALE, row_h));
	}
	if (btn_apply_changes) {
		btn_apply_changes->set_custom_minimum_size(Size2(112 * EDSCALE, row_h));
	}
	_clear_record_list_icon_cache();
	_refresh_record_list();
	if (db_tile_picker) {
		db_tile_picker->set_tileset_panel_mode(true, icon_scale);
	}
	_update_tileset_split_layout();
}

void ClockGearsDatabaseDialog::_on_editor_settings_changed() {
	if (ClockGearsIcons::get_editor_icon_scale(this) != last_icon_scale) {
		_refresh_icon_scale();
	}
}

void ClockGearsDatabaseDialog::open_database() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	if (ClockGearsIcons::get_editor_icon_scale(this) != last_icon_scale) {
		_refresh_icon_scale();
	}
	_load_database_from_disk();
	Dictionary ui_state = _load_editor_ui_state();
	int last_cat = int(ui_state.get("last_category", current_category));
	if (last_cat >= 0 && last_cat <= DB_CAT_UI_DESIGNER) {
		current_category = last_cat;
	}
	_on_category_selected(current_category);

	Size2i screen_sz(1920, 1080);
	if (DisplayServer::get_singleton()) {
		int scr = DisplayServer::get_singleton()->window_get_current_screen();
		Size2i ds_sz = DisplayServer::get_singleton()->screen_get_usable_rect(scr).size;
		if (ds_sz.x <= 0 || ds_sz.y <= 0) {
			ds_sz = DisplayServer::get_singleton()->screen_get_size(scr);
		}
		if (ds_sz.x > 0 && ds_sz.y > 0) {
			screen_sz = ds_sz;
		}
	} else if (Window *parent_win = get_parent_visible_window()) {
		Size2i pw = parent_win->get_size();
		if (pw.x > 0 && pw.y > 0) {
			screen_sz = pw;
		}
	}

	Size2i min_sz(int(1000 * EDSCALE), int(640 * EDSCALE));
	if (screen_sz.x < min_sz.x) {
		min_sz.x = screen_sz.x;
	}
	if (screen_sz.y < min_sz.y) {
		min_sz.y = screen_sz.y;
	}
	set_min_size(min_sz);

	Size2i max_def(int(1600 * EDSCALE), int(900 * EDSCALE));
	Size2i default_sz(
			MIN(int(screen_sz.x * 0.90f), max_def.x),
			MIN(int(screen_sz.y * 0.90f), max_def.y));
	default_sz = default_sz.max(min_sz).min(screen_sz);

	Size2i remembered_window_size(int(ui_state.get("window_w", 0)), int(ui_state.get("window_h", 0)));
	bool has_remembered_window_pos = bool(ui_state.get("has_pos", false));
	Point2i remembered_window_pos(int(ui_state.get("window_x", 0)), int(ui_state.get("window_y", 0)));

	Size2i target_size = default_sz;
	if (remembered_window_size.x > 0 && remembered_window_size.y > 0) {
		target_size = remembered_window_size.max(min_sz).min(screen_sz);
	}

	popup_centered(target_size);
	if (has_remembered_window_pos && DisplayServer::get_singleton()) {
		int scr = DisplayServer::get_singleton()->window_get_current_screen();
		Rect2i usable = DisplayServer::get_singleton()->screen_get_usable_rect(scr);
		if (usable.size.x > 0 && usable.size.y > 0) {
			Point2i clamped_pos(
					CLAMP(remembered_window_pos.x, usable.position.x, MAX(usable.position.x, usable.position.x + usable.size.x - target_size.x)),
					CLAMP(remembered_window_pos.y, usable.position.y, MAX(usable.position.y, usable.position.y + usable.size.y - target_size.y)));
			set_position(clamped_pos);
		}
	}

	if (name_edit && name_edit->is_visible_in_tree()) {
		name_edit->grab_focus();
	}
}

void ClockGearsDatabaseDialog::_save_database_to_disk() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	bool had_hud_rt = hud_preview_has_saved_rt;
	_restore_hud_preview_runtime_state();
	for (int i = 0; i < CAT_MAX; i++) {
		dm->set_category_array(i, category_data[i]);
	}
	dm->set_system_config(system_config);
	dm->set_title_menu_config(ClockGearsDataManager::normalize_title_menu_config(menu_title_config));
	dm->set_window_styles_config(ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft));
	dm->set_game_menu_config(ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft));
	dm->set_credits_config(ClockGearsDataManager::normalize_credits_config(credits_config_draft));
	dm->set_game_over_config(ClockGearsDataManager::normalize_game_over_config(game_over_config_draft));
	dm->set_hud_config(ClockGearsDataManager::normalize_hud_config(hud_config_draft));
	dm->save_all("res://clockgears_data");
	if (had_hud_rt && current_category == DB_CAT_HUD && is_visible()) {
		_update_hud_live_preview();
	}
}

void ClockGearsDatabaseDialog::_load_database_from_disk() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	_restore_hud_preview_runtime_state();
	dm->load_all("res://clockgears_data");
	for (int i = 0; i < CAT_MAX; i++) {
		category_data[i] = dm->get_category_array(i);
	}
	system_config = dm->get_system_config();
	menu_title_config = ClockGearsDataManager::normalize_title_menu_config(dm->get_title_menu_config());
	window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(dm->get_window_styles_config());
	game_menu_config_draft = ClockGearsDataManager::normalize_game_menu_config(dm->get_game_menu_config());
	credits_config_draft = ClockGearsDataManager::normalize_credits_config(dm->get_credits_config());
	game_over_config_draft = ClockGearsDataManager::normalize_game_over_config(dm->get_game_over_config());
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(dm->get_hud_config());
	current_combat_mode = (int)system_config.get("combat_mode", COMBAT_MODE_TURN_BASED);
	_clear_troop_texture_cache();
	_clear_record_list_icon_cache();
	if (ui_design_editor_panel) {
		ui_design_editor_panel->refresh_from_data_manager();
	}
	_set_pending_changes(false);
}

// ============================================================================
// Métodos de Tropas (Fase 8A / Cierre)
// ============================================================================

void ClockGearsDatabaseDialog::_refresh_troop_members_ui() {
	if (!troop_members_list_vbox) return;
	while (troop_members_list_vbox->get_child_count() > 0) {
		Node *c = troop_members_list_vbox->get_child(0);
		troop_members_list_vbox->remove_child(c);
		memdelete(c);
	}
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) {
		_queue_troop_preview_redraw();
		return;
	}
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) {
		_queue_troop_preview_redraw();
		return;
	}

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	if (selected_troop_member_idx >= mems.size()) {
		selected_troop_member_idx = MAX(0, mems.size() - 1);
	}
	if (btn_add_troop_member) {
		btn_add_troop_member->set_disabled(mems.size() >= 8);
	}
	if (btn_remove_troop_member_sel) {
		btn_remove_troop_member_sel->set_disabled(mems.is_empty());
	}
	if (btn_clear_troop_members) {
		btn_clear_troop_members->set_disabled(mems.is_empty());
	}
	if (btn_align_troop_members) {
		btn_align_troop_members->set_disabled(mems.is_empty());
	}

	for (int i = 0; i < mems.size(); i++) {
		Dictionary m = mems[i];
		int enemy_id = int(m.get("enemy_id", 1));
		int mx = int(m.get("x", 200 + i * 110));
		int my = int(m.get("y", 260));

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
		row->add_theme_constant_override("separation", int(8 * EDSCALE));

		Button *btn_sel = memnew(Button(vformat("#%d", i + 1)));
		btn_sel->set_toggle_mode(true);
		btn_sel->set_pressed(i == selected_troop_member_idx);
		btn_sel->set_custom_minimum_size(Size2(36 * EDSCALE, 32 * EDSCALE));
		btn_sel->connect("pressed", callable_mp_static(+[](ClockGearsDatabaseDialog *p_self, int p_m_idx) {
			if (p_self) {
				p_self->selected_troop_member_idx = p_m_idx;
				p_self->_refresh_troop_members_ui();
			}
		}).bind(this, i));
		row->add_child(btn_sel);

		OptionButton *opt = memnew(OptionButton);
		opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		opt->set_custom_minimum_size(Size2(0, 32 * EDSCALE));
		_populate_db_records_option(opt, CAT_ENEMIES, CG_DB_TR("(ninguno)"), false);
		_select_option_by_id_or_index(opt, enemy_id);
		opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_member_enemy_changed).bind(i));
		row->add_child(opt);

		Label *lbl_x = memnew(Label("X:"));
		row->add_child(lbl_x);

		SpinBox *sp_x = memnew(SpinBox);
		sp_x->set_min(0);
		sp_x->set_max(960);
		sp_x->set_value(mx);
		sp_x->set_custom_minimum_size(Size2(80 * EDSCALE, 32 * EDSCALE));
		_bind_numeric_spinbox(sp_x);
		sp_x->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_member_x_changed).bind(i));
		row->add_child(sp_x);

		Label *lbl_y = memnew(Label("Y:"));
		row->add_child(lbl_y);

		SpinBox *sp_y = memnew(SpinBox);
		sp_y->set_min(0);
		sp_y->set_max(540);
		sp_y->set_value(my);
		sp_y->set_custom_minimum_size(Size2(80 * EDSCALE, 32 * EDSCALE));
		_bind_numeric_spinbox(sp_y);
		sp_y->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_member_y_changed).bind(i));
		row->add_child(sp_y);

		Button *btn_del = memnew(Button("-"));
		btn_del->set_custom_minimum_size(Size2(56 * EDSCALE, 32 * EDSCALE));
		btn_del->set_tooltip_text(CG_DB_TR("Eliminar este enemigo de la tropa"));
		btn_del->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_troop_member_pressed).bind(i));
		row->add_child(btn_del);

		troop_members_list_vbox->add_child(row);
	}
	_update_troop_hud_warning(d);
	_queue_troop_preview_redraw();
}

void ClockGearsDatabaseDialog::_on_add_troop_member_pressed() {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	if (mems.size() >= 8) {
		return; // Máximo 8 miembros por tropa
	}

	int enemy_id = 1;
	const Array &enemies = category_data[CAT_ENEMIES];
	if (!enemies.is_empty()) {
		Dictionary e0 = enemies[0];
		enemy_id = int(e0.get("id", 1));
	}

	Dictionary new_m;
	new_m["enemy_id"] = enemy_id;
	new_m["x"] = CLAMP(160 + mems.size() * 96, 64, 896);
	new_m["y"] = 260;
	new_m["escala"] = 1;
	new_m["hp_bar_mode"] = "abajo";
	mems.push_back(new_m);
	selected_troop_member_idx = mems.size() - 1;

	d["members"] = mems;
	arr[idx] = d;
	_refresh_troop_members_ui();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_remove_troop_member_pressed(int p_idx) {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	if (p_idx >= 0 && p_idx < mems.size()) {
		mems.remove_at(p_idx);
		if (selected_troop_member_idx >= mems.size()) {
			selected_troop_member_idx = MAX(0, mems.size() - 1);
		}
		d["members"] = mems;
		arr[idx] = d;
		_refresh_troop_members_ui();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_remove_selected_troop_member_pressed() {
	_on_remove_troop_member_pressed(selected_troop_member_idx);
}

void ClockGearsDatabaseDialog::_on_clear_troop_members_pressed() {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	d["members"] = Array();
	arr[idx] = d;
	selected_troop_member_idx = 0;
	_refresh_troop_members_ui();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_align_troop_members_pressed() {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	int n = mems.size();
	if (n == 0) return;

	const Rect2 ref_vp(Point2(0, 0), Size2(ClockGearsBattleScene::BATTLE_REF_WIDTH, ClockGearsBattleScene::BATTLE_REF_HEIGHT));
	const Array &enemies = category_data[CAT_ENEMIES];
	Ref<Font> font = get_theme_default_font();

	Rect2 group_box;
	bool has_box = false;
	Vector<Size2> src_sizes;
	Vector<int> member_scales;
	src_sizes.resize(n);
	member_scales.resize(n);

	for (int i = 0; i < n; i++) {
		Dictionary m = mems[i];
		Vector2 mpos(float(m.get("x", 200 + i * 110)), float(m.get("y", 260)));
		Size2 src_sz = s_resolve_troop_member_src_size(m, enemies);
		int m_scale = s_get_troop_member_scale(m);
		src_sizes.write[i] = src_sz;
		member_scales.write[i] = m_scale;

		ClockGearsBattleScene::EnemyVisualLayout layout = ClockGearsBattleScene::compute_enemy_visual_layout(mpos, ref_vp, src_sz, m_scale, font, s_get_troop_member_hp_bar_mode(m), s_get_troop_member_hp_bar_pos(m));
		if (!has_box) {
			group_box = layout.full_box;
			has_box = true;
		} else {
			group_box = group_box.merge(layout.full_box);
		}
	}

	Point2 align_center(
			ClockGearsBattleScene::BATTLE_ALIGN_POINT_FX * ClockGearsBattleScene::BATTLE_REF_WIDTH,
			ClockGearsBattleScene::BATTLE_ALIGN_POINT_FY * ClockGearsBattleScene::BATTLE_REF_HEIGHT);
	Vector2 delta = align_center - group_box.get_center();

	bool snap = chk_troop_snap_grid && chk_troop_snap_grid->is_pressed();
	for (int i = 0; i < n; i++) {
		Dictionary m = mems[i];
		float fx = float(m.get("x", 200 + i * 110)) + delta.x;
		float fy = float(m.get("y", 260)) + delta.y;
		int x = int(Math::round(fx));
		int y = int(Math::round(fy));
		// Con un solo enemigo NO lo movemos del punto de referencia exacto aunque esté activada la rejilla
		if (snap && n > 1) {
			Vector2i snapped = s_snap_troop_member_pos_to_grid(fx, fy, src_sizes[i], member_scales[i]);
			x = snapped.x;
			y = snapped.y;
		}
		m["x"] = CLAMP(x, 0, int(ClockGearsBattleScene::BATTLE_REF_WIDTH));
		m["y"] = CLAMP(y, 0, int(ClockGearsBattleScene::BATTLE_REF_HEIGHT));
		mems[i] = m;
	}
	d["members"] = mems;
	arr[idx] = d;
	_refresh_troop_members_ui();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_autoname_troop_pressed() {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	const Array &enemies = category_data[CAT_ENEMIES];

	Vector<String> order_names;
	HashMap<String, int> counts;
	for (int i = 0; i < mems.size(); i++) {
		Dictionary m = mems[i];
		int eid = int(m.get("enemy_id", 1));
		String ename = vformat("Enemigo %d", eid);
		for (int k = 0; k < enemies.size(); k++) {
			Dictionary ed = enemies[k];
			if (int(ed.get("id", k + 1)) == eid) {
				String raw = String(ed.get("name", "")).strip_edges();
				if (!raw.is_empty()) {
					ename = raw;
				}
				break;
			}
		}
		if (!counts.has(ename)) {
			counts[ename] = 1;
			order_names.push_back(ename);
		} else {
			counts[ename] += 1;
		}
	}

	String auto_name;
	for (int i = 0; i < order_names.size(); i++) {
		if (i > 0) auto_name += ", ";
		String nm = order_names[i];
		int c = counts[nm];
		if (c > 1) {
			auto_name += vformat("%s*%d", nm, c);
		} else {
			auto_name += nm;
		}
	}
	if (auto_name.is_empty()) {
		auto_name = vformat("Tropa %03d", idx + 1);
	}
	if (name_edit) {
		name_edit->set_text(auto_name);
	}
	d["name"] = auto_name;
	arr[idx] = d;
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_battle_test_troop_pressed() {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	_save_database_to_disk();
	Dictionary d = arr[idx];
	int troop_id = int(d.get("id", idx + 1));
	_start_battle_test(troop_id);
}

void ClockGearsDatabaseDialog::_start_battle_test(int p_troop_id) {
	battle_test_troop_id = p_troop_id;
	battle_test_last_result = 0;

	if (!battle_test_dialog) {
		battle_test_dialog = memnew(AcceptDialog);
		battle_test_dialog->set_title(CG_DB_TR("Prueba de Batalla - ClockGears Engine"));
		battle_test_dialog->set_min_size(Size2(ClockGearsBattleScene::BATTLE_REF_WIDTH, ClockGearsBattleScene::BATTLE_REF_HEIGHT) * EDSCALE);
		if (battle_test_dialog->get_ok_button()) {
			battle_test_dialog->get_ok_button()->hide();
		}
		battle_test_dialog->connect("canceled", callable_mp(this, &ClockGearsDatabaseDialog::_on_battle_test_close_pressed));
		battle_test_dialog->connect("window_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_battle_test_window_input));
		add_child(battle_test_dialog);

		CanvasLayer *summary_layer = memnew(CanvasLayer);
		summary_layer->set_layer(70);
		battle_test_dialog->add_child(summary_layer);

		battle_test_root = memnew(Control);
		battle_test_root->set_anchors_preset(Control::PRESET_FULL_RECT);
		battle_test_root->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		battle_test_root->hide();
		summary_layer->add_child(battle_test_root);

		ColorRect *dim = memnew(ColorRect);
		dim->set_anchors_preset(Control::PRESET_FULL_RECT);
		dim->set_color(Color(0.03f, 0.04f, 0.08f, 0.72f));
		battle_test_root->add_child(dim);

		CenterContainer *cc = memnew(CenterContainer);
		cc->set_anchors_preset(Control::PRESET_FULL_RECT);
		battle_test_root->add_child(cc);

		battle_test_summary_panel = memnew(PanelContainer);
		battle_test_summary_panel->set_custom_minimum_size(Size2(440, 260) * EDSCALE);
		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_anti_aliased(true);
		sb->set_bg_color(Color(0.11f, 0.13f, 0.19f, 0.98f));
		sb->set_border_width_all(1);
		sb->set_border_color(Color(0.85f, 0.72f, 0.35f, 0.95f));
		sb->set_corner_radius_all(6);
		sb->set_content_margin_all(int(12 * EDSCALE));
		battle_test_summary_panel->add_theme_style_override("panel", sb);
		cc->add_child(battle_test_summary_panel);

		VBoxContainer *svb = memnew(VBoxContainer);
		svb->add_theme_constant_override("separation", int(12 * EDSCALE));
		battle_test_summary_panel->add_child(svb);

		battle_test_summary_title = memnew(Label);
		battle_test_summary_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		battle_test_summary_title->add_theme_font_size_override("font_size", int(20 * EDSCALE));
		svb->add_child(battle_test_summary_title);

		svb->add_child(memnew(HSeparator));

		battle_test_summary_body = memnew(Label);
		battle_test_summary_body->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		battle_test_summary_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		svb->add_child(battle_test_summary_body);

		Label *lbl_hint = memnew(Label(CG_DB_TR("Pulsa una tecla o Continuar para cerrar · Esc para salir")));
		lbl_hint->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl_hint->add_theme_color_override("font_color", Color(0.70f, 0.75f, 0.85f));
		lbl_hint->add_theme_font_size_override("font_size", int(11 * EDSCALE));
		svb->add_child(lbl_hint);

		HBoxContainer *shb = memnew(HBoxContainer);
		shb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		shb->add_theme_constant_override("separation", int(12 * EDSCALE));
		svb->add_child(shb);

		btn_battle_test_retry = memnew(Button(CG_DB_TR("Reintentar")));
		btn_battle_test_retry->set_custom_minimum_size(Size2(130 * EDSCALE, 34 * EDSCALE));
		btn_battle_test_retry->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_battle_test_retry_pressed));
		shb->add_child(btn_battle_test_retry);

		btn_battle_test_continue = memnew(Button(CG_DB_TR("Continuar")));
		btn_battle_test_continue->set_custom_minimum_size(Size2(130 * EDSCALE, 34 * EDSCALE));
		btn_battle_test_continue->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_battle_test_close_pressed));
		shb->add_child(btn_battle_test_continue);
	}

	if (battle_test_scene) {
		if (battle_test_scene->get_parent()) {
			battle_test_scene->get_parent()->remove_child(battle_test_scene);
		}
		memdelete(battle_test_scene);
		battle_test_scene = nullptr;
	}
	if (battle_test_root) {
		battle_test_root->hide();
	}

	battle_test_scene = memnew(ClockGearsBattleScene);
	battle_test_scene->connect("battle_finished", callable_mp(this, &ClockGearsDatabaseDialog::_on_battle_test_finished));
	battle_test_dialog->add_child(battle_test_scene);
	battle_test_dialog->popup_centered(Size2(980, 590) * EDSCALE);
	// Modo prueba: copias descartables, nunca modifica oro, inventario ni EXP del proyecto
	battle_test_scene->setup_battle(p_troop_id, true, true, "", nullptr, true);
}

void ClockGearsDatabaseDialog::_on_battle_test_finished(int p_result) {
	battle_test_last_result = p_result;
	if (!battle_test_root || !battle_test_summary_title || !battle_test_summary_body) {
		_on_battle_test_close_pressed();
		return;
	}
	Ref<ClockGearsBattleController> ctrl = battle_test_scene ? battle_test_scene->get_controller() : Ref<ClockGearsBattleController>();
	String currency = String(system_config.get("currency", "G"));
	if (currency.is_empty()) currency = "G";

	if (p_result == ClockGearsBattleController::RESULT_VICTORY) {
		battle_test_summary_title->set_text(CG_DB_TR("¡Victoria!"));
		battle_test_summary_title->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.35f));

		int exp_gained = ctrl.is_valid() ? ctrl->get_total_exp() : 0;
		int gold_gained = ctrl.is_valid() ? ctrl->get_total_gold() : 0;
		Array drs = ctrl.is_valid() ? ctrl->get_dropped_items() : Array();
		Array lvs = ctrl.is_valid() ? ctrl->get_level_up_messages() : Array();

		String drops_str = CG_DB_TR("Ninguno");
		if (!drs.is_empty()) {
			drops_str = "";
			for (int i = 0; i < drs.size(); i++) {
				drops_str += (i > 0 ? ", " : "") + String(drs[i]);
			}
		}
		String lvs_str = CG_DB_TR("Ninguna");
		if (!lvs.is_empty()) {
			lvs_str = "";
			for (int i = 0; i < lvs.size(); i++) {
				lvs_str += (i > 0 ? "\n" : "") + String(lvs[i]);
			}
		}
		battle_test_summary_body->set_text(vformat(
				CG_DB_TR("EXP obtenida: %d\nOro obtenido: %d %s\nObjetos: %s\nSubidas de nivel:\n%s"),
				exp_gained, gold_gained, currency, drops_str, lvs_str));
		if (btn_battle_test_continue) {
			btn_battle_test_continue->set_text(CG_DB_TR("Continuar"));
		}
	} else if (p_result == ClockGearsBattleController::RESULT_DEFEAT || p_result == ClockGearsBattleController::RESULT_GAME_OVER) {
		battle_test_summary_title->set_text(CG_DB_TR("Derrota"));
		battle_test_summary_title->add_theme_color_override("font_color", Color(0.95f, 0.35f, 0.35f));
		battle_test_summary_body->set_text(CG_DB_TR("El grupo ha sido derrotado.\nPuedes reintentar el combate o salir."));
		if (btn_battle_test_continue) {
			btn_battle_test_continue->set_text(CG_DB_TR("Salir"));
		}
	} else {
		battle_test_summary_title->set_text(CG_DB_TR("Batalla finalizada"));
		battle_test_summary_title->add_theme_color_override("font_color", Color(0.80f, 0.88f, 1.0f));
		battle_test_summary_body->set_text(CG_DB_TR("El grupo ha escapado del combate."));
		if (btn_battle_test_continue) {
			btn_battle_test_continue->set_text(CG_DB_TR("Continuar"));
		}
	}

	battle_test_root->show();
	if (btn_battle_test_continue) {
		btn_battle_test_continue->grab_focus();
	}
}

void ClockGearsDatabaseDialog::_on_battle_test_retry_pressed() {
	if (battle_test_troop_id > 0) {
		_start_battle_test(battle_test_troop_id);
	}
}

void ClockGearsDatabaseDialog::_on_battle_test_close_pressed() {
	if (battle_test_root) {
		battle_test_root->hide();
	}
	if (battle_test_dialog) {
		battle_test_dialog->hide();
	}
	if (battle_test_scene) {
		if (battle_test_scene->get_parent()) {
			battle_test_scene->get_parent()->remove_child(battle_test_scene);
		}
		memdelete(battle_test_scene);
		battle_test_scene = nullptr;
	}
}

void ClockGearsDatabaseDialog::_on_battle_test_window_input(const Ref<InputEvent> &p_ev) {
	Ref<InputEventKey> k = p_ev;
	if (!k.is_valid() || !k->is_pressed() || k->is_echo()) {
		return;
	}
	Key kc = k->get_keycode();
	if (kc == Key::ESCAPE) {
		_on_battle_test_close_pressed();
		if (battle_test_dialog) {
			battle_test_dialog->set_input_as_handled();
		}
		return;
	}
	if (battle_test_root && battle_test_root->is_visible()) {
		if (battle_test_last_result == ClockGearsBattleController::RESULT_VICTORY || battle_test_last_result == ClockGearsBattleController::RESULT_ESCAPED) {
			if (kc != Key::SHIFT && kc != Key::CTRL && kc != Key::ALT && kc != Key::META) {
				_on_battle_test_close_pressed();
				if (battle_test_dialog) {
					battle_test_dialog->set_input_as_handled();
				}
			}
		}
	}
}

void ClockGearsDatabaseDialog::_on_change_troop_bg_pressed() {
	if (!troop_bg_dialog) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	if (troop_bg1_edit) {
		troop_bg1_edit->set_text(String(d.get("battleback1", d.get("background", ""))));
	}
	if (troop_bg2_edit) {
		troop_bg2_edit->set_text(String(d.get("battleback2", d.get("floor", ""))));
	}
	troop_bg_dialog->popup_centered(Size2(460, 150) * EDSCALE);
}

void ClockGearsDatabaseDialog::_on_troop_bg_browse_pressed(int p_slot) {
	if (!troop_bg_file_dialog) return;
	troop_bg_selecting_slot = p_slot;
	String dir = ClockGearsProjectStructure::resolve_project_folder_global("img/battlebacks");
	if (dir.is_empty()) {
		dir = ClockGearsProjectStructure::resolve_project_folder_global("img");
	}
	if (!dir.is_empty()) {
		troop_bg_file_dialog->set_current_dir(dir);
	}
	troop_bg_file_dialog->set_title(p_slot == 1 ? CG_DB_TR("Seleccionar imagen de Fondo (battleback1)") : CG_DB_TR("Seleccionar imagen de Suelo (battleback2)"));
	troop_bg_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
}

static Rect2 s_calc_troop_vp_rect(Control *p_canvas) {
	if (!p_canvas) return Rect2();
	Size2 avail = p_canvas->get_size();
	if (avail.x <= 10.0f || avail.y <= 10.0f) return Rect2();
	const float logical_w = ClockGearsBattleScene::BATTLE_REF_WIDTH;
	const float logical_h = ClockGearsBattleScene::BATTLE_REF_HEIGHT;
	float max_w = MIN(avail.x, logical_w * EDSCALE);
	float scale = MIN(max_w / logical_w, avail.y / logical_h);
	Size2 vp_size(logical_w * scale, logical_h * scale);
	Point2 vp_pos((avail.x - vp_size.x) * 0.5f, 0.0f);
	return Rect2(vp_pos, vp_size);
}

static Vector<Point2> s_get_adjust_handle_points(const Rect2 &p_rect, const Rect2 &p_vp) {
	Vector<Point2> pts;
	float x0 = CLAMP(p_rect.position.x, p_vp.position.x + 6.0f, p_vp.position.x + p_vp.size.x - 6.0f);
	float y0 = CLAMP(p_rect.position.y, p_vp.position.y + 6.0f, p_vp.position.y + p_vp.size.y - 6.0f);
	float x1 = CLAMP(p_rect.position.x + p_rect.size.x, p_vp.position.x + 6.0f, p_vp.position.x + p_vp.size.x - 6.0f);
	float y1 = CLAMP(p_rect.position.y + p_rect.size.y, p_vp.position.y + 6.0f, p_vp.position.y + p_vp.size.y - 6.0f);
	pts.push_back(Point2(x0, y0)); // TL
	pts.push_back(Point2(x1, y0)); // TR
	pts.push_back(Point2(x0, y1)); // BL
	pts.push_back(Point2(x1, y1)); // BR
	return pts;
}

void ClockGearsDatabaseDialog::_queue_troop_preview_redraw() {
	if (troop_placement_canvas) troop_placement_canvas->queue_redraw();
	if (troop_bg_layer_ctrl) troop_bg_layer_ctrl->queue_redraw();
	if (troop_floor_layer_ctrl) troop_floor_layer_ctrl->queue_redraw();
	if (troop_enemies_overlay_ctrl) troop_enemies_overlay_ctrl->queue_redraw();
}

void ClockGearsDatabaseDialog::_clear_troop_texture_cache() const {
	troop_preview_tex_cache.clear();
}

Ref<Texture2D> ClockGearsDatabaseDialog::_get_troop_placeholder_texture() const {
	if (!troop_placeholder_tex_cache.is_valid()) {
		troop_placeholder_tex_cache = ClockGearsBattleScene::create_placeholder_marker_texture();
	}
	return troop_placeholder_tex_cache;
}

Ref<Texture2D> ClockGearsDatabaseDialog::_retain_troop_texture(const String &p_path) const {
	String clean = p_path.strip_edges();
	if (clean.is_empty()) {
		return Ref<Texture2D>();
	}
	if (troop_preview_tex_cache.has(clean)) {
		Ref<Texture2D> cached = troop_preview_tex_cache[clean];
		if (ClockGearsBattleScene::is_texture_ready(cached)) {
			return cached;
		}
		troop_preview_tex_cache.erase(clean);
	}
	Ref<Texture2D> tex = _load_texture_from_path(clean);
	if (!ClockGearsBattleScene::is_texture_ready(tex)) {
		tex = ClockGearsBattleScene::load_texture_with_fallbacks(clean);
	}
	if (ClockGearsBattleScene::is_texture_ready(tex)) {
		troop_preview_tex_cache[clean] = tex;
		return tex;
	}
	troop_waiting_tex_ready = true;
	return Ref<Texture2D>();
}

void ClockGearsDatabaseDialog::_update_troop_hud_warning(const Dictionary &p_troop_dict) {
	if (!troop_hud_warning_label) return;
	const Rect2 ref_vp(Point2(0, 0), Size2(ClockGearsBattleScene::BATTLE_REF_WIDTH, ClockGearsBattleScene::BATTLE_REF_HEIGHT));
	Rect2 hud_strip = ClockGearsBattleScene::get_battle_hud_strip_rect(ref_vp);
	Array mems = p_troop_dict.get("members", Array());
	const Array &enemies = category_data[CAT_ENEMIES];
	Ref<Font> font = get_theme_default_font();

	bool any_overlap = false;
	for (int i = 0; i < mems.size(); i++) {
		Dictionary m = mems[i];
		Vector2 mpos(float(m.get("x", 200 + i * 110)), float(m.get("y", 260)));
		Size2 src_sz = s_resolve_troop_member_src_size(m, enemies);
		int m_scale = s_get_troop_member_scale(m);
		ClockGearsBattleScene::EnemyVisualLayout layout = ClockGearsBattleScene::compute_enemy_visual_layout(mpos, ref_vp, src_sz, m_scale, font, s_get_troop_member_hp_bar_mode(m), s_get_troop_member_hp_bar_pos(m));
		if (layout.full_box.intersects(hud_strip)) {
			any_overlap = true;
			break;
		}
	}
	if (any_overlap) {
		troop_hud_warning_label->set_text(CG_DB_TR("Aviso: uno o más enemigos entran en la franja oscura inferior del HUD y quedarán parcial o totalmente tapados en combate."));
		troop_hud_warning_label->show();
	} else {
		troop_hud_warning_label->hide();
	}
}

void ClockGearsDatabaseDialog::_on_editor_filesystem_changed() {
	_clear_troop_texture_cache();
	_clear_record_list_icon_cache();
	_refresh_record_list();
	if (current_category >= 0 && current_category < CAT_MAX && record_list) {
		Vector<int> sel = record_list->get_selected_items();
		if (!sel.is_empty()) {
			int idx = sel[0];
			const Array &arr = category_data[current_category];
			if (idx >= 0 && idx < arr.size()) {
				_update_graphic_preview_ui(arr[idx]);
			}
		}
	}
	_queue_troop_preview_redraw();
	for (int i = 0; i < 2; i++) {
		if (troop_adjust_ui[i].dialog && troop_adjust_ui[i].dialog->is_visible()) {
			_redraw_adjust_dialog(i);
		}
	}
}

void ClockGearsDatabaseDialog::_on_editor_resources_reimported(const PackedStringArray &p_resources) {
	_on_editor_filesystem_changed();
}

void ClockGearsDatabaseDialog::_on_troop_bg_clear_pressed(int p_slot) {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	if (p_slot == 1) {
		d["battleback1"] = "";
		if (troop_bg1_edit) troop_bg1_edit->set_text("");
	} else {
		d["battleback2"] = "";
		if (troop_bg2_edit) troop_bg2_edit->set_text("");
	}
	arr[idx] = d;
	_clear_troop_texture_cache();
	_queue_troop_preview_redraw();
	callable_mp(this, &ClockGearsDatabaseDialog::_queue_troop_preview_redraw).call_deferred();
	_save_database_to_disk();
}

void ClockGearsDatabaseDialog::_on_troop_bg_file_selected(const String &p_path) {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	String final_path = p_path;
	String bb_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/battlebacks");
	if (!bb_dir.is_empty()) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			da->make_dir_recursive(bb_dir);
			String dest_global = bb_dir.path_join(p_path.get_file());
			if (dest_global != p_path && FileAccess::exists(p_path)) {
				da->copy(p_path, dest_global);
			}
			ClockGearsProjectStructure::write_pixel_art_import_preset(dest_global);
			final_path = String("res://img/battlebacks/").path_join(p_path.get_file());
		}
	}

	Dictionary d = arr[idx];
	if (troop_bg_selecting_slot == 1) {
		d["battleback1"] = final_path;
		if (troop_bg1_edit) troop_bg1_edit->set_text(final_path);
	} else {
		d["battleback2"] = final_path;
		if (troop_bg2_edit) troop_bg2_edit->set_text(final_path);
	}
	arr[idx] = d;
	_save_database_to_disk();

#ifdef TOOLS_ENABLED
	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem *efs = EditorFileSystem::get_singleton();
		efs->update_file(final_path);
		if (!efs->is_scanning()) {
			efs->scan_changes();
		}
	}
#endif

	_clear_troop_texture_cache();
	_queue_troop_preview_redraw();
	callable_mp(this, &ClockGearsDatabaseDialog::_queue_troop_preview_redraw).call_deferred();
}

void ClockGearsDatabaseDialog::_on_adjust_troop_bg_pressed() {
	if (!troop_bg_adjust_menu || !btn_adjust_troop_bg) return;
	Vector2 pos = btn_adjust_troop_bg->get_screen_position() + Vector2(0, btn_adjust_troop_bg->get_size().y);
	troop_bg_adjust_menu->set_position(Vector2i(pos));
	troop_bg_adjust_menu->reset_size();
	troop_bg_adjust_menu->popup();
}

void ClockGearsDatabaseDialog::_on_adjust_troop_bg_menu_id(int p_id) {
	if (p_id == 1 || p_id == 2) {
		_open_adjust_dialog(p_id);
	}
}

void ClockGearsDatabaseDialog::_on_troop_scale_menu_id_pressed(int p_id) {
	int new_scale = CLAMP(p_id, 1, 6);
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int rec_idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (rec_idx < 0 || rec_idx >= arr.size()) return;

	Dictionary d = arr[rec_idx];
	Array mems = d.get("members", Array());
	int target_idx = (troop_ctx_member_idx >= 0 && troop_ctx_member_idx < mems.size()) ? troop_ctx_member_idx : selected_troop_member_idx;
	if (target_idx < 0 || target_idx >= mems.size()) return;

	Dictionary m = mems[target_idx];
	m["escala"] = new_scale;
	mems[target_idx] = m;
	d["members"] = mems;
	arr[rec_idx] = d;

	if (troop_scale_submenu) {
		for (int s = 1; s <= 6; s++) {
			int item_idx = troop_scale_submenu->get_item_index(s);
			if (item_idx >= 0) {
				troop_scale_submenu->set_item_checked(item_idx, s == new_scale);
			}
		}
	}
	if (troop_ctx_menu && troop_ctx_menu->is_visible()) {
		troop_ctx_menu->hide();
	}

	_update_troop_hud_warning(d);
	_queue_troop_preview_redraw();
	_on_param_changed();
}

static void s_update_hp_bar_submenu_checks(PopupMenu *p_sub, const String &p_mode) {
	if (!p_sub) return;
	int active_id = s_hp_bar_mode_to_menu_id(ClockGearsBattleUnit::normalize_hp_bar_mode(p_mode));
	for (int id = 1; id <= 4; id++) {
		int item_idx = p_sub->get_item_index(id);
		if (item_idx >= 0) {
			p_sub->set_item_checked(item_idx, id == active_id);
		}
	}
}

void ClockGearsDatabaseDialog::_on_troop_hp_bar_menu_id_pressed(int p_id) {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int rec_idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (rec_idx < 0 || rec_idx >= arr.size()) return;

	Dictionary d = arr[rec_idx];
	Array mems = d.get("members", Array());
	int target_idx = (troop_ctx_member_idx >= 0 && troop_ctx_member_idx < mems.size()) ? troop_ctx_member_idx : selected_troop_member_idx;
	if (target_idx < 0 || target_idx >= mems.size()) return;

	Dictionary m = mems[target_idx];
	String prev_mode = s_get_troop_member_hp_bar_mode(m);
	String next_mode = prev_mode;
	if (p_id == 5) {
		next_mode = "abajo";
		m.erase("hp_bar_x");
		m.erase("hp_bar_y");
		m.erase("hp_bar_offset_x");
		m.erase("hp_bar_offset_y");
		troop_hp_bar_dragging = false;
	} else if (p_id == 3) {
		next_mode = "libre";
		if (prev_mode != "libre" || !m.has("hp_bar_x") || !m.has("hp_bar_y")) {
			const Rect2 ref_vp(Point2(0, 0), Size2(ClockGearsBattleScene::BATTLE_REF_WIDTH, ClockGearsBattleScene::BATTLE_REF_HEIGHT));
			int mx = int(m.get("x", 200));
			int my = int(m.get("y", 260));
			Size2 src_sz = s_resolve_troop_member_src_size(m, category_data[CAT_ENEMIES]);
			int m_scale = s_get_troop_member_scale(m);
			Ref<Font> font = get_theme_default_font();
			ClockGearsBattleScene::EnemyVisualLayout cur_lay = ClockGearsBattleScene::compute_enemy_visual_layout(
					Vector2(float(mx), float(my)), ref_vp, src_sz, m_scale, font, prev_mode, s_get_troop_member_hp_bar_pos(m));
			Vector2i init_pos = ClockGearsBattleUnit::clamp_free_hp_bar_pos(Vector2i(
					int(Math::round(cur_lay.group_center_x)),
					int(Math::round(cur_lay.label_baseline_y - float(cur_lay.font_sz) - 3.0f))));
			m["hp_bar_x"] = init_pos.x;
			m["hp_bar_y"] = init_pos.y;
		}
		m.erase("hp_bar_offset_x");
		m.erase("hp_bar_offset_y");
	} else if (p_id == 1) {
		next_mode = "arriba";
	} else if (p_id == 4) {
		next_mode = "oculta";
	} else {
		next_mode = "abajo";
	}
	m["hp_bar_mode"] = next_mode;
	mems[target_idx] = m;
	d["members"] = mems;
	arr[rec_idx] = d;

	s_update_hp_bar_submenu_checks(troop_hp_bar_submenu, next_mode);
	if (troop_ctx_menu && troop_ctx_menu->is_visible()) {
		troop_ctx_menu->hide();
	}

	_update_troop_hud_warning(d);
	_queue_troop_preview_redraw();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_build_adjust_dialog(int p_ui_idx, int p_slot) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	ui.slot = p_slot;

	ui.dialog = memnew(AcceptDialog);
	ui.dialog->set_title(p_slot == 1 ? CG_DB_TR("Ajustar Fondo (Fondo)") : CG_DB_TR("Ajustar Suelo (Suelo)"));
	ui.dialog->get_ok_button()->set_text(CG_DB_TR("Aceptar"));
	ui.dialog->add_cancel_button(CG_DB_TR("Cancelar"));
	ui.dialog->connect("confirmed", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_confirmed).bind(p_ui_idx));
	ui.dialog->connect("window_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_window_input).bind(p_ui_idx));
	add_child(ui.dialog);

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", int(8 * EDSCALE));
	ui.dialog->add_child(vb);

	// Barra de opciones sin coordenadas numéricas a mano
	HFlowContainer *tools = memnew(HFlowContainer);
	tools->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	tools->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	vb->add_child(tools);

	Ref<ButtonGroup> mode_grp;
	mode_grp.instantiate();
	static const char *mode_names[4] = { "Cubrir", "Ajustar", "Estirar", "Mosaico" };
	for (int m = 0; m < 4; m++) {
		Button *mb = memnew(Button(CG_DB_TR(mode_names[m])));
		mb->set_toggle_mode(true);
		mb->set_button_group(mode_grp);
		mb->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_mode_selected).bind(m, p_ui_idx));
		tools->add_child(mb);
		ui.mode_btns[m] = mb;
	}

	ui.chk_integer_scale = memnew(CheckBox(CG_DB_TR("Escala entera (pixel art)")));
	ui.chk_integer_scale->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_integer_toggled).bind(p_ui_idx));
	tools->add_child(ui.chk_integer_scale);

	ui.btn_center = memnew(Button(CG_DB_TR("Centrar")));
	ui.btn_center->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_center_pressed).bind(p_ui_idx));
	tools->add_child(ui.btn_center);

	ui.btn_reset = memnew(Button(CG_DB_TR("Restablecer")));
	ui.btn_reset->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_reset_pressed).bind(p_ui_idx));
	tools->add_child(ui.btn_reset);

	// Marco 16:9 interactivo (640x360 escalado)
	PanelContainer *frame_pc = memnew(PanelContainer);
	frame_pc->set_custom_minimum_size(Size2(640, 360) * EDSCALE);
	frame_pc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frame_pc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vb->add_child(frame_pc);

	ui.canvas = memnew(Control);
	ui.canvas->set_custom_minimum_size(Size2(640, 360) * EDSCALE);
	ui.canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	ui.canvas->set_focus_mode(Control::FOCUS_ALL);
	ui.canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	ui.canvas->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_layer_draw).bind(p_ui_idx, 0));
	ui.canvas->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_canvas_gui_input).bind(p_ui_idx));
	frame_pc->add_child(ui.canvas);

	ui.bg_layer = memnew(Control);
	ui.bg_layer->set_anchors_preset(Control::PRESET_FULL_RECT);
	ui.bg_layer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	ui.bg_layer->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_layer_draw).bind(p_ui_idx, 1));
	ui.canvas->add_child(ui.bg_layer);

	ui.floor_layer = memnew(Control);
	ui.floor_layer->set_anchors_preset(Control::PRESET_FULL_RECT);
	ui.floor_layer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	ui.floor_layer->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_layer_draw).bind(p_ui_idx, 2));
	ui.canvas->add_child(ui.floor_layer);

	ui.overlay = memnew(Control);
	ui.overlay->set_anchors_preset(Control::PRESET_FULL_RECT);
	ui.overlay->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	ui.overlay->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	ui.overlay->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_adjust_overlay_draw).bind(p_ui_idx));
	ui.canvas->add_child(ui.overlay);

	ui.no_img_label = memnew(Label(CG_DB_TR("No hay imagen asignada en esta capa. Usa 'Cambiar fondo' para elegir una.")));
	ui.no_img_label->set_anchors_preset(Control::PRESET_CENTER);
	ui.no_img_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	ui.no_img_label->add_theme_color_override("font_color", Color(0.85f, 0.88f, 0.95f, 0.85f));
	ui.no_img_label->hide();
	ui.canvas->add_child(ui.no_img_label);

	ui.info_label = memnew(Label(CG_DB_TR("Arrastra con el ratón para mover · Rueda o esquinas para escalar · Flechas 1 px (Shift 8 px)")));
	ui.info_label->add_theme_color_override("font_color", Color(0.75f, 0.80f, 0.90f));
	ui.info_label->add_theme_font_size_override("font_size", int(11 * EDSCALE));
	vb->add_child(ui.info_label);
}

void ClockGearsDatabaseDialog::_open_adjust_dialog(int p_slot) {
	int ui_idx = (p_slot == 1) ? 0 : 1;
	AdjustDialogUI &ui = troop_adjust_ui[ui_idx];
	if (!ui.dialog) return;

	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	String key_cfg = (p_slot == 1) ? "battleback1_cfg" : "battleback2_cfg";
	String path = (p_slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
	Ref<Texture2D> tex = path.is_empty() ? Ref<Texture2D>() : _retain_troop_texture(path);
	Size2 tsz = ClockGearsBattleScene::is_texture_ready(tex) ? tex->get_size() : Size2();
	Variant raw_cfg = d.get(key_cfg, Dictionary());
	Dictionary cfg_dict = (raw_cfg.get_type() == Variant::DICTIONARY) ? Dictionary(raw_cfg) : Dictionary();

	ui.troop_idx = idx;
	ui.drag_mode = 0;
	ui.edit_cfg = ClockGearsBattleScene::normalize_battleback_cfg(cfg_dict, p_slot != 1, tsz);
	if (ui.no_img_label) {
		ui.no_img_label->set_visible(!ClockGearsBattleScene::is_texture_ready(tex));
	}
	_sync_adjust_dialog_controls(ui_idx);
	_redraw_adjust_dialog(ui_idx);
	ui.dialog->popup_centered(Size2(700, 480) * EDSCALE);
	if (ui.canvas) {
		ui.canvas->grab_focus();
	}
}

void ClockGearsDatabaseDialog::_sync_adjust_dialog_controls(int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	int cfg_mode = CLAMP(int(ui.edit_cfg.get("mode", 0)), 0, 3);
	updating_ui = true;
	for (int m = 0; m < 4; m++) {
		if (ui.mode_btns[m]) {
			ui.mode_btns[m]->set_pressed(m == cfg_mode);
		}
	}
	if (ui.chk_integer_scale) {
		ui.chk_integer_scale->set_pressed(bool(ui.edit_cfg.get("integer_scale", false)));
	}
	updating_ui = false;
}

void ClockGearsDatabaseDialog::_redraw_adjust_dialog(int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	if (ui.canvas) ui.canvas->queue_redraw();
	if (ui.bg_layer) ui.bg_layer->queue_redraw();
	if (ui.floor_layer) ui.floor_layer->queue_redraw();
	if (ui.overlay) ui.overlay->queue_redraw();
}

void ClockGearsDatabaseDialog::_on_adjust_mode_selected(int p_mode, int p_ui_idx) {
	if (updating_ui) return;
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	ui.edit_cfg["mode"] = CLAMP(p_mode, 0, 3);
	_redraw_adjust_dialog(p_ui_idx);
}

void ClockGearsDatabaseDialog::_on_adjust_integer_toggled(bool p_pressed, int p_ui_idx) {
	if (updating_ui) return;
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	bool was_int = bool(ui.edit_cfg.get("integer_scale", false));
	ui.edit_cfg["integer_scale"] = p_pressed;
	if (p_pressed && !was_int && ui.troop_idx >= 0 && ui.troop_idx < category_data[CAT_TROOPS].size()) {
		Dictionary d = category_data[CAT_TROOPS][ui.troop_idx];
		String path = (ui.slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
		Ref<Texture2D> tex = path.is_empty() ? Ref<Texture2D>() : _retain_troop_texture(path);
		if (ClockGearsBattleScene::is_texture_ready(tex) && tex->get_width() > 0) {
			Dictionary prev_cfg = ui.edit_cfg.duplicate();
			prev_cfg["integer_scale"] = false;
			Rect2 cur_r = ClockGearsBattleScene::compute_battleback_screen_rect(Rect2(0, 0, ClockGearsBattleScene::BATTLE_REF_WIDTH, ClockGearsBattleScene::BATTLE_REF_HEIGHT), tex->get_size(), prev_cfg, ui.slot != 1);
			double int_f = MAX(1.0, Math::round(double(cur_r.size.x) / double(tex->get_width())));
			ui.edit_cfg["scale"] = int_f;
		} else {
			ui.edit_cfg["scale"] = MAX(1.0, Math::round(double(ui.edit_cfg.get("scale", 1.0))));
		}
	} else if (p_pressed) {
		ui.edit_cfg["scale"] = MAX(1.0, Math::round(double(ui.edit_cfg.get("scale", 1.0))));
	}
	_redraw_adjust_dialog(p_ui_idx);
}

void ClockGearsDatabaseDialog::_on_adjust_center_pressed(int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	ui.edit_cfg["offset_x"] = 0.0;
	ui.edit_cfg["offset_y"] = 0.0;
	_redraw_adjust_dialog(p_ui_idx);
}

void ClockGearsDatabaseDialog::_on_adjust_reset_pressed(int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	Size2 tsz;
	if (ui.troop_idx >= 0 && ui.troop_idx < category_data[CAT_TROOPS].size()) {
		Dictionary d = category_data[CAT_TROOPS][ui.troop_idx];
		String path = (ui.slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
		Ref<Texture2D> tex = path.is_empty() ? Ref<Texture2D>() : _retain_troop_texture(path);
		if (ClockGearsBattleScene::is_texture_ready(tex)) {
			tsz = tex->get_size();
		}
	}
	ui.edit_cfg = ClockGearsBattleScene::default_battleback_cfg(ui.slot != 1, tsz);
	_sync_adjust_dialog_controls(p_ui_idx);
	_redraw_adjust_dialog(p_ui_idx);
}

void ClockGearsDatabaseDialog::_on_adjust_confirmed(int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	int idx = ui.troop_idx;
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;
	Dictionary d = arr[idx];
	String key_cfg = (ui.slot == 1) ? "battleback1_cfg" : "battleback2_cfg";
	d[key_cfg] = ui.edit_cfg.duplicate(true);
	arr[idx] = d;
	_save_database_to_disk();
	_queue_troop_preview_redraw();
}

void ClockGearsDatabaseDialog::_on_adjust_layer_draw(int p_ui_idx, int p_layer_slot) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	Control *ctrl = (p_layer_slot == 0) ? ui.canvas : ((p_layer_slot == 1) ? ui.bg_layer : ui.floor_layer);
	if (!ctrl) return;
	Rect2 vp_rect = s_calc_troop_vp_rect(ctrl);
	if (vp_rect.size.x <= 1.0f || vp_rect.size.y <= 1.0f) {
		vp_rect = Rect2(Point2(0, 0), ctrl->get_size());
	}

	if (p_layer_slot == 0) {
		ctrl->draw_rect(vp_rect, Color(0.08f, 0.09f, 0.14f, 1.0f), true);
		return;
	}

	if (ui.troop_idx < 0 || ui.troop_idx >= category_data[CAT_TROOPS].size()) return;
	Dictionary d = category_data[CAT_TROOPS][ui.troop_idx];
	String path = (p_layer_slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
	if (path.is_empty()) return;
	Ref<Texture2D> tex = _retain_troop_texture(path);
	if (!ClockGearsBattleScene::is_texture_ready(tex)) return;

	Dictionary cfg;
	if (p_layer_slot == ui.slot) {
		cfg = ui.edit_cfg;
	} else {
		String key_other = (p_layer_slot == 1) ? "battleback1_cfg" : "battleback2_cfg";
		Variant rv = d.get(key_other, Dictionary());
		cfg = (rv.get_type() == Variant::DICTIONARY) ? Dictionary(rv) : Dictionary();
	}
	ClockGearsBattleScene::draw_battleback_layer(ctrl, vp_rect, tex, cfg, p_layer_slot != 1);
}

void ClockGearsDatabaseDialog::_on_adjust_overlay_draw(int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	if (!ui.overlay) return;
	Rect2 vp_rect = s_calc_troop_vp_rect(ui.overlay);
	if (vp_rect.size.x <= 1.0f || vp_rect.size.y <= 1.0f) {
		vp_rect = Rect2(Point2(0, 0), ui.overlay->get_size());
	}
	if (ui.troop_idx >= 0 && ui.troop_idx < category_data[CAT_TROOPS].size()) {
		Dictionary d = category_data[CAT_TROOPS][ui.troop_idx];
		_draw_troop_enemies_preview(ui.overlay, vp_rect, d, false, false);

		String path = (ui.slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
		Ref<Texture2D> tex = path.is_empty() ? Ref<Texture2D>() : _retain_troop_texture(path);
		if (ClockGearsBattleScene::is_texture_ready(tex)) {
			Rect2 active_r = ClockGearsBattleScene::compute_battleback_screen_rect(vp_rect, tex->get_size(), ui.edit_cfg, ui.slot != 1);
			ui.overlay->draw_rect(active_r.intersection(vp_rect), Color(0.35f, 0.85f, 1.0f, 0.75f), false, 1.5f);
			Vector<Point2> corners = s_get_adjust_handle_points(active_r, vp_rect);
			for (int i = 0; i < corners.size(); i++) {
				Rect2 hrect(corners[i] - Point2(5, 5), Size2(10, 10));
				ui.overlay->draw_rect(hrect, Color(0.95f, 0.85f, 0.25f, 0.98f), true);
				ui.overlay->draw_rect(hrect, Color(0.10f, 0.12f, 0.16f, 1.0f), false, 1.5f);
			}
		}
	}
	ui.overlay->draw_rect(vp_rect, Color(0.35f, 0.40f, 0.50f, 0.9f), false, 1.0f);
}

void ClockGearsDatabaseDialog::_on_adjust_canvas_gui_input(const Ref<InputEvent> &p_ev, int p_ui_idx) {
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	if (!ui.canvas) return;
	Rect2 vp_rect = s_calc_troop_vp_rect(ui.canvas);
	if (vp_rect.size.x <= 1.0f) return;
	float vp_scale = vp_rect.size.x / ClockGearsBattleScene::BATTLE_REF_WIDTH;
	if (vp_scale <= 0.001f) return;

	Ref<InputEventMouseButton> mb = p_ev;
	Ref<InputEventMouseMotion> mm = p_ev;
	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::WHEEL_UP && mb->is_pressed()) {
			bool is_int = bool(ui.edit_cfg.get("integer_scale", false));
			double s = double(ui.edit_cfg.get("scale", 1.0));
			s = is_int ? MIN(16.0, Math::round(s) + 1.0) : MIN(10.0, s * 1.1);
			ui.edit_cfg["scale"] = s;
			_redraw_adjust_dialog(p_ui_idx);
			ui.canvas->accept_event();
			return;
		}
		if (mb->get_button_index() == MouseButton::WHEEL_DOWN && mb->is_pressed()) {
			bool is_int = bool(ui.edit_cfg.get("integer_scale", false));
			double s = double(ui.edit_cfg.get("scale", 1.0));
			s = is_int ? MAX(1.0, Math::round(s) - 1.0) : MAX(0.1, s / 1.1);
			ui.edit_cfg["scale"] = s;
			_redraw_adjust_dialog(p_ui_idx);
			ui.canvas->accept_event();
			return;
		}
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				ui.canvas->grab_focus();
				ui.drag_start_mouse = mb->get_position();
				ui.drag_start_cfg = ui.edit_cfg.duplicate(true);
				ui.drag_mode = 1; // Mover por defecto

				if (ui.troop_idx >= 0 && ui.troop_idx < category_data[CAT_TROOPS].size()) {
					Dictionary d = category_data[CAT_TROOPS][ui.troop_idx];
					String path = (ui.slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
					Ref<Texture2D> tex = path.is_empty() ? Ref<Texture2D>() : _retain_troop_texture(path);
					if (ClockGearsBattleScene::is_texture_ready(tex)) {
						Rect2 active_r = ClockGearsBattleScene::compute_battleback_screen_rect(vp_rect, tex->get_size(), ui.edit_cfg, ui.slot != 1);
						Vector<Point2> corners = s_get_adjust_handle_points(active_r, vp_rect);
						for (int c = 0; c < corners.size(); c++) {
							if (mb->get_position().distance_to(corners[c]) <= 12.0f) {
								ui.drag_mode = 2 + c;
								break;
							}
						}
					}
				}
			} else {
				ui.drag_mode = 0;
			}
			ui.canvas->accept_event();
			return;
		}
	}
	if (mm.is_valid() && ui.drag_mode > 0) {
		if (ui.drag_mode == 1) {
			Vector2 delta_l = (mm->get_position() - ui.drag_start_mouse) / vp_scale;
			ui.edit_cfg["offset_x"] = double(ui.drag_start_cfg.get("offset_x", 0.0)) + delta_l.x;
			ui.edit_cfg["offset_y"] = double(ui.drag_start_cfg.get("offset_y", 0.0)) + delta_l.y;
		} else if (ui.drag_mode >= 2 && ui.drag_mode <= 5 && ui.troop_idx >= 0 && ui.troop_idx < category_data[CAT_TROOPS].size()) {
			Dictionary d = category_data[CAT_TROOPS][ui.troop_idx];
			String path = (ui.slot == 1) ? String(d.get("battleback1", d.get("background", ""))) : String(d.get("battleback2", d.get("floor", "")));
			Ref<Texture2D> tex = path.is_empty() ? Ref<Texture2D>() : _retain_troop_texture(path);
			if (ClockGearsBattleScene::is_texture_ready(tex)) {
				Rect2 start_r = ClockGearsBattleScene::compute_battleback_screen_rect(vp_rect, tex->get_size(), ui.drag_start_cfg, ui.slot != 1);
				Point2 center = start_r.get_center();
				float dist0 = MAX(10.0f, ui.drag_start_mouse.distance_to(center));
				float dist1 = MAX(5.0f, mm->get_position().distance_to(center));
				double base_s = double(ui.drag_start_cfg.get("scale", 1.0));
				bool is_int = bool(ui.edit_cfg.get("integer_scale", false));
				double ns = base_s * (double(dist1) / double(dist0));
				ns = is_int ? CLAMP(Math::round(ns), 1.0, 16.0) : CLAMP(ns, 0.1, 10.0);
				ui.edit_cfg["scale"] = ns;
			}
		}
		_redraw_adjust_dialog(p_ui_idx);
		ui.canvas->accept_event();
	}
}

void ClockGearsDatabaseDialog::_on_adjust_window_input(const Ref<InputEvent> &p_ev, int p_ui_idx) {
	Ref<InputEventKey> k = p_ev;
	if (!k.is_valid() || !k->is_pressed()) return;
	AdjustDialogUI &ui = troop_adjust_ui[p_ui_idx];
	double step = k->is_shift_pressed() ? 8.0 : 1.0;
	bool moved = false;
	switch (k->get_keycode()) {
		case Key::LEFT:
			ui.edit_cfg["offset_x"] = double(ui.edit_cfg.get("offset_x", 0.0)) - step;
			moved = true;
			break;
		case Key::RIGHT:
			ui.edit_cfg["offset_x"] = double(ui.edit_cfg.get("offset_x", 0.0)) + step;
			moved = true;
			break;
		case Key::UP:
			ui.edit_cfg["offset_y"] = double(ui.edit_cfg.get("offset_y", 0.0)) - step;
			moved = true;
			break;
		case Key::DOWN:
			ui.edit_cfg["offset_y"] = double(ui.edit_cfg.get("offset_y", 0.0)) + step;
			moved = true;
			break;
		default:
			break;
	}
	if (moved) {
		_redraw_adjust_dialog(p_ui_idx);
		if (ui.dialog) {
			ui.dialog->set_input_as_handled();
		}
	}
}

void ClockGearsDatabaseDialog::_draw_troop_enemies_preview(Control *p_canvas, const Rect2 &p_vp_rect, const Dictionary &p_troop_dict, bool p_show_selection, bool p_show_grid) const {
	if (!p_canvas || p_vp_rect.size.x <= 1.0f || p_vp_rect.size.y <= 1.0f) return;

	Array mems = p_troop_dict.get("members", Array());
	const Array &enemies = category_data[CAT_ENEMIES];
	Ref<Font> font = get_theme_default_font();

	Vector<ClockGearsBattleScene::TroopEnemyDrawEntry> entries;
	entries.resize(mems.size());

	for (int i = 0; i < mems.size(); i++) {
		Dictionary m = mems[i];
		int eid = int(m.get("enemy_id", 1));
		float mx = float(m.get("x", 200 + i * 110));
		float my = float(m.get("y", 260));

		Dictionary ed;
		for (int k = 0; k < enemies.size(); k++) {
			Dictionary cand = enemies[k];
			if (int(cand.get("id", k + 1)) == eid) {
				ed = cand;
				break;
			}
		}
		String ename = ed.get("name", vformat("Enemigo %d", eid));
		int elvl = MAX(1, int(ed.get("level", 1)));

		Ref<Texture2D> tex;
		Rect2 src_reg;
		String sprite_source;
		ClockGearsBattleScene::resolve_enemy_preview_sprite(ed, tex, src_reg, sprite_source);

		Ref<Texture2D> retained_tex;
		if (sprite_source == "placeholder") {
			retained_tex = _get_troop_placeholder_texture();
			if (retained_tex.is_valid()) {
				src_reg = Rect2(Point2(0, 0), retained_tex->get_size());
			}
		} else if (ClockGearsBattleScene::is_texture_ready(tex)) {
			// Retener Ref<Texture2D> por miembro para que dos instancias del mismo enemigo no se sobrescriban y liberen su RID antes del render.
			String cache_key = vformat("__enemy_%d_%s_m%d", eid, sprite_source, i);
			troop_preview_tex_cache[cache_key] = tex;
			retained_tex = tex;
		} else {
			troop_waiting_tex_ready = true;
			retained_tex = _get_troop_placeholder_texture();
			if (retained_tex.is_valid()) {
				src_reg = Rect2(Point2(0, 0), retained_tex->get_size());
			}
		}

		ClockGearsBattleScene::TroopEnemyDrawEntry entry;
		entry.name = ename;
		entry.level = elvl;
		entry.hp = 100;
		entry.max_hp = 100;
		entry.is_dead = false;
		entry.sprite_scale = s_get_troop_member_scale(m);
		entry.hp_bar_mode = s_get_troop_member_hp_bar_mode(m);
		entry.hp_bar_pos = s_get_troop_member_hp_bar_pos(m);
		entry.highlight_hud_group = (p_show_selection && troop_hp_bar_dragging && i == selected_troop_member_idx);
		entry.troop_pos = Vector2(mx, my);
		entry.texture = retained_tex;
		entry.src_region = src_reg;
		entries.write[i] = entry;
	}

	bool effective_grid = (p_show_grid && chk_troop_snap_grid && chk_troop_snap_grid->is_pressed());
	ClockGearsBattleScene::draw_troop_enemies(p_canvas, p_vp_rect, entries, selected_troop_member_idx, p_show_selection, effective_grid, font);
}

void ClockGearsDatabaseDialog::_on_troop_placement_canvas_draw() {
	if (!troop_placement_canvas) return;
	Size2 avail = troop_placement_canvas->get_size();
	if (avail.x <= 10 || avail.y <= 10) return;

	const float logical_w = ClockGearsBattleScene::BATTLE_REF_WIDTH;
	const float logical_h = ClockGearsBattleScene::BATTLE_REF_HEIGHT;
	float max_w = MIN(avail.x, logical_w * EDSCALE);
	float desired_h = max_w * (logical_h / logical_w);
	if (Math::abs(troop_placement_canvas->get_custom_minimum_size().y - desired_h) > 2.0f) {
		troop_placement_canvas->set_custom_minimum_size(Size2(0, desired_h));
	}
	Rect2 vp_rect = s_calc_troop_vp_rect(troop_placement_canvas);
	// Fondo azul oscuro liso por defecto (nunca blanco)
	troop_placement_canvas->draw_rect(vp_rect, Color(0.08f, 0.09f, 0.14f, 1.0f), true);
	troop_placement_canvas->draw_rect(vp_rect, Color(0.35f, 0.40f, 0.50f, 0.9f), false, 1.0f);
}

void ClockGearsDatabaseDialog::_on_troop_bg_layer_draw() {
	if (!troop_bg_layer_ctrl) return;
	Rect2 vp_rect = s_calc_troop_vp_rect(troop_bg_layer_ctrl);
	if (vp_rect.size.x <= 1.0f) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_TROOPS].size()) return;
	Dictionary d = category_data[CAT_TROOPS][sel[0]];
	String bg1_path = d.get("battleback1", d.get("background", ""));
	if (bg1_path.is_empty()) return;
	Ref<Texture2D> tex1 = _retain_troop_texture(bg1_path);
	if (!ClockGearsBattleScene::is_texture_ready(tex1)) return;
	Variant cv = d.get("battleback1_cfg", Dictionary());
	Dictionary cfg = (cv.get_type() == Variant::DICTIONARY) ? Dictionary(cv) : Dictionary();
	ClockGearsBattleScene::draw_battleback_layer(troop_bg_layer_ctrl, vp_rect, tex1, cfg, false);
}

void ClockGearsDatabaseDialog::_on_troop_floor_layer_draw() {
	if (!troop_floor_layer_ctrl) return;
	Rect2 vp_rect = s_calc_troop_vp_rect(troop_floor_layer_ctrl);
	if (vp_rect.size.x <= 1.0f) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_TROOPS].size()) return;
	Dictionary d = category_data[CAT_TROOPS][sel[0]];
	String bg2_path = d.get("battleback2", d.get("floor", ""));
	if (bg2_path.is_empty()) return;
	Ref<Texture2D> tex2 = _retain_troop_texture(bg2_path);
	if (!ClockGearsBattleScene::is_texture_ready(tex2)) return;
	Variant cv = d.get("battleback2_cfg", Dictionary());
	Dictionary cfg = (cv.get_type() == Variant::DICTIONARY) ? Dictionary(cv) : Dictionary();
	ClockGearsBattleScene::draw_battleback_layer(troop_floor_layer_ctrl, vp_rect, tex2, cfg, true);
}

void ClockGearsDatabaseDialog::_on_troop_enemies_overlay_draw() {
	if (!troop_enemies_overlay_ctrl) return;
	Rect2 vp_rect = s_calc_troop_vp_rect(troop_enemies_overlay_ctrl);
	if (vp_rect.size.x <= 1.0f) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_TROOPS].size()) return;
	Dictionary d = category_data[CAT_TROOPS][sel[0]];

	// Franja inferior oscurecida del HUD de combate (zona aproximada cubierta por grupo y comandos)
	Rect2 hud_strip = ClockGearsBattleScene::get_battle_hud_strip_rect(vp_rect);
	if (hud_strip.size.x > 1.0f && hud_strip.size.y > 1.0f) {
		troop_enemies_overlay_ctrl->draw_rect(hud_strip, Color(0.03f, 0.04f, 0.08f, 0.52f), true);
		troop_enemies_overlay_ctrl->draw_line(hud_strip.position, hud_strip.position + Vector2(hud_strip.size.x, 0.0f), Color(0.90f, 0.72f, 0.30f, 0.85f), 1.5f);
	}

	_draw_troop_enemies_preview(troop_enemies_overlay_ctrl, vp_rect, d, true, true);
	troop_enemies_overlay_ctrl->draw_rect(vp_rect, Color(0.35f, 0.40f, 0.50f, 0.9f), false, 1.0f);
}

void ClockGearsDatabaseDialog::_on_troop_placement_canvas_gui_input(const Ref<InputEvent> &p_ev) {
	if (!troop_placement_canvas) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_TROOPS].size()) return;
	int rec_idx = sel[0];

	Rect2 vp_rect = s_calc_troop_vp_rect(troop_placement_canvas);
	if (vp_rect.size.x <= 1.0f || vp_rect.size.y <= 1.0f) return;

	Dictionary d = category_data[CAT_TROOPS][rec_idx];
	Array mems = d.get("members", Array());
	const Array &enemies = category_data[CAT_ENEMIES];
	Ref<Font> font = get_theme_default_font();

	Ref<InputEventMouseButton> mb = p_ev;
	Ref<InputEventMouseMotion> mm = p_ev;

	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			int hit_hp_idx = -1;
			for (int i = mems.size() - 1; i >= 0; i--) {
				Dictionary m = mems[i];
				String hp_mode = s_get_troop_member_hp_bar_mode(m);
				if (hp_mode != "libre") {
					continue;
				}
				Vector2 mpos(float(m.get("x", 200)), float(m.get("y", 260)));
				Size2 src_sz = s_resolve_troop_member_src_size(m, enemies);
				int m_scale = s_get_troop_member_scale(m);
				Vector2i hp_pos = s_get_troop_member_hp_bar_pos(m);
				ClockGearsBattleScene::EnemyVisualLayout lay = ClockGearsBattleScene::compute_enemy_visual_layout(mpos, vp_rect, src_sz, m_scale, font, hp_mode, hp_pos);
				if (lay.hud_group_box.grow(4.0f).has_point(mb->get_position())) {
					hit_hp_idx = i;
					Vector2 cur_anchor_ref(float(hp_pos.x), float(hp_pos.y));
					troop_hp_drag_offset = cur_anchor_ref - ClockGearsBattleScene::screen_to_troop_pos(mb->get_position(), vp_rect);
					break;
				}
			}
			if (hit_hp_idx >= 0) {
				selected_troop_member_idx = hit_hp_idx;
				troop_hp_bar_dragging = true;
				troop_member_dragging = false;
				_refresh_troop_members_ui();
				_queue_troop_preview_redraw();
			} else {
				int hit_idx = -1;
				for (int i = mems.size() - 1; i >= 0; i--) {
					Dictionary m = mems[i];
					Vector2 mpos(float(m.get("x", 200)), float(m.get("y", 260)));
					Size2 src_sz = s_resolve_troop_member_src_size(m, enemies);
					int m_scale = s_get_troop_member_scale(m);
					String hp_mode = s_get_troop_member_hp_bar_mode(m);
					Vector2i hp_pos = s_get_troop_member_hp_bar_pos(m);
					ClockGearsBattleScene::EnemyVisualLayout lay = ClockGearsBattleScene::compute_enemy_visual_layout(mpos, vp_rect, src_sz, m_scale, font, hp_mode, hp_pos);
					if (lay.full_box.grow(6.0f).has_point(mb->get_position())) {
						hit_idx = i;
						troop_drag_offset = mpos - ClockGearsBattleScene::screen_to_troop_pos(mb->get_position(), vp_rect);
						break;
					}
				}
				if (hit_idx >= 0) {
					selected_troop_member_idx = hit_idx;
					troop_member_dragging = true;
					troop_hp_bar_dragging = false;
					_refresh_troop_members_ui();
				}
			}
		} else {
			if (troop_hp_bar_dragging) {
				troop_hp_bar_dragging = false;
				_queue_troop_preview_redraw();
				_on_param_changed();
			}
			if (troop_member_dragging) {
				troop_member_dragging = false;
				_refresh_troop_members_ui();
				_on_param_changed();
			}
		}
	} else if (mb.is_valid() && mb->get_button_index() == MouseButton::RIGHT && mb->is_pressed()) {
		int hit_idx = -1;
		for (int i = mems.size() - 1; i >= 0; i--) {
			Dictionary m = mems[i];
			Vector2 mpos(float(m.get("x", 200)), float(m.get("y", 260)));
			Size2 src_sz = s_resolve_troop_member_src_size(m, enemies);
			int m_scale = s_get_troop_member_scale(m);
			String hp_mode = s_get_troop_member_hp_bar_mode(m);
			Vector2i hp_pos = s_get_troop_member_hp_bar_pos(m);
			ClockGearsBattleScene::EnemyVisualLayout lay = ClockGearsBattleScene::compute_enemy_visual_layout(mpos, vp_rect, src_sz, m_scale, font, hp_mode, hp_pos);
			if (lay.full_box.grow(6.0f).has_point(mb->get_position())) {
				hit_idx = i;
				break;
			}
		}
		if (hit_idx >= 0 && troop_ctx_menu && troop_scale_submenu) {
			selected_troop_member_idx = hit_idx;
			troop_ctx_member_idx = hit_idx;
			_refresh_troop_members_ui();

			int cur_scale = s_get_troop_member_scale(mems[hit_idx]);
			for (int s = 1; s <= 6; s++) {
				int item_idx = troop_scale_submenu->get_item_index(s);
				if (item_idx >= 0) {
					troop_scale_submenu->set_item_checked(item_idx, s == cur_scale);
				}
			}
			s_update_hp_bar_submenu_checks(troop_hp_bar_submenu, s_get_troop_member_hp_bar_mode(mems[hit_idx]));

			if (ClockGearsTheme::get_singleton()) {
				ClockGearsTheme::get_singleton()->apply_to_window(troop_ctx_menu);
				ClockGearsTheme::get_singleton()->apply_to_window(troop_scale_submenu);
				if (troop_hp_bar_submenu) {
					ClockGearsTheme::get_singleton()->apply_to_window(troop_hp_bar_submenu);
				}
			}
			troop_ctx_menu->reset_size();
			Size2i menu_sz = troop_ctx_menu->get_contents_minimum_size();
			if (menu_sz.x <= 0 || menu_sz.y <= 0) {
				menu_sz = troop_ctx_menu->get_size();
			}
			menu_sz.x = MAX(menu_sz.x, 120);
			menu_sz.y = MAX(menu_sz.y, 40);

			Vector2 scr_pos = troop_placement_canvas->get_screen_position() + mb->get_position();
			int px = int(Math::round(scr_pos.x));
			int py = int(Math::round(scr_pos.y));
			Vector2i db_pos = get_position();
			Size2i db_sz = get_size();
			if (db_sz.x > menu_sz.x + 8 && db_sz.y > menu_sz.y + 8) {
				px = CLAMP(px, db_pos.x + 4, db_pos.x + db_sz.x - menu_sz.x - 4);
				py = CLAMP(py, db_pos.y + 4, db_pos.y + db_sz.y - menu_sz.y - 4);
			}
			Rect2i usable_scr = DisplayServer::get_singleton()->screen_get_usable_rect(DisplayServer::get_singleton()->window_get_current_screen(get_window_id()));
			if (usable_scr.size.x > menu_sz.x + 8 && usable_scr.size.y > menu_sz.y + 8) {
				px = CLAMP(px, usable_scr.position.x + 4, usable_scr.position.x + usable_scr.size.x - menu_sz.x - 4);
				py = CLAMP(py, usable_scr.position.y + 4, usable_scr.position.y + usable_scr.size.y - menu_sz.y - 4);
			}
			troop_ctx_menu->popup(Rect2i(Vector2i(px, py), Size2i()));
		}
	} else if (mm.is_valid() && troop_hp_bar_dragging) {
		if (selected_troop_member_idx >= 0 && selected_troop_member_idx < mems.size()) {
			Dictionary m = mems[selected_troop_member_idx];
			Vector2 desired_anchor = ClockGearsBattleScene::screen_to_troop_pos(mm->get_position(), vp_rect) + troop_hp_drag_offset;
			Vector2i clamped_pos = ClockGearsBattleUnit::clamp_free_hp_bar_pos(Vector2i(
					int(Math::round(desired_anchor.x)),
					int(Math::round(desired_anchor.y))));

			m["hp_bar_mode"] = "libre";
			m["hp_bar_x"] = clamped_pos.x;
			m["hp_bar_y"] = clamped_pos.y;
			m.erase("hp_bar_offset_x");
			m.erase("hp_bar_offset_y");
			mems[selected_troop_member_idx] = m;
			d["members"] = mems;
			category_data[CAT_TROOPS][rec_idx] = d;
			_update_troop_hud_warning(d);
			_queue_troop_preview_redraw();
		}
	} else if (mm.is_valid() && troop_member_dragging) {
		if (selected_troop_member_idx >= 0 && selected_troop_member_idx < mems.size()) {
			Vector2 mouse_logical = ClockGearsBattleScene::screen_to_troop_pos(mm->get_position(), vp_rect) + troop_drag_offset;
			Dictionary m = mems[selected_troop_member_idx];
			int nx = int(Math::round(mouse_logical.x));
			int ny = int(Math::round(mouse_logical.y));
			if (chk_troop_snap_grid && chk_troop_snap_grid->is_pressed()) {
				Size2 src_sz = s_resolve_troop_member_src_size(m, enemies);
				int m_scale = s_get_troop_member_scale(m);
				Vector2i snapped = s_snap_troop_member_pos_to_grid(mouse_logical.x, mouse_logical.y, src_sz, m_scale);
				nx = snapped.x;
				ny = snapped.y;
			}
			nx = CLAMP(nx, 0, int(ClockGearsBattleScene::BATTLE_REF_WIDTH));
			ny = CLAMP(ny, 0, int(ClockGearsBattleScene::BATTLE_REF_HEIGHT));
			m["x"] = nx;
			m["y"] = ny;
			mems[selected_troop_member_idx] = m;
			d["members"] = mems;
			category_data[CAT_TROOPS][rec_idx] = d;

			// Sincronizar los SpinBox X e Y en vivo
			if (troop_members_list_vbox && selected_troop_member_idx < troop_members_list_vbox->get_child_count()) {
				Node *row = troop_members_list_vbox->get_child(selected_troop_member_idx);
				if (row && row->get_child_count() >= 6) {
					updating_ui = true;
					SpinBox *sp_x = Object::cast_to<SpinBox>(row->get_child(3));
					SpinBox *sp_y = Object::cast_to<SpinBox>(row->get_child(5));
					if (sp_x) sp_x->set_value(nx);
					if (sp_y) sp_y->set_value(ny);
					updating_ui = false;
				}
			}
			_update_troop_hud_warning(d);
			_queue_troop_preview_redraw();
		}
	} else if (mm.is_valid()) {
		Rect2 hud_strip = ClockGearsBattleScene::get_battle_hud_strip_rect(vp_rect);
		if (hud_strip.has_point(mm->get_position())) {
			troop_placement_canvas->set_tooltip_text(CG_DB_TR("Zona del HUD de batalla (grupo y comandos): los enemigos colocados aquí quedarán parcial o totalmente tapados en combate."));
		} else {
			troop_placement_canvas->set_tooltip_text("");
		}
	}
}

void ClockGearsDatabaseDialog::_on_troop_member_enemy_changed(int p_item_idx, int p_idx) {
	if (updating_ui) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	if (p_idx >= 0 && p_idx < mems.size()) {
		int enemy_id = p_item_idx;
		if (troop_members_list_vbox && p_idx < troop_members_list_vbox->get_child_count()) {
			Node *row = troop_members_list_vbox->get_child(p_idx);
			if (row && row->get_child_count() > 1) {
				OptionButton *opt = Object::cast_to<OptionButton>(row->get_child(1));
				if (opt && p_item_idx >= 0 && p_item_idx < opt->get_item_count()) {
					enemy_id = opt->get_item_id(p_item_idx);
				}
			}
		}
		Dictionary m = mems[p_idx];
		m["enemy_id"] = enemy_id;
		mems[p_idx] = m;
		d["members"] = mems;
		arr[idx] = d;
		_update_troop_hud_warning(d);
		_queue_troop_preview_redraw();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_troop_member_x_changed(double p_val, int p_idx) {
	if (updating_ui) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	if (p_idx >= 0 && p_idx < mems.size()) {
		Dictionary m = mems[p_idx];
		m["x"] = int(p_val);
		mems[p_idx] = m;
		d["members"] = mems;
		arr[idx] = d;
		_update_troop_hud_warning(d);
		_queue_troop_preview_redraw();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_troop_member_y_changed(double p_val, int p_idx) {
	if (updating_ui) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());
	if (p_idx >= 0 && p_idx < mems.size()) {
		Dictionary m = mems[p_idx];
		m["y"] = int(p_val);
		mems[p_idx] = m;
		d["members"] = mems;
		arr[idx] = d;
		_update_troop_hud_warning(d);
		_queue_troop_preview_redraw();
		_on_param_changed();
	}
}

// ============================================================================
// Debilidades y Resistencias + Ranuras fijas y Lanzamientos de Enemigos
// ============================================================================

void ClockGearsDatabaseDialog::_refresh_enemy_element_rates_ui() {
	if (!enemy_element_rates_rows_vbox) return;
	while (enemy_element_rates_rows_vbox->get_child_count() > 0) {
		Node *c = enemy_element_rates_rows_vbox->get_child(0);
		enemy_element_rates_rows_vbox->remove_child(c);
		memdelete(c);
	}
	enemy_elem_preset_opts.clear();
	enemy_elem_rate_spins.clear();
	enemy_elem_ids.clear();

	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	PackedStringArray elems = cat ? cat->get_elements() : PackedStringArray();

	Dictionary d;
	int enemy_cat = (current_category == CAT_ARPG_ENEMIES) ? CAT_ARPG_ENEMIES : CAT_ENEMIES;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[enemy_cat].size()) {
		d = category_data[enemy_cat][sel[0]];
	}
	Array traits = d.get("traits", Array());

	int total_rows = elems.size() + ((current_category == CAT_ARPG_ENEMIES) ? 1 : 0);
	for (int row_n = 0; row_n < total_rows; row_n++) {
		int elem_id = 0;
		String elem_name;
		String elem_key;
		if (current_category == CAT_ARPG_ENEMIES) {
			if (row_n == 0) {
				elem_id = 0;
				elem_name = CG_DB_TR("Físico");
				elem_key = "fisico";
			} else {
				elem_id = row_n;
				elem_name = elems[row_n - 1];
				elem_key = cat ? cat->get_element_key(elem_id) : elem_name.to_lower();
			}
		} else {
			elem_id = row_n + 1;
			elem_name = elems[row_n];
			elem_key = cat ? cat->get_element_key(elem_id) : elem_name.to_lower();
		}

		double rate_pct = 100.0;
		for (int t = 0; t < traits.size(); t++) {
			if (traits[t].get_type() != Variant::DICTIONARY) continue;
			Dictionary tr = traits[t];
			if (int(tr.get("code", 0)) != CG_TRAIT_ELEMENT_RATE) continue;
			String tr_key = String(tr.get("element_key", "")).strip_edges().to_lower();
			int tr_id = int(tr.get("data_id", -1));
			if ((!tr_key.is_empty() && tr_key == elem_key.to_lower()) || tr_id == elem_id) {
				rate_pct = double(tr.get("value", 100.0));
				break;
			}
		}

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_theme_constant_override("separation", 4);

		Label *lbl = memnew(Label(elem_name + ":"));
		lbl->set_custom_minimum_size(Size2(82 * EDSCALE, 0));
		lbl->set_clip_text(true);
		row->add_child(lbl);

		OptionButton *opt = memnew(OptionButton);
		opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		opt->set_custom_minimum_size(Size2(115 * EDSCALE, 0));
		opt->set_clip_text(true);
		opt->add_item(CG_DB_TR("Normal x1 (100%)"), 100);
		opt->add_item(CG_DB_TR("Débil x2 (200%)"), 200);
		opt->add_item(CG_DB_TR("Resiste x0.5 (50%)"), 50);
		opt->add_item(CG_DB_TR("Inmune x0 (0%)"), 0);
		opt->add_item(CG_DB_TR("Libre (%)"), -1);

		int preset_sel = 4;
		if (Math::is_equal_approx(rate_pct, 100.0)) preset_sel = 0;
		else if (Math::is_equal_approx(rate_pct, 200.0)) preset_sel = 1;
		else if (Math::is_equal_approx(rate_pct, 50.0)) preset_sel = 2;
		else if (Math::is_equal_approx(rate_pct, 0.0)) preset_sel = 3;
		opt->select(preset_sel);

		int row_idx = enemy_elem_ids.size();
		opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_enemy_elem_preset_changed).bind(row_idx));
		row->add_child(opt);

		SpinBox *sp = memnew(SpinBox);
		sp->set_min(0);
		sp->set_max(1000);
		sp->set_step(1);
		sp->set_suffix("%");
		sp->set_value(rate_pct);
		sp->set_custom_minimum_size(Size2(72 * EDSCALE, 0));
		_bind_numeric_spinbox(sp);
		sp->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_enemy_elem_rate_spin_changed).bind(row_idx));
		row->add_child(sp);

		enemy_element_rates_rows_vbox->add_child(row);
		enemy_elem_preset_opts.push_back(opt);
		enemy_elem_rate_spins.push_back(sp);
		enemy_elem_ids.push_back(elem_id);
	}
}

void ClockGearsDatabaseDialog::_on_enemy_elem_preset_changed(int p_sel_idx, int p_row) {
	if (updating_ui || p_row < 0 || p_row >= enemy_elem_ids.size()) return;
	OptionButton *opt = enemy_elem_preset_opts[p_row];
	SpinBox *sp = enemy_elem_rate_spins[p_row];
	if (!opt || !sp) return;

	int preset_val = opt->get_item_id(p_sel_idx);
	if (preset_val >= 0) {
		updating_ui = true;
		sp->set_value(preset_val);
		updating_ui = false;
		_update_enemy_element_rate_trait(enemy_elem_ids[p_row], double(preset_val));
	}
}

void ClockGearsDatabaseDialog::_on_enemy_elem_rate_spin_changed(double p_val, int p_row) {
	if (updating_ui || p_row < 0 || p_row >= enemy_elem_ids.size()) return;
	OptionButton *opt = enemy_elem_preset_opts[p_row];
	if (opt) {
		int preset_sel = 4;
		if (Math::is_equal_approx(p_val, 100.0)) preset_sel = 0;
		else if (Math::is_equal_approx(p_val, 200.0)) preset_sel = 1;
		else if (Math::is_equal_approx(p_val, 50.0)) preset_sel = 2;
		else if (Math::is_equal_approx(p_val, 0.0)) preset_sel = 3;
		updating_ui = true;
		opt->select(preset_sel);
		updating_ui = false;
	}
	_update_enemy_element_rate_trait(enemy_elem_ids[p_row], p_val);
}

void ClockGearsDatabaseDialog::_update_enemy_element_rate_trait(int p_elem_id, double p_rate_pct) {
	if (current_category != CAT_ENEMIES && current_category != CAT_ARPG_ENEMIES) return;
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) return;
	int rec_idx = sel[0];

	Dictionary d = arr[rec_idx];
	Array traits = d.get("traits", Array());
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	String elem_key = (p_elem_id == 0) ? String("fisico") : (cat ? cat->get_element_key(p_elem_id) : String(""));
	int found_idx = -1;
	for (int i = 0; i < traits.size(); i++) {
		if (traits[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary tr = traits[i];
		if (int(tr.get("code", 0)) != CG_TRAIT_ELEMENT_RATE) continue;
		String tr_key = String(tr.get("element_key", "")).strip_edges().to_lower();
		if ((!tr_key.is_empty() && tr_key == elem_key.to_lower()) || int(tr.get("data_id", -1)) == p_elem_id) {
			found_idx = i;
			break;
		}
	}

	if (current_category == CAT_ENEMIES && Math::is_equal_approx(p_rate_pct, 100.0)) {
		if (found_idx >= 0) {
			traits.remove_at(found_idx);
		}
	} else {
		Dictionary tr;
		tr["code"] = int(CG_TRAIT_ELEMENT_RATE);
		tr["data_id"] = p_elem_id;
		tr["element_key"] = elem_key;
		tr["element_migrated"] = true;
		tr["value"] = p_rate_pct;
		if (found_idx >= 0) {
			traits[found_idx] = tr;
		} else {
			traits.push_back(tr);
		}
	}

	d["traits"] = traits;
	if (current_category == CAT_ARPG_ENEMIES) {
		ClockGearsDataManager::normalize_record_defaults(CAT_ARPG_ENEMIES, d);
	}
	arr[rec_idx] = d;
	_refresh_traits_tree(d.get("traits", traits));
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_arpg_ai_field_changed() {
	if (updating_ui || current_category != CAT_ARPG_ENEMIES) {
		return;
	}
	Array &arr = category_data[CAT_ARPG_ENEMIES];
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) {
		return;
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	if (arpg_ai_type_opt) {
		int ai_sel = arpg_ai_type_opt->get_selected();
		if (ai_sel >= 0 && ai_sel < arpg_ai_type_opt->get_item_count()) {
			d["ai_type"] = String(arpg_ai_type_opt->get_item_metadata(ai_sel));
		}
	}
	if (arpg_ai_vision_spin) {
		d["vision_radius"] = MAX(0.0, double(arpg_ai_vision_spin->get_value()));
	}
	if (arpg_ai_range_spin) {
		d["attack_range"] = MAX(0.0, double(arpg_ai_range_spin->get_value()));
	}
	if (arpg_ai_interval_spin) {
		d["attack_interval"] = MAX(0.0, double(arpg_ai_interval_spin->get_value()));
	}
	if (arpg_ai_kb_resist_spin) {
		d["knockback_resist"] = CLAMP(int(arpg_ai_kb_resist_spin->get_value()), 0, 100);
	}
	ClockGearsDataManager::normalize_record_defaults(CAT_ARPG_ENEMIES, d);
	arr[rec_idx] = d;
	_load_movement_tab_from_record(d);
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_arpg_attack_field_changed() {
	if (updating_ui || current_category != CAT_ARPG_ENEMIES) {
		return;
	}
	Array &arr = category_data[CAT_ARPG_ENEMIES];
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= arr.size()) {
		return;
	}
	int rec_idx = sel[0];
	Dictionary d = arr[rec_idx];
	if (arpg_atk_interval_spin) {
		d["attack_interval"] = MAX(0.0, double(arpg_atk_interval_spin->get_value()));
	}
	if (arpg_atk_interval_var_spin) {
		d["attack_interval_variance_pct"] = CLAMP(double(arpg_atk_interval_var_spin->get_value()), 0.0, 100.0);
	}
	if (arpg_atk_speed_spin) {
		d["attack_speed_pct"] = MAX(1.0, double(arpg_atk_speed_spin->get_value()));
	}
	if (arpg_atk_windup_spin) {
		d["attack_windup_ms"] = MAX(0, int(arpg_atk_windup_spin->get_value()));
	}
	if (arpg_atk_recovery_spin) {
		d["attack_recovery_ms"] = MAX(0, int(arpg_atk_recovery_spin->get_value()));
	}
	if (arpg_atk_dmg_mult_spin) {
		d["attack_damage_mult_pct"] = MAX(0.0, double(arpg_atk_dmg_mult_spin->get_value()));
	}
	if (arpg_atk_range_spin) {
		d["attack_range"] = MAX(0.0, double(arpg_atk_range_spin->get_value()));
	}
	ClockGearsDataManager::normalize_record_defaults(CAT_ARPG_ENEMIES, d);
	arr[rec_idx] = d;
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_ensure_enemy_fixed_anim_slots(Dictionary &r_enemy_dict) {
	r_enemy_dict["sprite_data"] = current_sprite_data.to_dict();
	ClockGearsSpriteSheetData::migrate_enemy_record(r_enemy_dict);
	current_sprite_data.from_dict(r_enemy_dict.get("sprite_data", Dictionary()));
}

static String s_format_enemy_slot_display_label(const String &p_key, const String &p_base_label, const ClockGearsSpriteSheetData &p_sprite_data) {
	int fc = 0;
	if (p_sprite_data.animations.has(p_key)) {
		fc = p_sprite_data.animations[p_key].get_frame_count();
	}
	if (fc > 0) {
		return vformat(String::utf8("%s (%d %s)"), p_base_label, fc, fc == 1 ? "frame" : "frames");
	}
	return vformat(String::utf8("%s (vacía)"), p_base_label);
}

static bool s_is_enemy_fixed_slot_name(const String &p_name) {
	String low = p_name.strip_edges().to_lower();
	return (low == "idle" || low == "attack" || low == "hurt" || low == "death" || low == "buff");
}

void ClockGearsDatabaseDialog::_refresh_enemy_casts_ui() {
	if (!enemy_casts_rows_vbox) return;
	while (enemy_casts_rows_vbox->get_child_count() > 0) {
		Node *c = enemy_casts_rows_vbox->get_child(0);
		enemy_casts_rows_vbox->remove_child(c);
		memdelete(c);
	}
	if (current_category != CAT_ENEMIES) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) return;

	Dictionary d = category_data[CAT_ENEMIES][sel[0]];
	Array casts = d.get("cast_animations", Array());
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	PackedStringArray elems = cat ? cat->get_elements() : PackedStringArray();

	for (int i = 0; i < casts.size(); i++) {
		if (casts[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary cd = casts[i];
		String cname = cd.get("name", vformat("Lanzamiento %d", i + 1));
		int celem = int(cd.get("element_id", 0));

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_theme_constant_override("separation", 4);

		String btn_label = s_format_enemy_slot_display_label(cname, cname, current_sprite_data);
		Button *btn_edit_slot = memnew(Button(btn_label));
		btn_edit_slot->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		btn_edit_slot->set_tooltip_text(CG_DB_TR("Clic para editar los frames de esta ranura de Lanzamiento"));
		btn_edit_slot->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::select_anim_slot).bind(cname));
		row->add_child(btn_edit_slot);

		OptionButton *opt_el = memnew(OptionButton);
		opt_el->set_custom_minimum_size(Size2(110 * EDSCALE, 0));
		opt_el->add_item(CG_DB_TR("(Ninguno)"), 0);
		for (int e = 0; e < elems.size(); e++) {
			opt_el->add_item(elems[e], e + 1);
		}
		int idx_to_sel = opt_el->get_item_index(celem);
		opt_el->select(idx_to_sel >= 0 ? idx_to_sel : 0);
		opt_el->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_enemy_cast_element_changed).bind(i));
		row->add_child(opt_el);

		Button *btn_ren = memnew(Button(String::utf8("✏")));
		btn_ren->set_tooltip_text(CG_DB_TR("Renombrar este lanzamiento"));
		btn_ren->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_rename_enemy_cast_pressed).bind(i));
		row->add_child(btn_ren);

		Button *btn_del = memnew(Button("-"));
		btn_del->set_tooltip_text(CG_DB_TR("Eliminar este lanzamiento"));
		btn_del->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_enemy_cast_pressed).bind(i));
		row->add_child(btn_del);

		enemy_casts_rows_vbox->add_child(row);
	}
}

void ClockGearsDatabaseDialog::_on_rename_enemy_cast_pressed(int p_idx) {
	if (current_category != CAT_ENEMIES) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) return;
	Dictionary d = category_data[CAT_ENEMIES][sel[0]];
	Array casts = d.get("cast_animations", Array());
	if (p_idx < 0 || p_idx >= casts.size() || casts[p_idx].get_type() != Variant::DICTIONARY) return;
	Dictionary cd = casts[p_idx];
	String cname = cd.get("name", "");
	if (cname.is_empty()) return;
	if (rename_anim_dialog && rename_anim_name_edit) {
		rename_anim_name_edit->set_text(cname);
		rename_anim_name_edit->set_meta("old_anim_name", cname);
		rename_anim_dialog->popup_centered();
		rename_anim_name_edit->grab_focus();
		rename_anim_name_edit->select_all();
	}
}

void ClockGearsDatabaseDialog::_refresh_enemy_others_ui() {
	if (!enemy_others_rows_vbox) return;
	while (enemy_others_rows_vbox->get_child_count() > 0) {
		Node *c = enemy_others_rows_vbox->get_child(0);
		enemy_others_rows_vbox->remove_child(c);
		memdelete(c);
	}
	if (current_category != CAT_ENEMIES) {
		if (enemy_others_box) enemy_others_box->set_visible(false);
		return;
	}
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) {
		if (enemy_others_box) enemy_others_box->set_visible(false);
		return;
	}

	Dictionary d = category_data[CAT_ENEMIES][sel[0]];
	Array others = d.get("other_animations", Array());
	Array casts = d.get("cast_animations", Array());
	if (enemy_others_box) {
		enemy_others_box->set_visible(!others.is_empty());
	}
	if (others.is_empty()) return;

	for (int i = 0; i < others.size(); i++) {
		String oname;
		if (others[i].get_type() == Variant::DICTIONARY) {
			oname = String(Dictionary(others[i]).get("name", "")).strip_edges();
		} else {
			oname = String(others[i]).strip_edges();
		}
		if (oname.is_empty()) continue;

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_theme_constant_override("separation", 4);

		String btn_label = s_format_enemy_slot_display_label(oname, oname, current_sprite_data);
		Button *btn_sel = memnew(Button(btn_label));
		btn_sel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		btn_sel->set_tooltip_text(CG_DB_TR("Clic para ver los frames de esta animación migrada"));
		btn_sel->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::select_anim_slot).bind(oname));
		row->add_child(btn_sel);

		OptionButton *opt_reassign = memnew(OptionButton);
		opt_reassign->set_custom_minimum_size(Size2(140 * EDSCALE, 0));
		opt_reassign->add_item(CG_DB_TR("Reasignar a..."), 0);
		opt_reassign->set_item_metadata(0, "");
		const char *fixed_ids[] = { "idle", "attack", "hurt", "death", "buff" };
		for (int f = 0; f < 5; f++) {
			String fid = fixed_ids[f];
			opt_reassign->add_item(ClockGearsSpriteSheetData::get_enemy_fixed_slot_label(fid), f + 1);
			opt_reassign->set_item_metadata(f + 1, fid);
		}
		int next_idx = 6;
		opt_reassign->add_item(CG_DB_TR("+ Como Lanzamiento"), next_idx);
		opt_reassign->set_item_metadata(next_idx, "__new_cast__");
		next_idx++;
		for (int c = 0; c < casts.size(); c++) {
			if (casts[c].get_type() != Variant::DICTIONARY) continue;
			String cname = String(Dictionary(casts[c]).get("name", "")).strip_edges();
			if (!cname.is_empty()) {
				opt_reassign->add_item(vformat("Lanzamiento: %s", cname), next_idx);
				opt_reassign->set_item_metadata(next_idx, cname);
				next_idx++;
			}
		}
		opt_reassign->select(0);
		opt_reassign->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_reassign_enemy_other_selected).bind(i));
		row->add_child(opt_reassign);

		Button *btn_del = memnew(Button("-"));
		btn_del->set_tooltip_text(CG_DB_TR("Borrar esta animación de 'Otras'"));
		btn_del->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_enemy_other_pressed).bind(i));
		row->add_child(btn_del);

		enemy_others_rows_vbox->add_child(row);
	}
}

void ClockGearsDatabaseDialog::_on_reassign_enemy_other_selected(int p_opt_idx, int p_other_idx) {
	if (updating_ui || current_category != CAT_ENEMIES || p_opt_idx <= 0) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) return;
	Dictionary d = category_data[CAT_ENEMIES][sel[0]];
	Array others = d.get("other_animations", Array());
	if (p_other_idx < 0 || p_other_idx >= others.size()) return;
	String oname = (others[p_other_idx].get_type() == Variant::DICTIONARY)
			? String(Dictionary(others[p_other_idx]).get("name", "")).strip_edges()
			: String(others[p_other_idx]).strip_edges();
	if (oname.is_empty()) return;

	String target_slot;
	if (p_opt_idx >= 1 && p_opt_idx <= 5) {
		const char *fixed_ids[] = { "idle", "attack", "hurt", "death", "buff" };
		target_slot = fixed_ids[p_opt_idx - 1];
	} else if (p_opt_idx == 6) {
		target_slot = oname; // Convertir en Lanzamiento con su propio nombre
	} else {
		Array casts = d.get("cast_animations", Array());
		int c_idx = p_opt_idx - 7;
		if (c_idx >= 0 && c_idx < casts.size() && casts[c_idx].get_type() == Variant::DICTIONARY) {
			target_slot = String(Dictionary(casts[c_idx]).get("name", "")).strip_edges();
		}
	}
	if (!target_slot.is_empty()) {
		reassign_enemy_other_animation(sel[0], oname, target_slot);
	}
}

void ClockGearsDatabaseDialog::_on_remove_enemy_other_pressed(int p_other_idx) {
	if (current_category != CAT_ENEMIES) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) return;
	Dictionary d = category_data[CAT_ENEMIES][sel[0]];
	Array others = d.get("other_animations", Array());
	if (p_other_idx < 0 || p_other_idx >= others.size()) return;
	String oname = (others[p_other_idx].get_type() == Variant::DICTIONARY)
			? String(Dictionary(others[p_other_idx]).get("name", "")).strip_edges()
			: String(others[p_other_idx]).strip_edges();
	if (!oname.is_empty()) {
		remove_enemy_other_animation(sel[0], oname);
	}
}

void ClockGearsDatabaseDialog::_on_add_enemy_cast_pressed() {
	if (current_category != CAT_ENEMIES) return;
	if (new_anim_dialog) {
		if (new_anim_name_edit) {
			new_anim_name_edit->set_text("");
			new_anim_name_edit->set_meta("is_enemy_cast_slot", true);
		}
		new_anim_dialog->popup_centered();
	}
}

void ClockGearsDatabaseDialog::_on_remove_enemy_cast_pressed(int p_idx) {
	if (current_category != CAT_ENEMIES) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) return;
	int rec_idx = sel[0];

	Dictionary d = category_data[CAT_ENEMIES][rec_idx];
	Array casts = d.get("cast_animations", Array());
	if (p_idx >= 0 && p_idx < casts.size()) {
		Dictionary cd = casts[p_idx];
		String cname = cd.get("name", "");
		casts.remove_at(p_idx);
		if (!cname.is_empty() && current_sprite_data.animations.has(cname) && !s_is_enemy_fixed_slot_name(cname)) {
			current_sprite_data.animations.erase(cname);
		}
		d["cast_animations"] = casts;
		d["sprite_data"] = current_sprite_data.to_dict();
		category_data[CAT_ENEMIES][rec_idx] = d;
		_refresh_enemy_casts_ui();
		_refresh_anim_track_options();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_enemy_cast_element_changed(int p_sel_idx, int p_idx) {
	if (updating_ui || current_category != CAT_ENEMIES) return;
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_ENEMIES].size()) return;
	int rec_idx = sel[0];

	Dictionary d = category_data[CAT_ENEMIES][rec_idx];
	Array casts = d.get("cast_animations", Array());
	if (p_idx >= 0 && p_idx < casts.size()) {
		Dictionary cd = casts[p_idx];
		cd["element_id"] = p_sel_idx;
		ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
		PackedStringArray elems = cat ? cat->get_elements() : PackedStringArray();
		cd["element"] = (p_sel_idx >= 1 && p_sel_idx <= elems.size()) ? elems[p_sel_idx - 1] : "";
		cd["element_key"] = (cat && p_sel_idx >= 1) ? cat->get_element_key(p_sel_idx) : "";
		casts[p_idx] = cd;
		d["cast_animations"] = casts;
		category_data[CAT_ENEMIES][rec_idx] = d;
		_mark_database_dirty();
	}
}

// ============================================================================
// Métodos del Editor de Sprites y Animaciones (SpriteFrames Style - Fase 8A)
// ============================================================================

String ClockGearsDatabaseDialog::_get_active_anim_name() const {
	if (anim_track_opt && anim_track_opt->get_selected() >= 0 && anim_track_opt->get_selected() < anim_track_opt->get_item_count()) {
		int sel_idx = anim_track_opt->get_selected();
		Variant meta = anim_track_opt->get_item_metadata(sel_idx);
		if (meta.get_type() == Variant::STRING && !String(meta).is_empty()) {
			return String(meta);
		}
		return anim_track_opt->get_item_text(sel_idx);
	}
	if (anim_item_list) {
		Vector<int> sel = anim_item_list->get_selected_items();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < anim_item_list->get_item_count()) {
			Variant meta = anim_item_list->get_item_metadata(sel[0]);
			if (meta.get_type() == Variant::STRING && !String(meta).is_empty()) {
				return String(meta);
			}
			return anim_item_list->get_item_text(sel[0]);
		}
	}
	return "";
}

static bool s_parse_actor_fixed_slot_key(const String &p_slot_name, String &r_state_id, int &r_dir) {
	String prefix;
	if (p_slot_name.begins_with("mover_")) {
		r_state_id = "caminar";
		prefix = "mover_";
	} else if (p_slot_name.begins_with("correr_")) {
		r_state_id = "correr";
		prefix = "correr_";
	} else if (p_slot_name.begins_with("quieto_")) {
		r_state_id = "quieto";
		prefix = "quieto_";
	} else if (p_slot_name.begins_with("atacar_")) {
		r_state_id = "atacar";
		prefix = "atacar_";
	} else if (p_slot_name.begins_with("bloquear_")) {
		r_state_id = "bloquear";
		prefix = "bloquear_";
	} else {
		return false;
	}
	String suf = p_slot_name.substr(prefix.length());
	static const struct {
		const char *suf;
		int dir;
	} map[] = {
		{ "abajo_izquierda", 1 },
		{ "abajo_derecha", 3 },
		{ "arriba_izquierda", 7 },
		{ "arriba_derecha", 9 },
		{ "abajo", 2 },
		{ "izquierda", 4 },
		{ "derecha", 6 },
		{ "arriba", 8 },
	};
	for (int i = 0; i < 8; i++) {
		if (suf == map[i].suf) {
			r_dir = map[i].dir;
			return true;
		}
	}
	return false;
}

void ClockGearsDatabaseDialog::_ensure_actor_fixed_slot_exists(const String &p_slot_name) {
	if (!s_is_anim_slot_category(current_category) || p_slot_name.is_empty()) {
		return;
	}
	if (!ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(p_slot_name)) {
		return;
	}
	if (!current_sprite_data.animations.has(p_slot_name)) {
		ClockGearsSpriteAnimation a;
		a.name = p_slot_name;
		a.fps = 8.0f;
		a.loop = true;
		a.random_order = false;
		current_sprite_data.animations[p_slot_name] = a;
	}
}

void ClockGearsDatabaseDialog::_update_current_anim_slot_labels() {
	if (anim_track_opt) {
		for (int i = 0; i < anim_track_opt->get_item_count(); i++) {
			String key = String(anim_track_opt->get_item_metadata(i));
			if (key.is_empty()) continue;
			String base_lbl = (current_category == CAT_ENEMIES && s_is_enemy_fixed_slot_name(key))
					? ClockGearsSpriteSheetData::get_enemy_fixed_slot_label(key)
					: key;
			String disp = s_format_enemy_slot_display_label(key, base_lbl, current_sprite_data);
			anim_track_opt->set_item_text(i, disp);
			if (anim_item_list && i < anim_item_list->get_item_count()) {
				anim_item_list->set_item_text(i, disp);
			}
		}
	}
	if (current_category == CAT_ENEMIES) {
		_refresh_enemy_casts_ui();
		_refresh_enemy_others_ui();
	}
}

void ClockGearsDatabaseDialog::_refresh_anim_track_options() {
	if (!anim_track_opt) {
		return;
	}
	if (!_has_valid_selected_record()) {
		_clear_and_disable_anim_editor();
		return;
	}
	updating_ui = true;
	String prev_sel = _get_active_anim_name();
	anim_track_opt->clear();
	if (anim_item_list) {
		anim_item_list->clear();
	}

	int idx = 0;
	int sel_idx = 0;

	if (current_category == CAT_ENEMIES) {
		auto add_enemy_slot_item = [&](const String &p_key, const String &p_base_label) {
			if (!current_sprite_data.animations.has(p_key)) return;
			String disp = s_format_enemy_slot_display_label(p_key, p_base_label, current_sprite_data);
			anim_track_opt->add_item(disp, idx);
			anim_track_opt->set_item_metadata(idx, p_key);
			if (anim_item_list) {
				anim_item_list->add_item(disp);
				anim_item_list->set_item_metadata(idx, p_key);
			}
			if (p_key == prev_sel || (prev_sel.is_empty() && p_key == "idle")) {
				sel_idx = idx;
			}
			idx++;
		};

		// 1. 5 ranuras fijas en orden estable
		const char *fixed_ids[] = { "idle", "attack", "hurt", "death", "buff" };
		for (int f = 0; f < 5; f++) {
			String fid = fixed_ids[f];
			add_enemy_slot_item(fid, ClockGearsSpriteSheetData::get_enemy_fixed_slot_label(fid));
		}

		// 2. Lanzamientos y Otras animaciones
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		Dictionary enemy_d;
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[CAT_ENEMIES].size()) {
			enemy_d = category_data[CAT_ENEMIES][sel[0]];
		}
		Array casts = enemy_d.get("cast_animations", Array());
		for (int c = 0; c < casts.size(); c++) {
			if (casts[c].get_type() != Variant::DICTIONARY) continue;
			String cname = String(Dictionary(casts[c]).get("name", "")).strip_edges();
			if (!cname.is_empty() && !s_is_enemy_fixed_slot_name(cname)) {
				add_enemy_slot_item(cname, cname);
			}
		}
		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : current_sprite_data.animations) {
			bool already_added = false;
			for (int k = 0; k < anim_track_opt->get_item_count(); k++) {
				if (String(anim_track_opt->get_item_metadata(k)) == E.key) {
					already_added = true;
					break;
				}
			}
			if (!already_added) {
				add_enemy_slot_item(E.key, E.key);
			}
		}
	} else if (s_is_anim_slot_category(current_category)) {
		int move_dirs = (int(system_config.get("movement_directions", 4)) == 8) ? 8 : 4;
		bool dir_idle = actor_dir_idle_chk ? actor_dir_idle_chk->is_pressed() : false;
		int combat_mode = int(system_config.get("combat_mode", COMBAT_MODE_TURN_BASED));
		PackedStringArray fixed_slots = ClockGearsCharacterStateRegistry::get_category_slot_ids(current_category, move_dirs, dir_idle, combat_mode);
		auto add_actor_slot_item = [&](const String &p_key) {
			String disp = s_format_enemy_slot_display_label(p_key, p_key, current_sprite_data);
			anim_track_opt->add_item(disp, idx);
			anim_track_opt->set_item_metadata(idx, p_key);
			if (anim_item_list) {
				anim_item_list->add_item(disp);
				anim_item_list->set_item_metadata(idx, p_key);
			}
			if (p_key == prev_sel || (prev_sel.is_empty() && p_key == "mover_abajo")) {
				sel_idx = idx;
			}
			idx++;
		};
		for (int s = 0; s < fixed_slots.size(); s++) {
			add_actor_slot_item(fixed_slots[s]);
		}
		// Ranuras adicionales personalizadas o de combate se conservan intactas
		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : current_sprite_data.animations) {
			if (ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(E.key)) {
				continue;
			}
			add_actor_slot_item(E.key);
		}
	} else {
		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : current_sprite_data.animations) {
			String disp = s_format_enemy_slot_display_label(E.key, E.key, current_sprite_data);
			anim_track_opt->add_item(disp, idx);
			anim_track_opt->set_item_metadata(idx, E.key);
			if (anim_item_list) {
				anim_item_list->add_item(disp);
				anim_item_list->set_item_metadata(idx, E.key);
			}
			if (E.key == prev_sel || (prev_sel.is_empty() && E.key.to_lower() == "idle")) {
				sel_idx = idx;
			}
			idx++;
		}
	}

	if (anim_track_opt->get_item_count() > 0) {
		anim_track_opt->select(sel_idx);
		if (anim_item_list) {
			anim_item_list->select(sel_idx);
		}
		updating_ui = false;
		_on_anim_track_selected(sel_idx);
	} else {
		updating_ui = false;
		if (anim_fps_spin) anim_fps_spin->set_value(6);
		if (anim_loop_chk) anim_loop_chk->set_pressed(true);
		if (anim_random_order_chk) anim_random_order_chk->set_pressed(false);
		_refresh_frames_timeline();
		_refresh_anim_preview();
	}
	_refresh_combat_anims_dropdowns();
}

void ClockGearsDatabaseDialog::_on_anim_track_selected(int p_idx) {
	if (updating_ui) return;
	if (!anim_track_opt || p_idx < 0 || p_idx >= anim_track_opt->get_item_count()) {
		return;
	}
	Variant meta = anim_track_opt->get_item_metadata(p_idx);
	String anim_name = (meta.get_type() == Variant::STRING && !String(meta).is_empty())
			? String(meta)
			: anim_track_opt->get_item_text(p_idx);
	bool is_fixed_actor_slot = (s_is_anim_slot_category(current_category) && ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(anim_name));
	if (!current_sprite_data.animations.has(anim_name) && !is_fixed_actor_slot) {
		return;
	}

	updating_ui = true;
	anim_fps_line_edited = false;
	float fallback_fps = s_is_anim_slot_category(current_category) ? 8.0f : 6.0f;
	if (current_sprite_data.animations.has(anim_name)) {
		const ClockGearsSpriteAnimation &a = current_sprite_data.animations[anim_name];
		if (anim_fps_spin) anim_fps_spin->set_value(a.compute_average_fps(fallback_fps));
		if (anim_loop_chk) anim_loop_chk->set_pressed(a.loop);
		if (anim_random_order_chk) anim_random_order_chk->set_pressed(a.random_order);
		if (anim_behind_body_chk) anim_behind_body_chk->set_pressed(a.behind_body);
	} else {
		if (anim_fps_spin) anim_fps_spin->set_value(8.0f);
		if (anim_loop_chk) anim_loop_chk->set_pressed(true);
		if (anim_random_order_chk) anim_random_order_chk->set_pressed(false);
		if (anim_behind_body_chk) anim_behind_body_chk->set_pressed(false);
	}
	if (anim_item_list) {
		for (int i = 0; i < anim_item_list->get_item_count(); i++) {
			Variant item_meta = anim_item_list->get_item_metadata(i);
			String item_key = (item_meta.get_type() == Variant::STRING && !String(item_meta).is_empty())
					? String(item_meta)
					: anim_item_list->get_item_text(i);
			if (item_key == anim_name) {
				anim_item_list->select(i);
				break;
			}
		}
	}
	bool is_fixed_enemy_slot = (current_category == CAT_ENEMIES && s_is_enemy_fixed_slot_name(anim_name));
	if (btn_rename_anim) {
		btn_rename_anim->set_disabled(is_fixed_enemy_slot || is_fixed_actor_slot);
	}
	if (btn_remove_anim) {
		btn_remove_anim->set_disabled(is_fixed_enemy_slot);
	}

	sheet_importer_selected_frames.clear();
	anim_preview_playing = false;
	anim_preview_timer = 0.0f;
	anim_preview_frame_idx = 0;
	shape_frame_idx = 0;
	if (btn_anim_play) {
		btn_anim_play->set_text(CG_DB_TR("▶ Probar"));
	}
	updating_ui = false;

	_refresh_frames_timeline();
	_refresh_anim_preview();
	_refresh_shape_editor_context();
}

void ClockGearsDatabaseDialog::_on_anim_item_selected(int p_idx) {
	if (updating_ui) return;
	if (!anim_item_list || p_idx < 0 || p_idx >= anim_item_list->get_item_count()) {
		return;
	}
	Variant meta = anim_item_list->get_item_metadata(p_idx);
	String anim_name = (meta.get_type() == Variant::STRING && !String(meta).is_empty())
			? String(meta)
			: anim_item_list->get_item_text(p_idx);
	if (anim_track_opt) {
		for (int i = 0; i < anim_track_opt->get_item_count(); i++) {
			Variant tmeta = anim_track_opt->get_item_metadata(i);
			String tkey = (tmeta.get_type() == Variant::STRING && !String(tmeta).is_empty())
					? String(tmeta)
					: anim_track_opt->get_item_text(i);
			if (tkey == anim_name) {
				anim_track_opt->select(i);
				break;
			}
		}
		_on_anim_track_selected(anim_track_opt->get_selected());
	}
}

void ClockGearsDatabaseDialog::_on_anim_item_activated(int p_idx) {
	if (!anim_item_list || p_idx < 0 || p_idx >= anim_item_list->get_item_count()) return;
	Variant meta = anim_item_list->get_item_metadata(p_idx);
	String old_name = (meta.get_type() == Variant::STRING && !String(meta).is_empty())
			? String(meta)
			: anim_item_list->get_item_text(p_idx);
	if (current_category == CAT_ENEMIES && s_is_enemy_fixed_slot_name(old_name)) {
		return; // Las 5 ranuras fijas (idle, attack, hurt, death, buff) no se renombran
	}
	if (s_is_anim_slot_category(current_category) && ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(old_name)) {
		return; // Las ranuras direccionales fijas no se renombran
	}
	if (rename_anim_dialog && rename_anim_name_edit) {
		rename_anim_name_edit->set_text(old_name);
		rename_anim_name_edit->set_meta("old_anim_name", old_name);
		rename_anim_dialog->popup_centered();
		rename_anim_name_edit->grab_focus();
		rename_anim_name_edit->select_all();
	}
}

void ClockGearsDatabaseDialog::_on_add_anim_pressed() {
	if (!_has_valid_selected_record()) {
		return;
	}
	if (new_anim_dialog) {
		if (new_anim_name_edit) {
			new_anim_name_edit->set_text("");
			new_anim_name_edit->set_meta("is_enemy_cast_slot", current_category == CAT_ENEMIES);
		}
		new_anim_dialog->popup_centered();
	}
}

void ClockGearsDatabaseDialog::_on_new_anim_dialog_confirmed() {
	if (!_has_valid_selected_record()) {
		return;
	}
	bool is_cast = (current_category == CAT_ENEMIES) || (new_anim_name_edit && bool(new_anim_name_edit->get_meta("is_enemy_cast_slot", false)));
	if (new_anim_name_edit) {
		new_anim_name_edit->set_meta("is_enemy_cast_slot", false);
	}
	String anim_name = new_anim_name_edit ? new_anim_name_edit->get_text().strip_edges() : "";
	if (anim_name.is_empty() || (current_category == CAT_ENEMIES && s_is_enemy_fixed_slot_name(anim_name))) {
		int c = 1;
		String prefix = is_cast ? "Lanzar " : "anim_";
		anim_name = vformat("%s%d", prefix, c);
		while (current_sprite_data.animations.has(anim_name)) {
			c++;
			anim_name = vformat("%s%d", prefix, c);
		}
	}
	ClockGearsSpriteAnimation a;
	a.name = anim_name;
	a.fps = 6.0f;
	a.loop = !is_cast;
	a.frames_data.clear();
	a.frames.clear();
	current_sprite_data.animations[anim_name] = a;

	if (current_category == CAT_ENEMIES) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[CAT_ENEMIES].size()) {
			int rec_idx = sel[0];
			Dictionary d = category_data[CAT_ENEMIES][rec_idx];
			Array casts = d.get("cast_animations", Array());
			bool exists = false;
			for (int i = 0; i < casts.size(); i++) {
				Dictionary cd = casts[i];
				if (String(cd.get("name", "")) == anim_name) {
					exists = true;
					break;
				}
			}
			if (!exists && !s_is_enemy_fixed_slot_name(anim_name)) {
				Dictionary cd;
				cd["name"] = anim_name;
				cd["element_id"] = 0;
				cd["element"] = "";
				cd["element_key"] = "";
				casts.push_back(cd);
				d["cast_animations"] = casts;
				category_data[CAT_ENEMIES][rec_idx] = d;
			}
		}
		_refresh_enemy_casts_ui();
	}

	_refresh_anim_track_options();
	select_anim_slot(anim_name);
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_rename_anim_pressed() {
	if (!_has_valid_selected_record()) {
		return;
	}
	String cur_name = _get_active_anim_name();
	if (cur_name.is_empty()) return;
	if (current_category == CAT_ENEMIES && s_is_enemy_fixed_slot_name(cur_name)) {
		return;
	}
	if (s_is_anim_slot_category(current_category) && ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(cur_name)) {
		return;
	}
	if (rename_anim_dialog && rename_anim_name_edit) {
		rename_anim_name_edit->set_text(cur_name);
		rename_anim_name_edit->set_meta("old_anim_name", cur_name);
		rename_anim_dialog->popup_centered();
		rename_anim_name_edit->grab_focus();
		rename_anim_name_edit->select_all();
	}
}

void ClockGearsDatabaseDialog::_on_rename_anim_dialog_confirmed() {
	if (!rename_anim_name_edit) return;
	String old_name = rename_anim_name_edit->get_meta("old_anim_name", "");
	String new_name = rename_anim_name_edit->get_text().strip_edges();
	if (new_name.is_empty() || new_name == old_name) return;
	if (current_category == CAT_ENEMIES && (s_is_enemy_fixed_slot_name(old_name) || s_is_enemy_fixed_slot_name(new_name))) return;
	if (s_is_anim_slot_category(current_category) && (ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(old_name) || ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(new_name))) return;

	if (current_sprite_data.animations.has(old_name)) {
		ClockGearsSpriteAnimation a = current_sprite_data.animations[old_name];
		a.name = new_name;
		current_sprite_data.animations.erase(old_name);
		current_sprite_data.animations[new_name] = a;

		if (current_category == CAT_ENEMIES) {
			Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
			if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[CAT_ENEMIES].size()) {
				int rec_idx = sel[0];
				Dictionary d = category_data[CAT_ENEMIES][rec_idx];
				Array casts = d.get("cast_animations", Array());
				for (int i = 0; i < casts.size(); i++) {
					Dictionary cd = casts[i];
					if (String(cd.get("name", "")) == old_name) {
						cd["name"] = new_name;
						casts[i] = cd;
					}
				}
				d["cast_animations"] = casts;
				Array others = d.get("other_animations", Array());
				for (int i = 0; i < others.size(); i++) {
					if (others[i].get_type() == Variant::DICTIONARY) {
						Dictionary od = others[i];
						if (String(od.get("name", "")) == old_name) {
							od["name"] = new_name;
							others[i] = od;
						}
					} else if (String(others[i]) == old_name) {
						others[i] = new_name;
					}
				}
				d["other_animations"] = others;
				category_data[CAT_ENEMIES][rec_idx] = d;
				_refresh_enemy_casts_ui();
				_refresh_enemy_others_ui();
			}
		}

		_refresh_anim_track_options();
		select_anim_slot(new_name);
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_remove_anim_pressed() {
	if (!_has_valid_selected_record()) {
		return;
	}
	String name_to_remove = _get_active_anim_name();
	if (name_to_remove.is_empty() || !current_sprite_data.animations.has(name_to_remove)) {
		return;
	}
	if (current_category == CAT_ENEMIES && s_is_enemy_fixed_slot_name(name_to_remove)) {
		// Las 5 ranuras fijas del enemigo no se pueden borrar.
		return;
	}
	current_sprite_data.animations.erase(name_to_remove);
	if (s_is_anim_slot_category(current_category) && ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(name_to_remove)) {
		_refresh_anim_track_options();
		select_anim_slot(name_to_remove);
		_on_param_changed();
		return;
	}
	if (current_category == CAT_ENEMIES) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[CAT_ENEMIES].size()) {
			int rec_idx = sel[0];
			Dictionary d = category_data[CAT_ENEMIES][rec_idx];
			Array casts = d.get("cast_animations", Array());
			for (int i = casts.size() - 1; i >= 0; i--) {
				Dictionary cd = casts[i];
				if (String(cd.get("name", "")) == name_to_remove) {
					casts.remove_at(i);
				}
			}
			d["cast_animations"] = casts;
			Array others = d.get("other_animations", Array());
			for (int i = others.size() - 1; i >= 0; i--) {
				String oname = (others[i].get_type() == Variant::DICTIONARY)
						? String(Dictionary(others[i]).get("name", ""))
						: String(others[i]);
				if (oname == name_to_remove) {
					others.remove_at(i);
				}
			}
			d["other_animations"] = others;
			category_data[CAT_ENEMIES][rec_idx] = d;
			_refresh_enemy_casts_ui();
			_refresh_enemy_others_ui();
		}
	}
	_refresh_anim_track_options();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_open_sheet_importer_pressed() {
	if (!sheet_importer_dialog || !_has_valid_selected_record()) return;

	String cur_anim_name = _get_active_anim_name();
	String path = "";
	if (current_sprite_data.animations.has(cur_anim_name)) {
		path = current_sprite_data.animations[cur_anim_name].spritesheet_path;
	}
	if (path.is_empty()) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[current_category].size()) {
			Dictionary d = category_data[current_category][sel[0]];
			path = d.get("graphic_path", "");
		}
	}

	sheet_importer_selected_frames.clear();
	if (!path.is_empty()) {
		_on_sheet_importer_file_selected(path);
	} else if (sheet_importer_texture.is_null()) {
		if (sheet_importer_path_edit) sheet_importer_path_edit->set_text("");
		if (sheet_importer_info_lbl) sheet_importer_info_lbl->set_text(CG_DB_TR("Pulsa 'Explorar...' para cargar un spritesheet PNG"));
	}

	sheet_importer_dialog->popup_centered(Size2(720, 520) * EDSCALE);
}

void ClockGearsDatabaseDialog::_on_sheet_importer_browse_pressed() {
	if (!sheet_importer_file_dialog) return;
	String proj_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/battlers");
	if (proj_dir.is_empty()) {
		proj_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/characters");
	}
	if (!proj_dir.is_empty()) {
		sheet_importer_file_dialog->set_current_dir(proj_dir);
	}
	sheet_importer_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
}

void ClockGearsDatabaseDialog::_on_sheet_importer_file_selected(const String &p_path) {
	sheet_importer_cur_path = p_path;
	if (sheet_importer_path_edit) {
		sheet_importer_path_edit->set_text(p_path);
	}
	sheet_importer_texture = _load_texture_from_path(p_path);
	sheet_importer_selected_frames.clear();

	if (sheet_importer_texture.is_valid()) {
		int tex_w = sheet_importer_texture->get_width();
		int tex_h = sheet_importer_texture->get_height();

		int cols = 4;
		int rows = 1;
		if (tex_h > 0 && tex_w >= tex_h && tex_w % tex_h == 0) {
			cols = tex_w / tex_h;
			rows = 1;
		} else if (tex_w > 0 && tex_h >= tex_w && tex_h % tex_w == 0) {
			cols = 1;
			rows = tex_h / tex_w;
		}

		if (sheet_importer_cols_spin) sheet_importer_cols_spin->set_value(cols);
		if (sheet_importer_rows_spin) sheet_importer_rows_spin->set_value(rows);
		_on_sheet_importer_grid_changed();
	} else {
		if (sheet_importer_info_lbl) {
			sheet_importer_info_lbl->set_text(CG_DB_TR("No se pudo cargar la imagen."));
		}
	}
}

void ClockGearsDatabaseDialog::_on_sheet_importer_grid_changed(double p_unused) {
	if (sheet_importer_texture.is_null()) {
		if (sheet_importer_info_lbl) sheet_importer_info_lbl->set_text(CG_DB_TR("No hay imagen cargada."));
		return;
	}
	int tex_w = sheet_importer_texture->get_width();
	int tex_h = sheet_importer_texture->get_height();
	int cols = sheet_importer_cols_spin ? int(sheet_importer_cols_spin->get_value()) : 4;
	int rows = sheet_importer_rows_spin ? int(sheet_importer_rows_spin->get_value()) : 1;
	if (cols <= 0) cols = 1;
	if (rows <= 0) rows = 1;

	int fw = MAX(1, tex_w / cols);
	int fh = MAX(1, tex_h / rows);

	if (sheet_importer_info_lbl) {
		sheet_importer_info_lbl->set_text(vformat(CG_DB_TR("Tamaño de frame: %dx%d px | Imagen: %dx%d px (%d cols x %d filas)"), fw, fh, tex_w, tex_h, cols, rows));
	}
	if (sheet_importer_canvas) {
		sheet_importer_canvas->set_custom_minimum_size(Size2(tex_w, tex_h));
		sheet_importer_canvas->queue_redraw();
	}
	_update_sheet_importer_ui();
}

void ClockGearsDatabaseDialog::_update_sheet_importer_ui() {
	int count = sheet_importer_selected_frames.size();
	if (sheet_importer_status_lbl) {
		sheet_importer_status_lbl->set_text(vformat(CG_DB_TR("%d frame(s) seleccionado(s) (Arrastra o pulsa Ctrl+Clic para elegir frames)"), count));
	}
	if (sheet_importer_dialog && sheet_importer_dialog->get_ok_button()) {
		sheet_importer_dialog->get_ok_button()->set_text(count > 0 ? vformat(CG_DB_TR("Añadir %d frames"), count) : CG_DB_TR("Añadir frames"));
		sheet_importer_dialog->get_ok_button()->set_disabled(count == 0);
	}
	if (sheet_importer_canvas) {
		sheet_importer_canvas->queue_redraw();
	}
}

void ClockGearsDatabaseDialog::_on_sheet_importer_canvas_draw() {
	if (!sheet_importer_canvas || sheet_importer_texture.is_null()) return;
	int tex_w = sheet_importer_texture->get_width();
	int tex_h = sheet_importer_texture->get_height();
	int cols = sheet_importer_cols_spin ? int(sheet_importer_cols_spin->get_value()) : 4;
	int rows = sheet_importer_rows_spin ? int(sheet_importer_rows_spin->get_value()) : 1;
	if (cols <= 0 || rows <= 0 || tex_w <= 0 || tex_h <= 0) return;

	int fw = MAX(1, tex_w / cols);
	int fh = MAX(1, tex_h / rows);

	// Fondo oscuro pixel art
	sheet_importer_canvas->draw_rect(Rect2(0, 0, tex_w, tex_h), Color(0.1f, 0.12f, 0.16f, 1.0f), true);

	// Textura
	sheet_importer_canvas->draw_texture(sheet_importer_texture, Point2());

	// Rejilla y selecciones
	Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Label"));
	int font_sz = 10;

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			int idx = r * cols + c;
			Rect2 cell_rect(c * fw, r * fh, fw, fh);
			bool sel = (sheet_importer_selected_frames.find(idx) != -1);

			if (sel) {
				sheet_importer_canvas->draw_rect(cell_rect, Color(0.2f, 0.6f, 1.0f, 0.45f), true);
				sheet_importer_canvas->draw_rect(cell_rect, Color(0.4f, 0.85f, 1.0f, 0.95f), false, 2.0f);
			} else {
				sheet_importer_canvas->draw_rect(cell_rect, Color(0.8f, 0.8f, 0.8f, 0.25f), false, 1.0f);
			}

			if (font.is_valid()) {
				String label_str = itos(idx);
				Vector2 text_pos = cell_rect.position + Vector2(3, font_sz + 2);
				sheet_importer_canvas->draw_string(font, text_pos + Vector2(1, 1), label_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_sz, Color(0, 0, 0, 0.8f));
				sheet_importer_canvas->draw_string(font, text_pos, label_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_sz, sel ? Color(1.0f, 1.0f, 0.3f) : Color(1.0f, 1.0f, 1.0f, 0.9f));
			}
		}
	}
}

void ClockGearsDatabaseDialog::_on_sheet_importer_canvas_gui_input(const Ref<InputEvent> &p_ev) {
	Ref<InputEventMouseButton> mb = p_ev;
	Ref<InputEventMouseMotion> mm = p_ev;

	if (!sheet_importer_canvas || sheet_importer_texture.is_null()) return;
	int tex_w = sheet_importer_texture->get_width();
	int tex_h = sheet_importer_texture->get_height();
	int cols = sheet_importer_cols_spin ? int(sheet_importer_cols_spin->get_value()) : 4;
	int rows = sheet_importer_rows_spin ? int(sheet_importer_rows_spin->get_value()) : 1;
	if (cols <= 0 || rows <= 0 || tex_w <= 0 || tex_h <= 0) return;

	int fw = MAX(1, tex_w / cols);
	int fh = MAX(1, tex_h / rows);

	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		Vector2 pos = mb->get_position();
		int c = CLAMP(int(pos.x / fw), 0, cols - 1);
		int r = CLAMP(int(pos.y / fh), 0, rows - 1);
		int idx = r * cols + c;

		if (mb->is_pressed()) {
			sheet_importer_drag_active = true;
			sheet_importer_drag_start_c = c;
			sheet_importer_drag_start_r = r;

			bool ctrl = mb->is_ctrl_pressed();
			if (!ctrl) {
				sheet_importer_selected_frames.clear();
			}
			if (sheet_importer_selected_frames.find(idx) == -1) {
				sheet_importer_selected_frames.push_back(idx);
			} else if (ctrl) {
				sheet_importer_selected_frames.erase(idx);
			}
			_update_sheet_importer_ui();
		} else {
			sheet_importer_drag_active = false;
		}
	} else if (mm.is_valid() && sheet_importer_drag_active) {
		Vector2 pos = mm->get_position();
		int c = CLAMP(int(pos.x / fw), 0, cols - 1);
		int r = CLAMP(int(pos.y / fh), 0, rows - 1);

		int min_c = MIN(sheet_importer_drag_start_c, c);
		int max_c = MAX(sheet_importer_drag_start_c, c);
		int min_r = MIN(sheet_importer_drag_start_r, r);
		int max_r = MAX(sheet_importer_drag_start_r, r);

		bool ctrl = mm->is_ctrl_pressed();
		if (!ctrl) {
			sheet_importer_selected_frames.clear();
		}
		for (int row = min_r; row <= max_r; row++) {
			for (int col = min_c; col <= max_c; col++) {
				int f_idx = row * cols + col;
				if (sheet_importer_selected_frames.find(f_idx) == -1) {
					sheet_importer_selected_frames.push_back(f_idx);
				}
			}
		}
		_update_sheet_importer_ui();
	}
}

void ClockGearsDatabaseDialog::_on_sheet_importer_sel_all_pressed() {
	int cols = sheet_importer_cols_spin ? int(sheet_importer_cols_spin->get_value()) : 4;
	int rows = sheet_importer_rows_spin ? int(sheet_importer_rows_spin->get_value()) : 1;
	sheet_importer_selected_frames.clear();
	int total = cols * rows;
	for (int i = 0; i < total; i++) {
		sheet_importer_selected_frames.push_back(i);
	}
	_update_sheet_importer_ui();
}

void ClockGearsDatabaseDialog::_on_sheet_importer_clear_sel_pressed() {
	sheet_importer_selected_frames.clear();
	_update_sheet_importer_ui();
}

void ClockGearsDatabaseDialog::_on_sheet_importer_add_confirmed() {
	if (sheet_importer_selected_frames.is_empty()) return;
	String cur_anim_name = _get_active_anim_name();
	_ensure_actor_fixed_slot_exists(cur_anim_name);
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];

	int tex_w = sheet_importer_texture.is_valid() ? sheet_importer_texture->get_width() : 128;
	int tex_h = sheet_importer_texture.is_valid() ? sheet_importer_texture->get_height() : 32;
	int cols = sheet_importer_cols_spin ? int(sheet_importer_cols_spin->get_value()) : 4;
	int rows = sheet_importer_rows_spin ? int(sheet_importer_rows_spin->get_value()) : 1;
	int fw = MAX(1, tex_w / cols);
	int fh = MAX(1, tex_h / rows);

	for (int i = 0; i < sheet_importer_selected_frames.size(); i++) {
		int idx = sheet_importer_selected_frames[i];
		int c = idx % cols;
		int r = idx / cols;
		Rect2i reg(c * fw, r * fh, fw, fh);

		ClockGearsAnimationFrame frame;
		frame.sheet_path = sheet_importer_cur_path;
		frame.region = reg;
		a.frames_data.push_back(frame);
		a.frames.push_back(idx);
	}
	if (a.spritesheet_path.is_empty()) {
		a.spritesheet_path = sheet_importer_cur_path;
	}
	a.frame_w = fw;
	a.frame_h = fh;
	a.cols = cols;
	a.rows = rows;

	sheet_importer_dialog->hide();

	_update_current_anim_slot_labels();
	callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
	callable_mp(this, &ClockGearsDatabaseDialog::_refresh_anim_preview).call_deferred();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_refresh_frames_timeline() {
	if (!frames_strip_hbox) return;
	_refresh_shape_editor_context();
	while (frames_strip_hbox->get_child_count() > 0) {
		Node *c = frames_strip_hbox->get_child(0);
		frames_strip_hbox->remove_child(c);
		c->queue_free();
	}

	String cur_anim_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_anim_name)) {
		if (s_is_anim_slot_category(current_category) && ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(cur_anim_name)) {
			Label *empty_lbl = memnew(Label(CG_DB_TR("(Ranura vacía — usará respaldo automático o pulsa 'Añadir frames desde spritesheet')")));
			empty_lbl->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f));
			frames_strip_hbox->add_child(empty_lbl);
		}
		return;
	}
	const ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	int total_frames = a.get_frame_count();

	if (total_frames == 0) {
		Label *empty_lbl = memnew(Label(CG_DB_TR("(Animación vacía. Pulsa 'Añadir frames desde spritesheet')")));
		empty_lbl->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f));
		frames_strip_hbox->add_child(empty_lbl);
		return;
	}

	String fallback_sheet = a.spritesheet_path;
	if (fallback_sheet.is_empty()) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[current_category].size()) {
			Dictionary rec_d = category_data[current_category][sel[0]];
			fallback_sheet = rec_d.get("graphic_path", "");
		}
	}

	for (int s = 0; s < total_frames; s++) {
		ClockGearsAnimationFrame raw_f = a.get_frame(s, 0, 0);
		String spath = raw_f.sheet_path.is_empty() ? fallback_sheet : raw_f.sheet_path;
		Ref<Texture2D> tex = _load_texture_from_path(spath);
		int tw = tex.is_valid() ? tex->get_width() : 0;
		int th = tex.is_valid() ? tex->get_height() : 0;
		ClockGearsAnimationFrame frame = a.get_frame(s, tw, th);
		if (frame.sheet_path.is_empty()) {
			frame.sheet_path = spath;
		}
		PanelContainer *pnl = memnew(PanelContainer);
		pnl->set_custom_minimum_size(Size2(76 * EDSCALE, 168 * EDSCALE));

		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_anti_aliased(true);
		sb->set_bg_color(Color(0.14f, 0.16f, 0.22f, 0.95f));
		sb->set_border_width_all(1);
		sb->set_border_color(Color(0.35f, 0.4f, 0.55f, 0.8f));
		sb->set_corner_radius_all(6);
		pnl->add_theme_style_override("panel", sb);

		VBoxContainer *vb = memnew(VBoxContainer);
		vb->add_theme_constant_override("separation", 4);
		pnl->add_child(vb);

		Label *lbl = memnew(Label(vformat("#%d", s + 1)));
		lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl->add_theme_font_size_override("font_size", 10);
		vb->add_child(lbl);

		TextureRect *tr = memnew(TextureRect);
		tr->set_custom_minimum_size(Size2(64 * EDSCALE, 64 * EDSCALE));
		tr->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		tr->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		tr->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);

		if (tex.is_valid()) {
			Ref<AtlasTexture> at;
			at.instantiate();
			at->set_atlas(tex);
			Rect2 reg(frame.region);
			if (reg.size.x <= 0 || reg.size.y <= 0 || reg.position.x + reg.size.x > tw || reg.position.y + reg.size.y > th) {
				reg = Rect2(0, 0, tw, th);
			}
			at->set_region(reg);
			tr->set_texture(at);
		}
		vb->add_child(tr);

		HBoxContainer *btn_row = memnew(HBoxContainer);
		btn_row->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		btn_row->add_theme_constant_override("separation", 2);
		vb->add_child(btn_row);

		Button *btn_l = memnew(Button("<"));
		btn_l->set_disabled(s == 0);
		btn_l->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_move_left).bind(s));
		btn_row->add_child(btn_l);

		Button *btn_r = memnew(Button(">"));
		btn_r->set_disabled(s == total_frames - 1);
		btn_r->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_move_right).bind(s));
		btn_row->add_child(btn_r);

		Button *btn_x = memnew(Button("x"));
		btn_x->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_remove).bind(s));
		btn_row->add_child(btn_x);

		// Duración individual del frame
		HBoxContainer *dur_hb = memnew(HBoxContainer);
		dur_hb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		dur_hb->add_theme_constant_override("separation", 2);
		vb->add_child(dur_hb);

		Label *lbl_d = memnew(Label("s:"));
		lbl_d->add_theme_font_size_override("font_size", 9);
		dur_hb->add_child(lbl_d);

		SpinBox *sp_dur = memnew(SpinBox);
		sp_dur->set_min(0.01);
		sp_dur->set_max(60.0);
		sp_dur->set_step(0.01);
		sp_dur->set_custom_minimum_size(Size2(54 * EDSCALE, 0));
		float cur_dur = frame.duration > 0.001f ? frame.duration : (a.fps > 0 ? 1.0f / a.fps : 0.16f);
		sp_dur->set_value(cur_dur);
		_bind_numeric_spinbox(sp_dur);
		sp_dur->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_duration_changed).bind(s));
		dur_hb->add_child(sp_dur);

		// Opción Aleatorio por frame
		CheckBox *chk_rnd = memnew(CheckBox);
		chk_rnd->set_text(CG_DB_TR("🎲"));
		chk_rnd->set_tooltip_text(CG_DB_TR("Duración aleatoria entre Mín y Máx"));
		chk_rnd->set_pressed(frame.is_random_duration);
		chk_rnd->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_random_toggled).bind(s));
		vb->add_child(chk_rnd);

		if (frame.is_random_duration) {
			HBoxContainer *min_hb = memnew(HBoxContainer);
			min_hb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
			min_hb->add_theme_constant_override("separation", 2);
			vb->add_child(min_hb);

			Label *lbl_min = memnew(Label(CG_DB_TR("Mín:")));
			lbl_min->add_theme_font_size_override("font_size", 8);
			min_hb->add_child(lbl_min);

			SpinBox *sp_min = memnew(SpinBox);
			sp_min->set_min(0.01);
			sp_min->set_max(60.0);
			sp_min->set_step(0.01);
			sp_min->set_custom_minimum_size(Size2(48 * EDSCALE, 0));
			sp_min->set_value(frame.duration_min > 0.001f ? frame.duration_min : cur_dur);
			_bind_numeric_spinbox(sp_min);
			sp_min->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_min_dur_changed).bind(s));
			min_hb->add_child(sp_min);

			HBoxContainer *max_hb = memnew(HBoxContainer);
			max_hb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
			max_hb->add_theme_constant_override("separation", 2);
			vb->add_child(max_hb);

			Label *lbl_max = memnew(Label(CG_DB_TR("Máx:")));
			lbl_max->add_theme_font_size_override("font_size", 8);
			max_hb->add_child(lbl_max);

			SpinBox *sp_max = memnew(SpinBox);
			sp_max->set_min(0.01);
			sp_max->set_max(60.0);
			sp_max->set_step(0.01);
			sp_max->set_custom_minimum_size(Size2(48 * EDSCALE, 0));
			sp_max->set_value(frame.duration_max > 0.001f ? frame.duration_max : cur_dur * 1.5f);
			_bind_numeric_spinbox(sp_max);
			sp_max->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_frame_max_dur_changed).bind(s));
			max_hb->add_child(sp_max);
		}

		frames_strip_hbox->add_child(pnl);
	}
}

void ClockGearsDatabaseDialog::_refresh_anim_preview() {
	String cur_name = _get_active_anim_name();
	if (s_is_anim_slot_category(current_category)) {
		ClockGearsSpriteAnimation resolved_anim;
		bool has_anim = false;
		String state_id;
		int dir = 2;
		bool is_fixed_slot = s_parse_actor_fixed_slot_key(cur_name, state_id, dir);
		if (is_fixed_slot) {
			bool dir_idle = actor_dir_idle_chk ? actor_dir_idle_chk->is_pressed() : false;
			bool freeze_f0 = false;
			String resolved_slot;
			has_anim = ClockGearsCharacterStateRegistry::get_singleton() &&
					ClockGearsCharacterStateRegistry::get_singleton()->resolve_actor_animation(
							current_sprite_data, dir_idle, state_id, dir, resolved_anim, freeze_f0, resolved_slot);
		} else if (current_sprite_data.animations.has(cur_name)) {
			resolved_anim = current_sprite_data.animations[cur_name];
			has_anim = resolved_anim.get_frame_count() > 0;
		}

		if (has_anim && resolved_anim.get_frame_count() > 0) {
			ClockGearsAnimationFrame raw_f0 = resolved_anim.get_frame(0, 0, 0);
			String spath = raw_f0.sheet_path.is_empty() ? resolved_anim.spritesheet_path : raw_f0.sheet_path;
			if (spath.is_empty()) {
				Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
				if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[current_category].size()) {
					spath = Dictionary(category_data[current_category][sel[0]]).get("graphic_path", "");
				}
			}
			Ref<Texture2D> tex = _load_texture_from_path(spath);
			if (tex.is_valid()) {
				ClockGearsAnimationFrame f0 = resolved_anim.get_frame(0, tex->get_width(), tex->get_height());
				if (anim_preview_atlas.is_null()) anim_preview_atlas.instantiate();
				anim_preview_atlas->set_atlas(tex);
				Rect2 reg(f0.region);
				if (reg.size.x <= 0 || reg.size.y <= 0 || reg.position.x + reg.size.x > tex->get_width() || reg.position.y + reg.size.y > tex->get_height()) {
					reg = Rect2(0, 0, tex->get_width(), tex->get_height());
				}
				anim_preview_atlas->set_region(reg);
				if (anim_preview_rect) anim_preview_rect->set_texture(anim_preview_atlas);
				if (s_is_arpg_equip_category(current_category)) {
					_update_arpg_preview_canvas();
				}
				return;
			}
		} else {
			String spath;
			Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
			if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[current_category].size()) {
				spath = Dictionary(category_data[current_category][sel[0]]).get("graphic_path", "");
			}
			Ref<Texture2D> tex = _load_texture_from_path(spath);
			if (tex.is_valid() && is_fixed_slot) {
				int rows = (current_sprite_data.rows == 4 || current_sprite_data.rows >= 8)
						? current_sprite_data.rows
						: (tex->get_height() >= tex->get_width() * 2 ? 8 : 4);
				int cols = current_sprite_data.cols > 0 ? current_sprite_data.cols : 4;
				int fw = MAX(1, tex->get_width() / cols);
				int fh = MAX(1, tex->get_height() / rows);
				int row_idx = ClockGearsCharacterStateRegistry::direction_to_sheet_row(dir, rows);
				if (anim_preview_atlas.is_null()) anim_preview_atlas.instantiate();
				anim_preview_atlas->set_atlas(tex);
				anim_preview_atlas->set_region(Rect2(0, row_idx * fh, fw, fh));
				if (anim_preview_rect) anim_preview_rect->set_texture(anim_preview_atlas);
				if (s_is_arpg_equip_category(current_category)) {
					_update_arpg_preview_canvas();
				}
				return;
			}
		}

		anim_preview_playing = false;
		if (btn_anim_play) btn_anim_play->set_text(CG_DB_TR("▶ Probar"));
		if (btn_arpg_prev_play) btn_arpg_prev_play->set_text(CG_DB_TR("▶ Probar"));
		if (anim_preview_rect) anim_preview_rect->set_texture(ClockGearsCharacterStateRegistry::get_missing_question_marker_texture());
		if (s_is_arpg_equip_category(current_category)) {
			_update_arpg_preview_canvas();
		}
		return;
	}

	if (!current_sprite_data.animations.has(cur_name)) {
		if (anim_preview_rect) anim_preview_rect->set_texture(Ref<Texture2D>());
		return;
	}
	const ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	if (a.get_frame_count() == 0) {
		if (anim_preview_rect) anim_preview_rect->set_texture(Ref<Texture2D>());
		anim_preview_playing = false;
		if (btn_anim_play) btn_anim_play->set_text(CG_DB_TR("▶ Probar"));
		return;
	}
	ClockGearsAnimationFrame raw_f0 = a.get_frame(0, 0, 0);
	String spath = raw_f0.sheet_path.is_empty() ? a.spritesheet_path : raw_f0.sheet_path;
	if (spath.is_empty()) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[current_category].size()) {
			spath = Dictionary(category_data[current_category][sel[0]]).get("graphic_path", "");
		}
	}
	Ref<Texture2D> tex = _load_texture_from_path(spath);
	if (tex.is_valid()) {
		ClockGearsAnimationFrame f0 = a.get_frame(0, tex->get_width(), tex->get_height());
		if (anim_preview_atlas.is_null()) anim_preview_atlas.instantiate();
		anim_preview_atlas->set_atlas(tex);
		Rect2 reg(f0.region);
		if (reg.size.x <= 0 || reg.size.y <= 0 || reg.position.x + reg.size.x > tex->get_width() || reg.position.y + reg.size.y > tex->get_height()) {
			reg = Rect2(0, 0, tex->get_width(), tex->get_height());
		}
		anim_preview_atlas->set_region(reg);
		if (anim_preview_rect) anim_preview_rect->set_texture(anim_preview_atlas);
	} else {
		if (anim_preview_rect) anim_preview_rect->set_texture(Ref<Texture2D>());
	}
}

void ClockGearsDatabaseDialog::_on_frame_move_left(int p_step_idx) {
	String cur_anim_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	int total = a.get_frame_count();
	if (p_step_idx > 0 && p_step_idx < total) {
		if (p_step_idx < a.frames_data.size()) {
			ClockGearsAnimationFrame tmp = a.frames_data[p_step_idx];
			a.frames_data.write[p_step_idx] = a.frames_data[p_step_idx - 1];
			a.frames_data.write[p_step_idx - 1] = tmp;
		}
		if (p_step_idx < a.frames.size()) {
			int tmp = a.frames[p_step_idx];
			a.frames.write[p_step_idx] = a.frames[p_step_idx - 1];
			a.frames.write[p_step_idx - 1] = tmp;
		}
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_anim_preview).call_deferred();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_frame_move_right(int p_step_idx) {
	String cur_anim_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	int total = a.get_frame_count();
	if (p_step_idx >= 0 && p_step_idx < total - 1) {
		if (p_step_idx + 1 < a.frames_data.size()) {
			ClockGearsAnimationFrame tmp = a.frames_data[p_step_idx];
			a.frames_data.write[p_step_idx] = a.frames_data[p_step_idx + 1];
			a.frames_data.write[p_step_idx + 1] = tmp;
		}
		if (p_step_idx + 1 < a.frames.size()) {
			int tmp = a.frames[p_step_idx];
			a.frames.write[p_step_idx] = a.frames[p_step_idx + 1];
			a.frames.write[p_step_idx + 1] = tmp;
		}
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_anim_preview).call_deferred();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_frame_remove(int p_step_idx) {
	String cur_anim_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	int total = a.get_frame_count();
	if (p_step_idx >= 0 && p_step_idx < total) {
		if (p_step_idx < a.frames_data.size()) {
			a.frames_data.remove_at(p_step_idx);
		}
		if (p_step_idx < a.frames.size()) {
			a.frames.remove_at(p_step_idx);
		}
		_update_current_anim_slot_labels();
		bool prev_upd = updating_ui;
		updating_ui = true;
		anim_fps_line_edited = false;
		if (anim_fps_spin) {
			float fallback_fps = s_is_anim_slot_category(current_category) ? 8.0f : 6.0f;
			anim_fps_spin->set_value(a.compute_average_fps(fallback_fps));
		}
		updating_ui = prev_upd;
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_anim_preview).call_deferred();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_apply_anim_fps_to_current_slot() {
	if (updating_ui) return;
	anim_fps_line_edited = false;
	String cur_anim_name = _get_active_anim_name();
	_ensure_actor_fixed_slot_exists(cur_anim_name);
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	float new_fps = CLAMP(anim_fps_spin ? float(anim_fps_spin->get_value()) : a.fps, 1.0f, 60.0f);
	float target_dur = 1.0f / new_fps;
	bool already_applied = (Math::abs(a.fps - new_fps) <= 0.0001f);
	if (already_applied) {
		for (int i = 0; i < a.frames_data.size(); i++) {
			if (Math::abs(a.frames_data[i].duration - target_dur) > 0.0001f) {
				already_applied = false;
				break;
			}
		}
	}
	if (already_applied) return;
	a.apply_fps_to_all_frames(new_fps);
	if (a.get_frame_count() > 0) {
		int cur_idx = CLAMP(anim_preview_frame_idx, 0, a.get_frame_count() - 1);
		anim_preview_frame_target_dur = a.get_frame_duration(cur_idx);
	}
	_refresh_frames_timeline();
	_on_param_changed();
	if (s_is_arpg_equip_category(current_category)) {
		_update_arpg_preview_canvas();
	}
}

void ClockGearsDatabaseDialog::_on_anim_fps_value_changed(double p_val) {
	if (updating_ui) return;
	_apply_anim_fps_to_current_slot();
}

void ClockGearsDatabaseDialog::_on_anim_fps_text_changed(const String &p_text) {
	if (!updating_ui) {
		anim_fps_line_edited = true;
	}
}

void ClockGearsDatabaseDialog::_on_anim_fps_submitted(const String &p_text) {
	if (updating_ui) return;
	_apply_anim_fps_to_current_slot();
}

void ClockGearsDatabaseDialog::_on_anim_fps_focus_exited() {
	if (updating_ui || !anim_fps_line_edited) return;
	_apply_anim_fps_to_current_slot();
}

void ClockGearsDatabaseDialog::_on_anim_playback_settings_changed() {
	if (updating_ui) return;
	String cur_anim_name = _get_active_anim_name();
	_ensure_actor_fixed_slot_exists(cur_anim_name);
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	if (anim_loop_chk) a.loop = anim_loop_chk->is_pressed();
	if (anim_random_order_chk) a.random_order = anim_random_order_chk->is_pressed();
	if (anim_behind_body_chk) a.behind_body = anim_behind_body_chk->is_pressed();
	_on_param_changed();
	if (s_is_arpg_equip_category(current_category)) {
		_update_arpg_preview_canvas();
	}
}

void ClockGearsDatabaseDialog::_on_anim_play_pressed() {
	anim_preview_playing = !anim_preview_playing;
	anim_preview_timer = 0.0f;
	anim_preview_frame_idx = 0;
	String cur_name = _get_active_anim_name();
	if (current_sprite_data.animations.has(cur_name)) {
		const ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
		if (a.get_frame_count() > 0) {
			anim_preview_frame_target_dur = a.get_frame_duration(0);
		}
	} else {
		anim_preview_frame_target_dur = 0.125f;
	}
	if (btn_anim_play) {
		btn_anim_play->set_text(anim_preview_playing ? CG_DB_TR("⏹ Detener") : CG_DB_TR("▶ Probar"));
	}
	if (btn_arpg_prev_play) {
		btn_arpg_prev_play->set_text(anim_preview_playing ? CG_DB_TR("⏹ Detener") : CG_DB_TR("▶ Probar"));
	}
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_update_anim_playback(float p_delta) {
	if (!anim_preview_playing) return;
	String cur_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_name)) {
		if (s_is_arpg_equip_category(current_category)) {
			anim_preview_timer += p_delta;
			if (anim_preview_timer >= 0.125f) {
				anim_preview_timer = fmod(anim_preview_timer, 0.125f);
				anim_preview_frame_idx = (anim_preview_frame_idx + 1) % 256;
				_update_arpg_preview_canvas();
			}
		}
		return;
	}
	const ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	int total = a.get_frame_count();
	if (total == 0) {
		if (s_is_arpg_equip_category(current_category)) {
			anim_preview_timer += p_delta;
			if (anim_preview_timer >= 0.125f) {
				anim_preview_timer = fmod(anim_preview_timer, 0.125f);
				anim_preview_frame_idx = (anim_preview_frame_idx + 1) % 256;
				_update_arpg_preview_canvas();
			}
		}
		return;
	}

	if (anim_preview_frame_target_dur <= 0.001f) {
		anim_preview_frame_target_dur = a.get_frame_duration(CLAMP(anim_preview_frame_idx, 0, total - 1));
	}

	anim_preview_timer += p_delta;
	while (anim_preview_timer >= anim_preview_frame_target_dur && anim_preview_playing) {
		anim_preview_timer -= anim_preview_frame_target_dur;
		if (a.random_order && total > 1) {
			anim_preview_frame_idx = Math::rand() % total;
		} else {
			anim_preview_frame_idx++;
			if (anim_preview_frame_idx >= total) {
				if (a.loop) {
					anim_preview_frame_idx = 0;
				} else {
					anim_preview_frame_idx = total - 1;
					anim_preview_playing = false;
					if (btn_anim_play) {
						btn_anim_play->set_text(CG_DB_TR("▶ Probar"));
					}
					break;
				}
			}
		}
		anim_preview_frame_target_dur = a.get_frame_duration(anim_preview_frame_idx);
	}

	ClockGearsAnimationFrame raw_f = a.get_frame(anim_preview_frame_idx, 0, 0);
	String spath = raw_f.sheet_path.is_empty() ? a.spritesheet_path : raw_f.sheet_path;
	Ref<Texture2D> tex = _load_texture_from_path(spath);
	if (tex.is_valid()) {
		ClockGearsAnimationFrame f = a.get_frame(anim_preview_frame_idx, tex->get_width(), tex->get_height());
		if (anim_preview_atlas.is_null()) anim_preview_atlas.instantiate();
		if (anim_preview_atlas->get_atlas() != tex) {
			anim_preview_atlas->set_atlas(tex);
		}
		Rect2 reg(f.region);
		if (reg.size.x <= 0 || reg.size.y <= 0 || reg.position.x + reg.size.x > tex->get_width() || reg.position.y + reg.size.y > tex->get_height()) {
			reg = Rect2(0, 0, tex->get_width(), tex->get_height());
		}
		anim_preview_atlas->set_region(reg);
		if (anim_preview_rect) {
			anim_preview_rect->set_texture(anim_preview_atlas);
		}
	}
	_update_arpg_preview_canvas();
}

Dictionary ClockGearsDatabaseDialog::get_troop_member_preview_info(int p_troop_idx, int p_member_idx) const {
	Dictionary info;
	info["source"] = "";
	info["has_texture"] = false;
	info["texture_w"] = 0;
	info["texture_h"] = 0;
	info["region"] = Rect2();
	info["enemy_id"] = 0;
	info["enemy_name"] = "";
	info["enemy_level"] = 1;
	if (p_troop_idx < 0 || p_troop_idx >= category_data[CAT_TROOPS].size()) {
		return info;
	}
	Dictionary td = category_data[CAT_TROOPS][p_troop_idx];
	Array members = td.get("members", Array());
	if (p_member_idx < 0 || p_member_idx >= members.size() || members[p_member_idx].get_type() != Variant::DICTIONARY) {
		return info;
	}
	Dictionary m = members[p_member_idx];
	int eid = m.get("enemy_id", 1);
	info["enemy_id"] = eid;

	Dictionary enemy_rec;
	for (int k = 0; k < category_data[CAT_ENEMIES].size(); k++) {
		Dictionary ed = category_data[CAT_ENEMIES][k];
		if (int(ed.get("id", k + 1)) == eid) {
			enemy_rec = ed.duplicate(true);
			break;
		}
	}
	if (enemy_rec.is_empty()) {
		enemy_rec["name"] = vformat("Enemigo #%d", eid);
		enemy_rec["level"] = 1;
	}
	info["enemy_name"] = String(enemy_rec.get("name", vformat("Enemigo #%d", eid)));
	info["enemy_level"] = int(enemy_rec.get("level", 1));

	Ref<Texture2D> tex;
	Rect2 src_rect;
	String src_mode;
	ClockGearsBattleScene::resolve_enemy_preview_sprite(enemy_rec, tex, src_rect, src_mode);
	info["source"] = src_mode;
	info["has_texture"] = tex.is_valid();
	if (tex.is_valid()) {
		info["texture_w"] = tex->get_width();
		info["texture_h"] = tex->get_height();
		info["texture"] = tex;
	}
	info["region"] = src_rect;
	return info;
}

Rect2 ClockGearsDatabaseDialog::get_troop_preview_viewport_rect() const {
	if (!troop_placement_canvas) {
		return Rect2();
	}
	return s_calc_troop_vp_rect(troop_placement_canvas);
}

Dictionary ClockGearsDatabaseDialog::get_troop_preview_enemy_layout(int p_member_idx) const {
	Dictionary out;
	if (!troop_placement_canvas) {
		return out;
	}
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[CAT_TROOPS].size()) {
		return out;
	}
	Dictionary td = category_data[CAT_TROOPS][sel[0]];
	Array mems = td.get("members", Array());
	if (p_member_idx < 0 || p_member_idx >= mems.size() || mems[p_member_idx].get_type() != Variant::DICTIONARY) {
		return out;
	}
	Dictionary m = mems[p_member_idx];
	Vector2 mpos(float(m.get("x", 200 + p_member_idx * 110)), float(m.get("y", 260)));
	Size2 src_sz = s_resolve_troop_member_src_size(m, category_data[CAT_ENEMIES]);
	int m_scale = s_get_troop_member_scale(m);
	String hp_mode = s_get_troop_member_hp_bar_mode(m);
	Vector2i hp_pos = s_get_troop_member_hp_bar_pos(m);
	Rect2 vp_rect = s_calc_troop_vp_rect(troop_placement_canvas);
	Ref<Font> font = get_theme_default_font();
	ClockGearsBattleScene::EnemyVisualLayout lay = ClockGearsBattleScene::compute_enemy_visual_layout(mpos, vp_rect, src_sz, m_scale, font, hp_mode, hp_pos);
	out["sprite_rect"] = lay.draw_spr_rect;
	out["hud_group_rect"] = lay.hud_group_box;
	out["hp_rect"] = lay.hp_rect;
	out["hp_visible"] = lay.hp_bar_visible;
	out["label_baseline_y"] = lay.label_baseline_y;
	out["hp_bar_mode"] = lay.hp_bar_mode;
	out["hp_bar_pos"] = hp_pos;
	out["hud_highlighted"] = (troop_hp_bar_dragging && p_member_idx == selected_troop_member_idx);
	return out;
}

Dictionary ClockGearsDatabaseDialog::get_enemy_anim_slot_info(int p_enemy_idx, const String &p_slot_key) const {
	Dictionary info;
	info["exists"] = false;
	info["frame_count"] = 0;
	info["fps"] = 6.0f;
	info["loop"] = false;
	info["spritesheet_path"] = "";
	if (p_enemy_idx < 0 || p_enemy_idx >= category_data[CAT_ENEMIES].size()) {
		return info;
	}
	Dictionary ed = Dictionary(category_data[CAT_ENEMIES][p_enemy_idx]).duplicate(true);
	ClockGearsSpriteSheetData::migrate_enemy_record(ed);
	ClockGearsSpriteSheetData sd;
	sd.from_dict(ed.get("sprite_data", Dictionary()));
	if (sd.animations.has(p_slot_key)) {
		const ClockGearsSpriteAnimation &a = sd.animations[p_slot_key];
		info["exists"] = true;
		info["frame_count"] = a.get_frame_count();
		info["fps"] = a.fps;
		info["loop"] = a.loop;
		info["random_order"] = a.random_order;
		info["spritesheet_path"] = a.spritesheet_path;
	}
	return info;
}

bool ClockGearsDatabaseDialog::reassign_enemy_other_animation(int p_enemy_idx, const String &p_other_name, const String &p_target_slot) {
	if (p_enemy_idx < 0 || p_enemy_idx >= category_data[CAT_ENEMIES].size()) {
		return false;
	}
	String oname = p_other_name.strip_edges();
	String target = p_target_slot.strip_edges();
	if (oname.is_empty() || target.is_empty()) {
		return false;
	}
	Dictionary d = Dictionary(category_data[CAT_ENEMIES][p_enemy_idx]).duplicate(true);
	ClockGearsSpriteSheetData::migrate_enemy_record(d);
	ClockGearsSpriteSheetData sd;
	sd.from_dict(d.get("sprite_data", Dictionary()));
	if (!sd.animations.has(oname)) {
		return false;
	}

	ClockGearsSpriteAnimation moved = sd.animations[oname];
	sd.animations.erase(oname);
	moved.name = target;
	moved.loop = (target == "idle");
	sd.animations[target] = moved;

	// Quitar de other_animations
	Array others = d.get("other_animations", Array());
	for (int i = others.size() - 1; i >= 0; i--) {
		String cur = (others[i].get_type() == Variant::DICTIONARY)
				? String(Dictionary(others[i]).get("name", "")).strip_edges()
				: String(others[i]).strip_edges();
		if (cur == oname) {
			others.remove_at(i);
		}
	}
	d["other_animations"] = others;

	// Si el destino no es una de las 5 ranuras fijas, asegurar que esté en cast_animations
	if (!s_is_enemy_fixed_slot_name(target)) {
		Array casts = d.get("cast_animations", Array());
		bool found = false;
		for (int i = 0; i < casts.size(); i++) {
			if (casts[i].get_type() == Variant::DICTIONARY && String(Dictionary(casts[i]).get("name", "")) == target) {
				found = true;
				break;
			}
		}
		if (!found) {
			Dictionary cd;
			cd["name"] = target;
			cd["element_id"] = 0;
			cd["element"] = "";
			cd["element_key"] = "";
			casts.push_back(cd);
			d["cast_animations"] = casts;
		}
	}

	d["sprite_data"] = sd.to_dict();
	category_data[CAT_ENEMIES][p_enemy_idx] = d;

	if (current_category == CAT_ENEMIES) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] == p_enemy_idx) {
			current_sprite_data = sd;
			_refresh_enemy_casts_ui();
			_refresh_enemy_others_ui();
			_refresh_anim_track_options();
			select_anim_slot(target);
		}
		_update_record_list_item_icon(p_enemy_idx);
	}
	_mark_database_dirty();
	return true;
}

bool ClockGearsDatabaseDialog::remove_enemy_other_animation(int p_enemy_idx, const String &p_other_name) {
	if (p_enemy_idx < 0 || p_enemy_idx >= category_data[CAT_ENEMIES].size()) {
		return false;
	}
	String oname = p_other_name.strip_edges();
	if (oname.is_empty() || s_is_enemy_fixed_slot_name(oname)) {
		return false;
	}
	Dictionary d = Dictionary(category_data[CAT_ENEMIES][p_enemy_idx]).duplicate(true);
	ClockGearsSpriteSheetData::migrate_enemy_record(d);
	ClockGearsSpriteSheetData sd;
	sd.from_dict(d.get("sprite_data", Dictionary()));
	sd.animations.erase(oname);

	Array others = d.get("other_animations", Array());
	for (int i = others.size() - 1; i >= 0; i--) {
		String cur = (others[i].get_type() == Variant::DICTIONARY)
				? String(Dictionary(others[i]).get("name", "")).strip_edges()
				: String(others[i]).strip_edges();
		if (cur == oname) {
			others.remove_at(i);
		}
	}
	d["other_animations"] = others;
	d["sprite_data"] = sd.to_dict();
	category_data[CAT_ENEMIES][p_enemy_idx] = d;

	if (current_category == CAT_ENEMIES) {
		Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
		if (!sel.is_empty() && sel[0] == p_enemy_idx) {
			current_sprite_data = sd;
			_refresh_enemy_others_ui();
			_refresh_anim_track_options();
		}
		_update_record_list_item_icon(p_enemy_idx);
	}
	_mark_database_dirty();
	return true;
}

void ClockGearsDatabaseDialog::select_anim_slot(const String &p_slot_key) {
	if (!anim_track_opt) return;
	for (int i = 0; i < anim_track_opt->get_item_count(); i++) {
		Variant meta = anim_track_opt->get_item_metadata(i);
		String key = (meta.get_type() == Variant::STRING && !String(meta).is_empty())
				? String(meta)
				: anim_track_opt->get_item_text(i);
		if (key == p_slot_key) {
			anim_track_opt->select(i);
			_on_anim_track_selected(i);
			break;
		}
	}
}

void ClockGearsDatabaseDialog::remove_anim_frame_at(int p_step_idx) {
	_on_frame_remove(p_step_idx);
}

void ClockGearsDatabaseDialog::set_system_movement_directions(int p_dirs) {
	int dirs = (p_dirs == 8) ? 8 : 4;
	system_config["movement_directions"] = dirs;
	if (ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager::get_singleton()->set_system_config(system_config);
	}
	if (sys_movement_dirs_opt) {
		bool prev_upd = updating_ui;
		updating_ui = true;
		sys_movement_dirs_opt->select(dirs == 8 ? 1 : 0);
		updating_ui = prev_upd;
	}
	if (s_is_anim_slot_category(current_category)) {
		_refresh_anim_track_options();
	}
	_mark_database_dirty();
}

int ClockGearsDatabaseDialog::get_system_movement_directions() const {
	return (int(system_config.get("movement_directions", 4)) == 8) ? 8 : 4;
}

void ClockGearsDatabaseDialog::set_actor_directional_idle(bool p_enabled) {
	if (actor_dir_idle_chk) {
		bool prev_upd = updating_ui;
		updating_ui = true;
		actor_dir_idle_chk->set_pressed(p_enabled);
		updating_ui = prev_upd;
	}
	if (s_is_anim_slot_category(current_category)) {
		_on_param_changed();
		_refresh_anim_track_options();
	}
}

bool ClockGearsDatabaseDialog::is_actor_directional_idle() const {
	if (actor_dir_idle_chk) {
		return actor_dir_idle_chk->is_pressed();
	}
	return false;
}

void ClockGearsDatabaseDialog::_setup_combat_anims_ui() {
	combat_anims_box = memnew(VBoxContainer);
	combat_anims_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	combat_anims_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	sprite_anim_editor_box->add_child(combat_anims_box);

	combat_anims_box->add_child(memnew(HSeparator));
	Label *lbl_title = memnew(Label(CG_DB_TR("Animaciones de combate")));
	lbl_title->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	combat_anims_box->add_child(lbl_title);

	GridContainer *ca_grid = memnew(GridContainer);
	ca_grid->set_columns(4);
	ca_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ca_grid->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	ca_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	combat_anims_box->add_child(ca_grid);

	// 1. Reposo (Idle)
	ca_grid->add_child(memnew(Label(CG_DB_TR("Reposo (Idle):"))));
	opt_anim_idle = memnew(OptionButton);
	opt_anim_idle->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_anim_idle->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_anim_selected));
	ca_grid->add_child(opt_anim_idle);

	// 2. Ataque
	ca_grid->add_child(memnew(Label(CG_DB_TR("Ataque:"))));
	opt_anim_attack = memnew(OptionButton);
	opt_anim_attack->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_anim_attack->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_anim_selected));
	ca_grid->add_child(opt_anim_attack);

	// 3. Daño recibido
	ca_grid->add_child(memnew(Label(CG_DB_TR("Daño recibido:"))));
	opt_anim_damage = memnew(OptionButton);
	opt_anim_damage->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_anim_damage->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_anim_selected));
	ca_grid->add_child(opt_anim_damage);

	// 4. Muerte
	ca_grid->add_child(memnew(Label(CG_DB_TR("Muerte:"))));
	opt_anim_death = memnew(OptionButton);
	opt_anim_death->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_anim_death->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_anim_selected));
	ca_grid->add_child(opt_anim_death);

	// 5. Defender
	ca_grid->add_child(memnew(Label(CG_DB_TR("Defender:"))));
	opt_anim_defend = memnew(OptionButton);
	opt_anim_defend->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_anim_defend->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_anim_selected));
	ca_grid->add_child(opt_anim_defend);

	// 6. Victoria (solo Actores)
	lbl_anim_victory = memnew(Label(CG_DB_TR("Victoria:")));
	ca_grid->add_child(lbl_anim_victory);
	opt_anim_victory = memnew(OptionButton);
	opt_anim_victory->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	opt_anim_victory->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_anim_selected));
	ca_grid->add_child(opt_anim_victory);
}

void ClockGearsDatabaseDialog::_refresh_combat_anims_dropdowns() {
	if (!opt_anim_idle || !opt_anim_attack || !opt_anim_damage || !opt_anim_death || !opt_anim_defend) {
		return;
	}

	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	Dictionary d;
	if (!sel.is_empty() && sel[0] >= 0 && sel[0] < category_data[current_category].size()) {
		d = category_data[current_category][sel[0]];
	}
	Dictionary ca = d.get("combat_animations", Dictionary());
	Dictionary tt = d.get("trigger_table", Dictionary());

	OptionButton *opts[] = { opt_anim_idle, opt_anim_attack, opt_anim_damage, opt_anim_death, opt_anim_defend, opt_anim_victory };
	const char *keys[] = { "idle", "attack", "damage", "death", "defend", "victory" };
	int triggers[] = { ANIM_TRIGGER_IDLE, ANIM_TRIGGER_ATTACK, ANIM_TRIGGER_DAMAGE, ANIM_TRIGGER_DEATH, ANIM_TRIGGER_DEFEND, ANIM_TRIGGER_VICTORY };

	for (int i = 0; i < 6; i++) {
		if (!opts[i]) continue;
		opts[i]->clear();
		opts[i]->add_item(CG_DB_TR("(ninguna - frame 0)"), 0);

		int item_idx = 1;
		int to_select = 0;
		String target_anim = String(ca.get(keys[i], "")).strip_edges();
		if (target_anim.is_empty() && tt.has(itos(triggers[i]))) {
			target_anim = String(tt.get(itos(triggers[i]), "")).strip_edges();
		}

		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : current_sprite_data.animations) {
			opts[i]->add_item(E.key, item_idx);
			if (!target_anim.is_empty() && E.key.to_lower() == target_anim.to_lower()) {
				to_select = item_idx;
			}
			item_idx++;
		}
		opts[i]->select(to_select);
	}

	if (lbl_anim_victory && opt_anim_victory) {
		bool is_act = (current_category == CAT_ACTORS);
		lbl_anim_victory->set_visible(is_act);
		opt_anim_victory->set_visible(is_act);
	}
}

void ClockGearsDatabaseDialog::_on_combat_anim_selected(int p_unused) {
	if (updating_ui) return;
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_frame_duration_changed(double p_val, int p_idx) {
	if (updating_ui) return;
	String cur_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	if (p_idx >= 0 && p_idx < a.frames_data.size()) {
		a.frames_data.write[p_idx].duration = (float)p_val;
		bool prev_upd = updating_ui;
		updating_ui = true;
		anim_fps_line_edited = false;
		if (anim_fps_spin) {
			float fallback_fps = s_is_anim_slot_category(current_category) ? 8.0f : 6.0f;
			anim_fps_spin->set_value(a.compute_average_fps(fallback_fps));
		}
		updating_ui = prev_upd;
		if (anim_preview_playing && anim_preview_frame_idx == p_idx) {
			anim_preview_frame_target_dur = a.get_frame_duration(p_idx);
		}
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_frame_random_toggled(bool p_pressed, int p_idx) {
	if (updating_ui) return;
	String cur_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	if (p_idx >= 0 && p_idx < a.frames_data.size()) {
		a.frames_data.write[p_idx].is_random_duration = p_pressed;
		_on_param_changed();
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
	}
}

void ClockGearsDatabaseDialog::_on_frame_min_dur_changed(double p_val, int p_idx) {
	if (updating_ui) return;
	String cur_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	if (p_idx >= 0 && p_idx < a.frames_data.size()) {
		a.frames_data.write[p_idx].duration_min = (float)p_val;
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_frame_max_dur_changed(double p_val, int p_idx) {
	if (updating_ui) return;
	String cur_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	if (p_idx >= 0 && p_idx < a.frames_data.size()) {
		a.frames_data.write[p_idx].duration_max = (float)p_val;
		_on_param_changed();
	}
}

Dictionary ClockGearsDatabaseDialog::get_troop_battleback_cfg(int p_troop_idx, int p_slot) const {
	if (p_troop_idx < 0 || p_troop_idx >= category_data[CAT_TROOPS].size()) {
		return ClockGearsBattleScene::default_battleback_cfg(p_slot != 1);
	}
	Dictionary d = category_data[CAT_TROOPS][p_troop_idx];
	String key_cfg = (p_slot == 1) ? "battleback1_cfg" : "battleback2_cfg";
	Variant rv = d.get(key_cfg, Dictionary());
	Dictionary cfg = (rv.get_type() == Variant::DICTIONARY) ? Dictionary(rv) : Dictionary();
	return ClockGearsBattleScene::normalize_battleback_cfg(cfg, p_slot != 1);
}

void ClockGearsDatabaseDialog::set_troop_battleback_cfg(int p_troop_idx, int p_slot, const Dictionary &p_cfg) {
	Array &arr = category_data[CAT_TROOPS];
	if (p_troop_idx < 0 || p_troop_idx >= arr.size()) return;
	Dictionary d = arr[p_troop_idx];
	String key_cfg = (p_slot == 1) ? "battleback1_cfg" : "battleback2_cfg";
	d[key_cfg] = ClockGearsBattleScene::normalize_battleback_cfg(p_cfg, p_slot != 1);
	arr[p_troop_idx] = d;
	_save_database_to_disk();
	_queue_troop_preview_redraw();
}

String ClockGearsDatabaseDialog::get_formatted_troop_enemy_label(int p_troop_idx, int p_member_idx, float p_max_w) const {
	if (p_troop_idx < 0 || p_troop_idx >= category_data[CAT_TROOPS].size()) return String();
	Dictionary d = category_data[CAT_TROOPS][p_troop_idx];
	Array mems = d.get("members", Array());
	if (p_member_idx < 0 || p_member_idx >= mems.size()) return String();
	Dictionary m = mems[p_member_idx];
	int eid = int(m.get("enemy_id", 1));
	const Array &enemies = category_data[CAT_ENEMIES];
	Dictionary ed;
	for (int k = 0; k < enemies.size(); k++) {
		Dictionary cand = enemies[k];
		if (int(cand.get("id", k + 1)) == eid) {
			ed = cand;
			break;
		}
	}
	String ename = ed.get("name", vformat("Enemigo %d", eid));
	int elvl = MAX(1, int(ed.get("level", 1)));
	Ref<Font> font = get_theme_default_font();
	float scale = 1.0f;
	if (troop_placement_canvas) {
		Rect2 vp_rect = s_calc_troop_vp_rect(troop_placement_canvas);
		if (vp_rect.size.x > 1.0f) {
			scale = vp_rect.size.x / 960.0f;
		}
	}
	int font_sz = MAX(9, int(12 * scale));
	float max_w = (p_max_w > 0.0f) ? p_max_w : MAX(96.0f * scale, 140.0f * scale);
	return ClockGearsBattleScene::format_enemy_name_level(font, font_sz, ename, elvl, max_w);
}

void ClockGearsDatabaseDialog::_refresh_event_tab_summary() {
	if (!event_tab_summary_label || !event_tab_pages_tree) {
		return;
	}
	event_tab_pages_tree->clear();
	TreeItem *root = event_tab_pages_tree->create_item();
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (!s_has_event_tab_category(current_category) || sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[current_category].size()) {
		event_tab_summary_label->set_text(CG_DB_TR("0 páginas de evento configuradas."));
		if (btn_open_record_event_editor) {
			btn_open_record_event_editor->set_disabled(true);
		}
		return;
	}
	if (btn_open_record_event_editor) {
		btn_open_record_event_editor->set_disabled(false);
	}
	Dictionary d = category_data[current_category][sel[0]];
	Array pages = d.get("event_pages", Array());
	int count = pages.size();
	event_tab_summary_label->set_text(count == 1
					? CG_DB_TR("1 página de evento configurada (compartida con eventos de mapa).")
					: vformat(CG_DB_TR("%d páginas de evento configuradas (compartidas con eventos de mapa)."), count));

	static const char *trigger_names[5] = {
		"Botón de Acción",
		"Tocar Jugador",
		"Tocar Evento",
		"Inicio Automático",
		"Proceso Paralelo"
	};
	for (int i = 0; i < count; i++) {
		Variant pv = pages[i];
		if (pv.get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary pd = pv;
		int trig = CLAMP(int(pd.get("trigger", 0)), 0, 4);
		int cmds_count = Array(pd.get("commands", Array())).size();
		TreeItem *item = event_tab_pages_tree->create_item(root);
		item->set_text(0, vformat(CG_DB_TR("Página %d"), i + 1));
		item->set_text(1, CG_DB_TR(trigger_names[trig]));
		item->set_text(2, vformat(CG_DB_TR("%d comandos"), cmds_count));
	}
}

void ClockGearsDatabaseDialog::_on_open_record_event_editor_pressed() {
	open_record_event_editor();
}

void ClockGearsDatabaseDialog::_on_record_event_saved(const Array &p_pages) {
	int target_cat = current_category;
	int target_idx = -1;
	if (record_event_editor) {
		if (record_event_editor->has_meta("cg_db_event_cat")) {
			target_cat = int(record_event_editor->get_meta("cg_db_event_cat"));
		}
		if (record_event_editor->has_meta("cg_db_event_idx")) {
			target_idx = int(record_event_editor->get_meta("cg_db_event_idx"));
		}
	}
	if (target_idx < 0 && record_list) {
		Vector<int> sel = record_list->get_selected_items();
		if (!sel.is_empty()) {
			target_idx = sel[0];
		}
	}
	if (target_cat < 0 || target_cat >= CAT_MAX || target_idx < 0 || target_idx >= category_data[target_cat].size()) {
		return;
	}
	Dictionary d = category_data[target_cat][target_idx];
	d["event_pages"] = p_pages.duplicate(true);
	category_data[target_cat][target_idx] = d;
	if (target_cat == current_category) {
		_refresh_event_tab_summary();
		_refresh_zone_action_detail_ui();
	}
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_ensure_record_event_editor() {
	if (!record_event_editor) {
		record_event_editor = memnew(ClockGearsEventEditorDialog);
		add_child(record_event_editor);
	}
}

bool ClockGearsDatabaseDialog::open_record_event_editor() {
	_ensure_record_event_editor();
	if (!s_has_event_tab_category(current_category) || !record_event_editor || !record_list) {
		return false;
	}
	Vector<int> sel = record_list->get_selected_items();
	if (sel.is_empty() || sel[0] < 0 || sel[0] >= category_data[current_category].size()) {
		return false;
	}
	int idx = sel[0];
	Dictionary d = category_data[current_category][idx];
	int rec_id = int(d.get("id", idx + 1));
	String rec_name = String(d.get("name", ""));
	Array pages = d.get("event_pages", Array());
	String cat_key = ClockGearsDataManager::get_category_key(current_category);

	record_event_editor->set_meta("cg_db_event_cat", current_category);
	record_event_editor->set_meta("cg_db_event_idx", idx);
	record_event_editor->edit_database_event(cat_key, rec_id, rec_name, pages,
			callable_mp(this, &ClockGearsDatabaseDialog::_on_record_event_saved));
	return true;
}

Dictionary ClockGearsDatabaseDialog::get_chapter_structure_info() const {
	Dictionary out;
	Array chaps;
	Dictionary tooltips;
	for (int i = 0; i < chapters.size(); i++) {
		Dictionary cd;
		cd["name"] = chapters[i].name;
		Array cat_keys;
		Array cat_names;
		Array cat_ids;
		for (int j = 0; j < chapters[i].categories.size(); j++) {
			int cid = chapters[i].categories[j];
			String ckey;
			if (cid == DB_CAT_MAIN_MENU) {
				ckey = "main_menu";
			} else if (cid == DB_CAT_WINDOW_STYLES) {
				ckey = "window_styles";
			} else if (cid == DB_CAT_GAME_MENU) {
				ckey = "game_menu";
			} else if (cid == DB_CAT_CREDITS) {
				ckey = "credits";
			} else if (cid == DB_CAT_HUD) {
				ckey = "hud";
			} else if (cid == DB_CAT_UI_DESIGNER) {
				ckey = "ui_designer";
			} else if (cid == DB_CAT_GAME_OVER) {
				ckey = "game_over";
			} else {
				ckey = ClockGearsDataManager::get_category_key(cid);
			}
			cat_ids.push_back(cid);
			cat_keys.push_back(ckey);
			cat_names.push_back(_get_category_name(cid));
			if (cid >= 0 && cid < category_buttons.size() && category_buttons[cid]) {
				tooltips[ckey] = category_buttons[cid]->get_tooltip_text();
			}
		}
		cd["categories"] = cat_keys;
		cd["category_ids"] = cat_ids;
		cd["category_keys"] = cat_keys;
		cd["category_names"] = cat_names;
		chaps.push_back(cd);
	}
	out["chapters"] = chaps;
	out["tooltips"] = tooltips;
	out["current_chapter"] = current_chapter;
	out["current_category"] = current_category;
	if (current_category == DB_CAT_MAIN_MENU) {
		out["current_category_key"] = String("main_menu");
	} else if (current_category == DB_CAT_WINDOW_STYLES) {
		out["current_category_key"] = String("window_styles");
	} else if (current_category == DB_CAT_GAME_MENU) {
		out["current_category_key"] = String("game_menu");
	} else if (current_category == DB_CAT_CREDITS) {
		out["current_category_key"] = String("credits");
	} else if (current_category == DB_CAT_HUD) {
		out["current_category_key"] = String("hud");
	} else if (current_category == DB_CAT_UI_DESIGNER) {
		out["current_category_key"] = String("ui_designer");
	} else if (current_category == DB_CAT_GAME_OVER) {
		out["current_category_key"] = String("game_over");
	} else {
		out["current_category_key"] = ClockGearsDataManager::get_category_key(current_category);
	}
	out["category_count"] = CAT_MAX;
	return out;
}

void ClockGearsDatabaseDialog::select_category_by_key(const String &p_key) {
	if (p_key == "main_menu") {
		_on_category_selected(DB_CAT_MAIN_MENU);
		return;
	}
	if (p_key == "window_styles") {
		_on_category_selected(DB_CAT_WINDOW_STYLES);
		return;
	}
	if (p_key == "game_menu") {
		_on_category_selected(DB_CAT_GAME_MENU);
		return;
	}
	if (p_key == "credits") {
		_on_category_selected(DB_CAT_CREDITS);
		return;
	}
	if (p_key == "hud") {
		_on_category_selected(DB_CAT_HUD);
		return;
	}
	if (p_key == "ui_designer") {
		_on_category_selected(DB_CAT_UI_DESIGNER);
		return;
	}
	if (p_key == "game_over") {
		_on_category_selected(DB_CAT_GAME_OVER);
		return;
	}
	for (int i = 0; i < CAT_MAX; i++) {
		if (ClockGearsDataManager::get_category_key(i) == p_key) {
			_on_category_selected(i);
			return;
		}
	}
}

Ref<Texture2D> ClockGearsDatabaseDialog::get_record_list_icon(int p_row_idx) const {
	if (!record_list || p_row_idx < 0 || p_row_idx >= record_list->get_item_count()) {
		return Ref<Texture2D>();
	}
	return record_list->get_item_icon(p_row_idx);
}

// ============================================================================
// Equipo ARPG: Modo Completo/Simple, Orden de Capa y Vista Previa en Capas
// ============================================================================

void ClockGearsDatabaseDialog::_setup_arpg_equip_anim_ui() {
	const float row_h = 32.0f * EDSCALE;

	arpg_piece_mode_bar = memnew(HFlowContainer);
	arpg_piece_mode_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_piece_mode_bar->add_theme_constant_override("h_separation", int(12 * EDSCALE));
	arpg_piece_mode_bar->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	arpg_piece_mode_bar->set_visible(false);
	sprite_anim_editor_box->add_child(arpg_piece_mode_bar);

	Label *lbl_dm = memnew(Label(CG_DB_TR("Modo de dibujo:")));
	arpg_piece_mode_bar->add_child(lbl_dm);

	arpg_draw_mode_opt = memnew(OptionButton);
	arpg_draw_mode_opt->set_custom_minimum_size(Size2(220 * EDSCALE, row_h));
	arpg_draw_mode_opt->add_item(CG_DB_TR("Completo (animado)"), 0);
	arpg_draw_mode_opt->add_item(CG_DB_TR("Simple (imagen estática)"), 1);
	arpg_draw_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_draw_mode_selected));
	arpg_piece_mode_bar->add_child(arpg_draw_mode_opt);

	Label *lbl_lo = memnew(Label(CG_DB_TR("Orden de capa:")));
	arpg_piece_mode_bar->add_child(lbl_lo);

	arpg_layer_order_spin = memnew(SpinBox);
	arpg_layer_order_spin->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
	arpg_layer_order_spin->set_min(-99);
	arpg_layer_order_spin->set_max(99);
	arpg_layer_order_spin->set_value(1);
	_bind_numeric_spinbox(arpg_layer_order_spin);
	arpg_layer_order_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_layer_order_changed));
	arpg_piece_mode_bar->add_child(arpg_layer_order_spin);

	// Subpanel Modo Simple
	arpg_simple_editor_box = memnew(VBoxContainer);
	arpg_simple_editor_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_simple_editor_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	arpg_simple_editor_box->set_visible(false);
	sprite_anim_editor_box->add_child(arpg_simple_editor_box);

	Label *lbl_simp_hdr = memnew(Label(CG_DB_TR("Imagen estática por grupo y dirección")));
	lbl_simp_hdr->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	arpg_simple_editor_box->add_child(lbl_simp_hdr);
	arpg_simple_editor_box->add_child(memnew(HSeparator));

	HFlowContainer *simp_sel_row = memnew(HFlowContainer);
	simp_sel_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	simp_sel_row->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	simp_sel_row->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	arpg_simple_editor_box->add_child(simp_sel_row);

	simp_sel_row->add_child(memnew(Label(CG_DB_TR("Grupo:"))));
	arpg_simple_group_opt = memnew(OptionButton);
	arpg_simple_group_opt->set_custom_minimum_size(Size2(130 * EDSCALE, row_h));
	arpg_simple_group_opt->add_item(CG_DB_TR("Normal"), 0);
	arpg_simple_group_opt->set_item_metadata(0, "normal");
	arpg_simple_group_opt->add_item(CG_DB_TR("Atacar"), 1);
	arpg_simple_group_opt->set_item_metadata(1, "atacar");
	arpg_simple_group_opt->add_item(CG_DB_TR("Bloquear"), 2);
	arpg_simple_group_opt->set_item_metadata(2, "bloquear");
	arpg_simple_group_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_group_or_dir_changed));
	simp_sel_row->add_child(arpg_simple_group_opt);

	simp_sel_row->add_child(memnew(Label(CG_DB_TR("Dirección:"))));
	arpg_simple_dir_opt = memnew(OptionButton);
	arpg_simple_dir_opt->set_custom_minimum_size(Size2(170 * EDSCALE, row_h));
	arpg_simple_dir_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_group_or_dir_changed));
	simp_sel_row->add_child(arpg_simple_dir_opt);

	HBoxContainer *simp_path_hb = memnew(HBoxContainer);
	simp_path_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	simp_path_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	arpg_simple_editor_box->add_child(simp_path_hb);

	simp_path_hb->add_child(memnew(Label(CG_DB_TR("Archivo PNG:"))));
	arpg_simple_path_edit = memnew(LineEdit);
	arpg_simple_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_simple_path_edit->set_editable(false);
	arpg_simple_path_edit->set_placeholder(CG_DB_TR("(Ninguna imagen asignada)"));
	simp_path_hb->add_child(arpg_simple_path_edit);

	arpg_simple_browse_btn = memnew(Button(CG_DB_TR("Imagen...")));
	arpg_simple_browse_btn->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	arpg_simple_browse_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_browse_pressed));
	simp_path_hb->add_child(arpg_simple_browse_btn);

	arpg_simple_clear_btn = memnew(Button(CG_DB_TR("Limpiar")));
	arpg_simple_clear_btn->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	arpg_simple_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_clear_pressed));
	simp_path_hb->add_child(arpg_simple_clear_btn);

	arpg_simple_file_dialog = memnew(FileDialog);
	arpg_simple_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	arpg_simple_file_dialog->set_access(FileDialog::ACCESS_RESOURCES);
	arpg_simple_file_dialog->add_filter("*.png", CG_DB_TR("Imágenes PNG"));
	arpg_simple_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_file_selected));
	add_child(arpg_simple_file_dialog);

	HFlowContainer *simp_off_row = memnew(HFlowContainer);
	simp_off_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	simp_off_row->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	simp_off_row->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	arpg_simple_editor_box->add_child(simp_off_row);

	simp_off_row->add_child(memnew(Label(CG_DB_TR("Desplazamiento X:"))));
	arpg_simple_offset_x_spin = memnew(SpinBox);
	arpg_simple_offset_x_spin->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	arpg_simple_offset_x_spin->set_min(-256);
	arpg_simple_offset_x_spin->set_max(256);
	_bind_numeric_spinbox(arpg_simple_offset_x_spin);
	arpg_simple_offset_x_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_prop_changed).unbind(1));
	simp_off_row->add_child(arpg_simple_offset_x_spin);

	simp_off_row->add_child(memnew(Label(CG_DB_TR("Y:"))));
	arpg_simple_offset_y_spin = memnew(SpinBox);
	arpg_simple_offset_y_spin->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	arpg_simple_offset_y_spin->set_min(-256);
	arpg_simple_offset_y_spin->set_max(256);
	_bind_numeric_spinbox(arpg_simple_offset_y_spin);
	arpg_simple_offset_y_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_prop_changed).unbind(1));
	simp_off_row->add_child(arpg_simple_offset_y_spin);

	arpg_simple_center_btn = memnew(Button(CG_DB_TR("Centrar")));
	arpg_simple_center_btn->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	arpg_simple_center_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_center_pressed));
	simp_off_row->add_child(arpg_simple_center_btn);

	arpg_simple_behind_chk = memnew(CheckBox(CG_DB_TR("Dibujar detrás del cuerpo")));
	arpg_simple_behind_chk->set_custom_minimum_size(Size2(0, row_h));
	arpg_simple_behind_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_simple_prop_changed).unbind(1));
	simp_off_row->add_child(arpg_simple_behind_chk);

	// Vista previa compuesta sobre el cuerpo del actor (funciona tanto en Completo como en Simple)
	arpg_preview_box = memnew(VBoxContainer);
	arpg_preview_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_preview_box->add_theme_constant_override("separation", int(8 * EDSCALE));
	arpg_preview_box->set_visible(false);
	sprite_anim_editor_box->add_child(arpg_preview_box);

	arpg_preview_box->add_child(memnew(HSeparator));
	Label *lbl_ap_hdr = memnew(Label(CG_DB_TR("Vista previa sobre personaje")));
	lbl_ap_hdr->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	arpg_preview_box->add_child(lbl_ap_hdr);

	HFlowContainer *ap_ctrl_row = memnew(HFlowContainer);
	ap_ctrl_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ap_ctrl_row->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	ap_ctrl_row->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	arpg_preview_box->add_child(ap_ctrl_row);

	ap_ctrl_row->add_child(memnew(Label(CG_DB_TR("Actor:"))));
	arpg_prev_actor_opt = memnew(OptionButton);
	arpg_prev_actor_opt->set_custom_minimum_size(Size2(150 * EDSCALE, row_h));
	arpg_prev_actor_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_prev_selector_changed));
	ap_ctrl_row->add_child(arpg_prev_actor_opt);

	ap_ctrl_row->add_child(memnew(Label(CG_DB_TR("Estado:"))));
	arpg_prev_state_opt = memnew(OptionButton);
	arpg_prev_state_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	arpg_prev_state_opt->add_item(CG_DB_TR("Mover"), 0);
	arpg_prev_state_opt->set_item_metadata(0, "caminar");
	arpg_prev_state_opt->add_item(CG_DB_TR("Quieto"), 1);
	arpg_prev_state_opt->set_item_metadata(1, "quieto");
	arpg_prev_state_opt->add_item(CG_DB_TR("Atacar"), 2);
	arpg_prev_state_opt->set_item_metadata(2, "atacar");
	arpg_prev_state_opt->add_item(CG_DB_TR("Bloquear"), 3);
	arpg_prev_state_opt->set_item_metadata(3, "bloquear");
	arpg_prev_state_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_prev_selector_changed));
	ap_ctrl_row->add_child(arpg_prev_state_opt);

	ap_ctrl_row->add_child(memnew(Label(CG_DB_TR("Dirección:"))));
	arpg_prev_dir_opt = memnew(OptionButton);
	arpg_prev_dir_opt->set_custom_minimum_size(Size2(160 * EDSCALE, row_h));
	arpg_prev_dir_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_prev_selector_changed));
	ap_ctrl_row->add_child(arpg_prev_dir_opt);

	arpg_prev_show_rest_chk = memnew(CheckBox(CG_DB_TR("Mostrar el resto del equipo")));
	arpg_prev_show_rest_chk->set_custom_minimum_size(Size2(0, row_h));
	arpg_prev_show_rest_chk->set_pressed(false);
	arpg_prev_show_rest_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_prev_show_rest_toggled));
	ap_ctrl_row->add_child(arpg_prev_show_rest_chk);

	btn_arpg_prev_play = memnew(Button(CG_DB_TR("▶ Probar")));
	btn_arpg_prev_play->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	btn_arpg_prev_play->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_play_pressed));
	ap_ctrl_row->add_child(btn_arpg_prev_play);

	PanelContainer *ap_panel = memnew(PanelContainer);
	ap_panel->set_custom_minimum_size(Size2(220 * EDSCALE, 190 * EDSCALE));
	Ref<StyleBoxFlat> ap_sb;
	ap_sb.instantiate();
	ap_sb->set_anti_aliased(true);
	ap_sb->set_bg_color(Color(0.11f, 0.13f, 0.17f, 1.0f));
	ap_sb->set_border_width_all(1);
	ap_sb->set_border_color(Color(0.32f, 0.37f, 0.48f, 0.85f));
	ap_sb->set_corner_radius_all(6);
	ap_panel->add_theme_style_override("panel", ap_sb);
	arpg_preview_box->add_child(ap_panel);

	arpg_prev_canvas = memnew(Control);
	arpg_prev_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_prev_canvas->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	arpg_prev_canvas->set_custom_minimum_size(Size2(200 * EDSCALE, 180 * EDSCALE));
	arpg_prev_canvas->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	arpg_prev_canvas->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	arpg_prev_canvas->connect("draw", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_prev_canvas_draw));
	arpg_prev_canvas->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_arpg_prev_canvas_gui_input));
	ap_panel->add_child(arpg_prev_canvas);
}

static void s_populate_direction_option(OptionButton *p_opt, int p_dirs) {
	if (!p_opt) {
		return;
	}
	int prev_dir = (p_opt->get_selected() >= 0) ? int(p_opt->get_selected_metadata()) : 2;
	p_opt->clear();
	struct DirEntry {
		int code;
		const char *label;
	};
	static const DirEntry cards[4] = {
		{ 2, "Abajo" },
		{ 4, "Izquierda" },
		{ 6, "Derecha" },
		{ 8, "Arriba" }
	};
	static const DirEntry diags[4] = {
		{ 1, "Abajo-Izquierda" },
		{ 3, "Abajo-Derecha" },
		{ 7, "Arriba-Izquierda" },
		{ 9, "Arriba-Derecha" }
	};
	int sel_idx = 0;
	for (int i = 0; i < 4; i++) {
		int idx = p_opt->get_item_count();
		p_opt->add_item(CG_DB_TR(cards[i].label), cards[i].code);
		p_opt->set_item_metadata(idx, cards[i].code);
		if (cards[i].code == prev_dir) sel_idx = idx;
	}
	if (p_dirs == 8) {
		for (int i = 0; i < 4; i++) {
			int idx = p_opt->get_item_count();
			p_opt->add_item(CG_DB_TR(diags[i].label), diags[i].code);
			p_opt->set_item_metadata(idx, diags[i].code);
			if (diags[i].code == prev_dir) sel_idx = idx;
		}
	}
	if (p_opt->get_item_count() > 0) {
		p_opt->select(sel_idx);
	}
}

void ClockGearsDatabaseDialog::_refresh_arpg_piece_anim_mode_ui() {
	bool is_arpg_eq = s_is_arpg_equip_category(current_category);
	bool is_simple = is_arpg_eq && arpg_draw_mode_opt && arpg_draw_mode_opt->get_selected() == 1;
	if (anim_sel_sec) {
		anim_sel_sec->set_visible(!is_simple);
	}
	_refresh_anim_submode_ui();
	if (arpg_simple_editor_box) {
		arpg_simple_editor_box->set_visible(is_simple);
	}
	if (btn_arpg_prev_play) {
		btn_arpg_prev_play->set_visible(!is_simple);
	}
	if (arpg_preview_box && sprite_anim_editor_box && arpg_preview_box->get_parent() == sprite_anim_editor_box) {
		sprite_anim_editor_box->move_child(arpg_preview_box, sprite_anim_editor_box->get_child_count() - 1);
	}
}

static int _db_selected_record_index(ItemList *p_list) {
	if (!p_list) {
		return -1;
	}
	Vector<int> sel = p_list->get_selected_items();
	return sel.is_empty() ? -1 : sel[0];
}

void ClockGearsDatabaseDialog::_refresh_arpg_simple_fields() {
	if (!s_is_arpg_equip_category(current_category)) {
		return;
	}
	int dirs = (int(system_config.get("movement_directions", 4)) == 8) ? 8 : 4;
	bool prev_upd = updating_ui;
	updating_ui = true;
	s_populate_direction_option(arpg_simple_dir_opt, dirs);

	int idx = _db_selected_record_index(record_list);
	Dictionary entry;
	if (idx >= 0 && idx < category_data[current_category].size()) {
		Dictionary d = category_data[current_category][idx];
		Dictionary simp = d.get("simple_images", Dictionary());
		String grp = arpg_simple_group_opt && arpg_simple_group_opt->get_selected() >= 0
				? String(arpg_simple_group_opt->get_selected_metadata())
				: "normal";
		int dir_code = arpg_simple_dir_opt && arpg_simple_dir_opt->get_selected() >= 0
				? int(arpg_simple_dir_opt->get_selected_metadata())
				: 2;
		String dsuf = ClockGearsCharacterStateRegistry::direction_to_suffix(dir_code);
		if (simp.has(grp) && simp[grp].get_type() == Variant::DICTIONARY) {
			Dictionary gd = simp[grp];
			if (gd.has(dsuf) && gd[dsuf].get_type() == Variant::DICTIONARY) {
				entry = gd[dsuf];
			}
		}
	}
	if (arpg_simple_path_edit) {
		arpg_simple_path_edit->set_text(String(entry.get("path", "")));
	}
	if (arpg_simple_offset_x_spin) {
		int ox = int(entry.get("offset_x", 0));
		arpg_simple_offset_x_spin->set_value(ox);
		arpg_simple_offset_x_spin->set_meta("cg_last_valid_val", double(ox));
	}
	if (arpg_simple_offset_y_spin) {
		int oy = int(entry.get("offset_y", 0));
		arpg_simple_offset_y_spin->set_value(oy);
		arpg_simple_offset_y_spin->set_meta("cg_last_valid_val", double(oy));
	}
	if (arpg_simple_behind_chk) {
		arpg_simple_behind_chk->set_pressed(bool(entry.get("behind_body", false)));
	}
	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_refresh_arpg_preview_selectors() {
	bool prev_upd = updating_ui;
	updating_ui = true;
	int dirs = (int(system_config.get("movement_directions", 4)) == 8) ? 8 : 4;
	s_populate_direction_option(arpg_prev_dir_opt, dirs);
	if (arpg_prev_actor_opt) {
		int prev_id = (arpg_prev_actor_opt->get_selected() >= 0) ? arpg_prev_actor_opt->get_selected_id() : 0;
		_populate_db_records_option(arpg_prev_actor_opt, CAT_ACTORS, CG_DB_TR("(ningún actor)"), false);
		if (prev_id > 0) {
			_select_option_by_id_or_index(arpg_prev_actor_opt, prev_id);
		} else if (arpg_prev_actor_opt->get_item_count() > 1 && arpg_prev_actor_opt->get_item_id(0) == 0) {
			// Por defecto seleccionar el primer actor real disponible
			arpg_prev_actor_opt->select(1);
		}
	}
	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_update_arpg_preview_canvas() {
	if (arpg_prev_canvas) {
		arpg_prev_canvas->queue_redraw();
	}
}

void ClockGearsDatabaseDialog::_on_arpg_draw_mode_selected(int p_idx) {
	if (updating_ui || !_has_valid_selected_record()) return;
	_refresh_arpg_piece_anim_mode_ui();
	_on_param_changed();
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_on_arpg_layer_order_changed(double p_val) {
	if (updating_ui || !_has_valid_selected_record()) return;
	_on_param_changed();
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_on_arpg_simple_group_or_dir_changed(int p_idx) {
	if (updating_ui || !_has_valid_selected_record()) return;
	_refresh_arpg_simple_fields();
	// Sincronizar selector de Vista Previa para que editar una postura sea inmediato
	if (arpg_simple_group_opt && arpg_prev_state_opt) {
		String grp = String(arpg_simple_group_opt->get_selected_metadata());
		String target_state = (grp == "atacar") ? "atacar" : ((grp == "bloquear") ? "bloquear" : "caminar");
		for (int i = 0; i < arpg_prev_state_opt->get_item_count(); i++) {
			if (String(arpg_prev_state_opt->get_item_metadata(i)) == target_state) {
				arpg_prev_state_opt->select(i);
				break;
			}
		}
	}
	if (arpg_simple_dir_opt && arpg_prev_dir_opt && arpg_simple_dir_opt->get_selected() >= 0) {
		int dcode = int(arpg_simple_dir_opt->get_selected_metadata());
		for (int i = 0; i < arpg_prev_dir_opt->get_item_count(); i++) {
			if (int(arpg_prev_dir_opt->get_item_metadata(i)) == dcode) {
				arpg_prev_dir_opt->select(i);
				break;
			}
		}
	}
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_on_arpg_simple_browse_pressed() {
	if (arpg_simple_file_dialog) {
		arpg_simple_file_dialog->popup_centered_ratio(0.6);
	}
}

void ClockGearsDatabaseDialog::_on_arpg_simple_file_selected(const String &p_path) {
	if (arpg_simple_path_edit) {
		arpg_simple_path_edit->set_text(p_path);
	}
	_on_arpg_simple_prop_changed();
}

void ClockGearsDatabaseDialog::_on_arpg_simple_clear_pressed() {
	if (arpg_simple_path_edit) {
		arpg_simple_path_edit->set_text("");
	}
	_on_arpg_simple_prop_changed();
}

void ClockGearsDatabaseDialog::_on_arpg_simple_center_pressed() {
	updating_ui = true;
	if (arpg_simple_offset_x_spin) {
		arpg_simple_offset_x_spin->set_value(0);
		arpg_simple_offset_x_spin->set_meta("cg_last_valid_val", 0.0);
	}
	if (arpg_simple_offset_y_spin) {
		arpg_simple_offset_y_spin->set_value(0);
		arpg_simple_offset_y_spin->set_meta("cg_last_valid_val", 0.0);
	}
	updating_ui = false;
	_on_arpg_simple_prop_changed();
}

void ClockGearsDatabaseDialog::_on_arpg_simple_prop_changed() {
	if (updating_ui || !s_is_arpg_equip_category(current_category)) {
		return;
	}
	int idx = _db_selected_record_index(record_list);
	if (idx < 0 || idx >= category_data[current_category].size()) {
		return;
	}
	Dictionary d = category_data[current_category][idx];
	Dictionary simp = d.has("simple_images") && d["simple_images"].get_type() == Variant::DICTIONARY
			? Dictionary(d["simple_images"]).duplicate(true)
			: Dictionary();
	String grp = arpg_simple_group_opt && arpg_simple_group_opt->get_selected() >= 0
			? String(arpg_simple_group_opt->get_selected_metadata())
			: "normal";
	int dir_code = arpg_simple_dir_opt && arpg_simple_dir_opt->get_selected() >= 0
			? int(arpg_simple_dir_opt->get_selected_metadata())
			: 2;
	String dsuf = ClockGearsCharacterStateRegistry::direction_to_suffix(dir_code);
	Dictionary gd = simp.has(grp) && simp[grp].get_type() == Variant::DICTIONARY
			? Dictionary(simp[grp]).duplicate(true)
			: Dictionary();
	Dictionary entry;
	entry["path"] = arpg_simple_path_edit ? arpg_simple_path_edit->get_text().strip_edges() : "";
	entry["offset_x"] = arpg_simple_offset_x_spin ? int(arpg_simple_offset_x_spin->get_value()) : 0;
	entry["offset_y"] = arpg_simple_offset_y_spin ? int(arpg_simple_offset_y_spin->get_value()) : 0;
	entry["behind_body"] = arpg_simple_behind_chk ? arpg_simple_behind_chk->is_pressed() : false;
	gd[dsuf] = entry;
	simp[grp] = gd;
	d["simple_images"] = simp;
	category_data[current_category][idx] = d;
	_mark_database_dirty();
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_on_arpg_prev_selector_changed(int p_idx) {
	if (updating_ui) return;
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_on_arpg_prev_show_rest_toggled(bool p_pressed) {
	if (updating_ui) return;
	_update_arpg_preview_canvas();
}

void ClockGearsDatabaseDialog::_on_arpg_prev_canvas_draw() {
	if (!arpg_prev_canvas || !s_is_arpg_equip_category(current_category)) {
		return;
	}
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		return;
	}
	Size2 csz = arpg_prev_canvas->get_size();
	arpg_prev_canvas->draw_rect(Rect2(Vector2(), csz), Color(0.11f, 0.13f, 0.17f, 1.0f));
	int scale = MAX(1, arpg_prev_draw_scale);
	const int tile_size = 32;
	Vector2 cell_origin((csz.x - tile_size * scale) * 0.5f, (csz.y - tile_size * scale) * 0.5f);
	arpg_prev_canvas->draw_rect(Rect2(cell_origin, Size2(tile_size * scale, tile_size * scale)), Color(1, 1, 1, 0.08f), false, 1.0f);

	String state_id = arpg_prev_state_opt && arpg_prev_state_opt->get_selected() >= 0
			? String(arpg_prev_state_opt->get_selected_metadata())
			: "caminar";
	int dir = arpg_prev_dir_opt && arpg_prev_dir_opt->get_selected() >= 0
			? int(arpg_prev_dir_opt->get_selected_metadata())
			: 2;
	int move_dirs = (int(system_config.get("movement_directions", 4)) == 8) ? 8 : 4;

	// Obtener actor seleccionado (por defecto el primero disponible)
	Dictionary actor_rec;
	int actor_id = arpg_prev_actor_opt ? arpg_prev_actor_opt->get_selected_id() : 0;
	for (int i = 0; i < category_data[CAT_ACTORS].size(); i++) {
		if (category_data[CAT_ACTORS][i].get_type() != Variant::DICTIONARY) continue;
		Dictionary ad = category_data[CAT_ACTORS][i];
		if (actor_id <= 0 || int(ad.get("id", 0)) == actor_id) {
			actor_rec = ad;
			actor_id = int(ad.get("id", 0));
			break;
		}
	}
	bool act_dir_idle = bool(actor_rec.get("directional_idle", false));

	struct DrawLayerItem {
		int z = 0;
		Ref<Texture2D> tex;
		bool use_region = false;
		Rect2 region;
		Vector2 pos_px;
		Size2 size_px;
	};
	Vector<DrawLayerItem> items;

	// 1. Capa del cuerpo del actor (z = 0)
	if (!actor_rec.is_empty()) {
		ClockGearsSpriteSheetData act_sheet;
		if (actor_rec.has("sprite_data") && actor_rec["sprite_data"].get_type() == Variant::DICTIONARY) {
			act_sheet.from_dict(Dictionary(actor_rec["sprite_data"]));
		}
		ClockGearsSpriteAnimation act_anim;
		bool act_freeze = false;
		String act_slot;
		if (reg->resolve_actor_animation(act_sheet, act_dir_idle, state_id, dir, act_anim, act_freeze, act_slot) && act_anim.get_frame_count() > 0) {
			int total = act_anim.get_frame_count();
			int fi = act_freeze ? 0 : (anim_preview_frame_idx % total);
			ClockGearsAnimationFrame fr = act_anim.get_frame(fi, 0, 0);
			String spath = !fr.sheet_path.is_empty() ? fr.sheet_path : act_anim.spritesheet_path;
			if (spath.is_empty()) spath = String(actor_rec.get("graphic_path", ""));
			Ref<Texture2D> btex = _load_texture_from_path(spath);
			if (btex.is_valid()) {
				ClockGearsAnimationFrame fr_tex = act_anim.get_frame(fi, btex->get_width(), btex->get_height());
				Rect2 reg_r(fr_tex.region);
				if (reg_r.size.x <= 0 || reg_r.size.y <= 0 || reg_r.position.x + reg_r.size.x > btex->get_width() || reg_r.position.y + reg_r.size.y > btex->get_height()) {
					reg_r = Rect2(0, 0, btex->get_width(), btex->get_height());
				}
				DrawLayerItem body_it;
				body_it.z = 0;
				body_it.tex = btex;
				body_it.use_region = true;
				body_it.region = reg_r;
				body_it.size_px = reg_r.size;
				body_it.pos_px = ClockGearsCharacterStateRegistry::compute_layer_sprite_position(true, tile_size, int(reg_r.size.x), int(reg_r.size.y), 0, 0);
				items.push_back(body_it);
			}
		} else {
			String gp = String(actor_rec.get("graphic_path", ""));
			Ref<Texture2D> btex = _load_texture_from_path(gp);
			if (btex.is_valid()) {
				int rows = (btex->get_height() >= btex->get_width() * 2) ? 8 : 4;
				int cols = MAX(1, act_sheet.cols > 0 ? act_sheet.cols : 4);
				int fw = MAX(1, btex->get_width() / cols);
				int fh = MAX(1, btex->get_height() / rows);
				int row_idx = ClockGearsCharacterStateRegistry::direction_to_sheet_row(dir, rows);
				int fi = (anim_preview_frame_idx % cols);
				DrawLayerItem body_it;
				body_it.z = 0;
				body_it.tex = btex;
				body_it.use_region = true;
				body_it.region = Rect2(fi * fw, row_idx * fh, fw, fh);
				body_it.size_px = Size2(fw, fh);
				body_it.pos_px = ClockGearsCharacterStateRegistry::compute_layer_sprite_position(true, tile_size, fw, fh, 0, 0);
				items.push_back(body_it);
			}
		}
	}

	// 2. Pieza actual en edición (usando datos en vivo)
	int cur_idx = _db_selected_record_index(record_list);
	int cur_piece_id = 0;
	if (cur_idx >= 0 && cur_idx < category_data[current_category].size()) {
		Dictionary live_piece = Dictionary(category_data[current_category][cur_idx]).duplicate(true);
		cur_piece_id = int(live_piece.get("id", 0));
		live_piece["sprite_data"] = current_sprite_data.to_dict();
		if (arpg_draw_mode_opt) {
			live_piece["draw_mode"] = (arpg_draw_mode_opt->get_selected() == 1) ? "simple" : "completo";
		}
		if (arpg_layer_order_spin) {
			live_piece["layer_order"] = int(arpg_layer_order_spin->get_value());
		}
		ClockGearsResolvedEquipLayer rl;
		if (reg->resolve_arpg_piece_layer(live_piece, current_category, state_id, dir, anim_preview_frame_idx, act_dir_idle, move_dirs, rl) && rl.visible) {
			Ref<Texture2D> ptex = _load_texture_from_path(rl.texture_path);
			if (ptex.is_valid()) {
				int fw = rl.is_completo && rl.region.size.x > 0 ? rl.region.size.x : ptex->get_width();
				int fh = rl.is_completo && rl.region.size.y > 0 ? rl.region.size.y : ptex->get_height();
				DrawLayerItem pi;
				pi.z = ClockGearsCharacterStateRegistry::compute_layer_z_index(rl.behind_body, rl.layer_order, 0);
				pi.tex = ptex;
				pi.use_region = rl.is_completo && rl.region.size.x > 0 && rl.region.size.y > 0;
				pi.region = Rect2(rl.region);
				pi.size_px = Size2(fw, fh);
				pi.pos_px = ClockGearsCharacterStateRegistry::compute_layer_sprite_position(rl.is_completo, tile_size, fw, fh, rl.offset_x, rl.offset_y);
				items.push_back(pi);
			}
		}
	}

	// 3. Resto del equipo del actor si "Mostrar el resto del equipo" está marcado
	if (arpg_prev_show_rest_chk && arpg_prev_show_rest_chk->is_pressed() && actor_id > 0 && ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		Array eq_slots = dm->get_actor_arpg_equip_slots(actor_id);
		for (int s = 0; s < eq_slots.size(); s++) {
			if (eq_slots[s].get_type() != Variant::DICTIONARY) continue;
			Dictionary sd = eq_slots[s];
			int pid = int(sd.get("piece_id", 0));
			int pcat = ClockGearsDataManager::get_category_from_key(String(sd.get("category", "")));
			if (pid <= 0 || pcat < 0 || (pcat == current_category && pid == cur_piece_id)) {
				continue;
			}
			Dictionary other_piece = dm->get_record_by_id(pcat, pid);
			ClockGearsResolvedEquipLayer ol;
			if (reg->resolve_arpg_piece_layer(other_piece, pcat, state_id, dir, anim_preview_frame_idx, act_dir_idle, move_dirs, ol) && ol.visible) {
				Ref<Texture2D> otex = _load_texture_from_path(ol.texture_path);
				if (otex.is_valid()) {
					int fw = ol.is_completo && ol.region.size.x > 0 ? ol.region.size.x : otex->get_width();
					int fh = ol.is_completo && ol.region.size.y > 0 ? ol.region.size.y : otex->get_height();
					DrawLayerItem oi;
					oi.z = ClockGearsCharacterStateRegistry::compute_layer_z_index(ol.behind_body, ol.layer_order, s + 1);
					oi.tex = otex;
					oi.use_region = ol.is_completo && ol.region.size.x > 0 && ol.region.size.y > 0;
					oi.region = Rect2(ol.region);
					oi.size_px = Size2(fw, fh);
					oi.pos_px = ClockGearsCharacterStateRegistry::compute_layer_sprite_position(ol.is_completo, tile_size, fw, fh, ol.offset_x, ol.offset_y);
					items.push_back(oi);
				}
			}
		}
	}

	// Orden estable detrás -> delante por z
	for (int i = 1; i < items.size(); i++) {
		DrawLayerItem key = items[i];
		int j = i - 1;
		while (j >= 0 && items[j].z > key.z) {
			items.write[j + 1] = items[j];
			j--;
		}
		items.write[j + 1] = key;
	}

	for (int i = 0; i < items.size(); i++) {
		const DrawLayerItem &it = items[i];
		Rect2 dst(cell_origin + it.pos_px * float(scale), it.size_px * float(scale));
		if (it.use_region) {
			arpg_prev_canvas->draw_texture_rect_region(it.tex, dst, it.region);
		} else {
			arpg_prev_canvas->draw_texture_rect(it.tex, dst, false);
		}
	}
}

void ClockGearsDatabaseDialog::_on_arpg_prev_canvas_gui_input(const Ref<InputEvent> &p_ev) {
	if (!s_is_arpg_equip_category(current_category) || !arpg_draw_mode_opt || arpg_draw_mode_opt->get_selected() != 1) {
		return;
	}
	Ref<InputEventMouseButton> mb = p_ev;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			arpg_prev_dragging = true;
			arpg_prev_drag_start_mouse = mb->get_position();
			arpg_prev_drag_start_ox = arpg_simple_offset_x_spin ? int(arpg_simple_offset_x_spin->get_value()) : 0;
			arpg_prev_drag_start_oy = arpg_simple_offset_y_spin ? int(arpg_simple_offset_y_spin->get_value()) : 0;
		} else {
			arpg_prev_dragging = false;
		}
		return;
	}
	Ref<InputEventMouseMotion> mm = p_ev;
	if (mm.is_valid() && arpg_prev_dragging) {
		Vector2 delta = mm->get_position() - arpg_prev_drag_start_mouse;
		int scale = MAX(1, arpg_prev_draw_scale);
		int nx = arpg_prev_drag_start_ox + int(Math::round(delta.x / float(scale)));
		int ny = arpg_prev_drag_start_oy + int(Math::round(delta.y / float(scale)));
		updating_ui = true;
		if (arpg_simple_offset_x_spin) {
			arpg_simple_offset_x_spin->set_value(nx);
			arpg_simple_offset_x_spin->set_meta("cg_last_valid_val", double(nx));
		}
		if (arpg_simple_offset_y_spin) {
			arpg_simple_offset_y_spin->set_value(ny);
			arpg_simple_offset_y_spin->set_meta("cg_last_valid_val", double(ny));
		}
		updating_ui = false;
		_on_arpg_simple_prop_changed();
	}
}

// ============================================================================
// Editor de Formas, Hitboxes y Maniquí (9A Tanda 2)
// ============================================================================

static void s_sync_anim_frames_data(ClockGearsSpriteAnimation &r_anim) {
	int fc = r_anim.get_frame_count();
	if (r_anim.frames_data.size() < fc) {
		int prev_sz = r_anim.frames_data.size();
		r_anim.frames_data.resize(fc);
		for (int i = prev_sz; i < fc; i++) {
			r_anim.frames_data.write[i] = r_anim.get_frame(i, 0, 0);
		}
	}
}

static bool s_parse_slot_dir_and_prefix(const String &p_slot_name, String &r_prefix, int &r_dir_code) {
	static const int k_dir_order[8] = { 1, 3, 7, 9, 2, 4, 6, 8 };
	String low = p_slot_name.strip_edges().to_lower();
	for (int i = 0; i < 8; i++) {
		int d = k_dir_order[i];
		String suf = "_" + ClockGearsCharacterStateRegistry::direction_to_suffix(d);
		if (low.ends_with(suf) && low.length() > suf.length()) {
			r_prefix = low.substr(0, low.length() - suf.length());
			r_dir_code = d;
			return true;
		}
	}
	r_prefix = low;
	r_dir_code = 0;
	return false;
}

static String s_dir_code_display_name(int p_dir) {
	switch (p_dir) {
		case 2: return CG_DB_TR("Abajo");
		case 4: return CG_DB_TR("Izquierda");
		case 6: return CG_DB_TR("Derecha");
		case 8: return CG_DB_TR("Arriba");
		case 1: return CG_DB_TR("Abajo-Izquierda");
		case 3: return CG_DB_TR("Abajo-Derecha");
		case 7: return CG_DB_TR("Arriba-Izquierda");
		case 9: return CG_DB_TR("Arriba-Derecha");
		default: return ClockGearsCharacterStateRegistry::direction_to_suffix(p_dir);
	}
}

void ClockGearsDatabaseDialog::_setup_shape_editor_ui() {
	const float row_h = 32.0f * EDSCALE;

	anim_formas_sec = memnew(VBoxContainer);
	anim_formas_sec->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_formas_sec->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	anim_formas_sec->add_theme_constant_override("separation", int(8 * EDSCALE));
	anim_formas_sec->set_visible(false);
	sprite_anim_editor_box->add_child(anim_formas_sec);

	formas_frame_bar = memnew(HFlowContainer);
	formas_frame_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	formas_frame_bar->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	formas_frame_bar->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	anim_formas_sec->add_child(formas_frame_bar);

	formas_frame_bar->add_child(memnew(Label(CG_DB_TR("Frame:"))));

	btn_shape_prev_frame = memnew(Button("<"));
	btn_shape_prev_frame->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	btn_shape_prev_frame->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_prev_frame_pressed));
	formas_frame_bar->add_child(btn_shape_prev_frame);

	shape_frame_opt = memnew(OptionButton);
	shape_frame_opt->set_custom_minimum_size(Size2(126 * EDSCALE, row_h));
	shape_frame_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_frame_opt_selected));
	formas_frame_bar->add_child(shape_frame_opt);

	btn_shape_next_frame = memnew(Button(">"));
	btn_shape_next_frame->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	btn_shape_next_frame->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_next_frame_pressed));
	formas_frame_bar->add_child(btn_shape_next_frame);

	btn_shape_copy_next = memnew(Button(CG_DB_TR("Copiar al siguiente")));
	btn_shape_copy_next->set_custom_minimum_size(Size2(138 * EDSCALE, row_h));
	btn_shape_copy_next->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_copy_next_pressed));
	formas_frame_bar->add_child(btn_shape_copy_next);

	btn_shape_copy_all = memnew(Button(CG_DB_TR("Copiar a todos")));
	btn_shape_copy_all->set_custom_minimum_size(Size2(114 * EDSCALE, row_h));
	btn_shape_copy_all->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_copy_all_pressed));
	formas_frame_bar->add_child(btn_shape_copy_all);

	shape_copy_dir_opt = memnew(OptionButton);
	shape_copy_dir_opt->set_custom_minimum_size(Size2(146 * EDSCALE, row_h));
	formas_frame_bar->add_child(shape_copy_dir_opt);

	shape_copy_mode_opt = memnew(OptionButton);
	shape_copy_mode_opt->set_custom_minimum_size(Size2(104 * EDSCALE, row_h));
	shape_copy_mode_opt->add_item(CG_DB_TR("Auto"), 0);
	shape_copy_mode_opt->set_item_metadata(0, "auto");
	shape_copy_mode_opt->add_item(CG_DB_TR("Rotar"), 1);
	shape_copy_mode_opt->set_item_metadata(1, "rotate");
	shape_copy_mode_opt->add_item(CG_DB_TR("Espejar"), 2);
	shape_copy_mode_opt->set_item_metadata(2, "mirror");
	shape_copy_mode_opt->select(0);
	formas_frame_bar->add_child(shape_copy_mode_opt);

	btn_shape_copy_dir = memnew(Button(CG_DB_TR("Copiar desde otra dirección")));
	btn_shape_copy_dir->set_custom_minimum_size(Size2(184 * EDSCALE, row_h));
	btn_shape_copy_dir->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_copy_dir_pressed));
	formas_frame_bar->add_child(btn_shape_copy_dir);

	anim_formas_sec->add_child(memnew(HSeparator));

	shape_no_frames_lbl = memnew(Label(CG_DB_TR("(Esta ranura no tiene frames. Añádelos primero en el modo Frames.)")));
	shape_no_frames_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	shape_no_frames_lbl->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.80f));
	shape_no_frames_lbl->set_visible(false);
	anim_formas_sec->add_child(shape_no_frames_lbl);

	shape_editor = memnew(ClockGearsShapeEditor);
	shape_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	shape_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	shape_editor->connect("shape_data_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_shape_editor_data_changed));
	anim_formas_sec->add_child(shape_editor);
}

void ClockGearsDatabaseDialog::_refresh_anim_submode_ui() {
	bool supports_shapes = s_is_arpg_shape_category(current_category);
	bool is_arpg_eq = s_is_arpg_equip_category(current_category);
	bool is_simple = is_arpg_eq && arpg_draw_mode_opt && arpg_draw_mode_opt->get_selected() == 1;

	if (!supports_shapes || is_simple) {
		if (!supports_shapes) {
			anim_submode = 0;
		}
		if (anim_submode_bar) {
			anim_submode_bar->set_visible(false);
		}
		if (anim_formas_sec) {
			anim_formas_sec->set_visible(false);
		}
		if (anim_frames_sec) {
			anim_frames_sec->set_visible(!is_simple);
		}
		return;
	}

	if (anim_submode_bar) {
		anim_submode_bar->set_visible(true);
	}
	if (btn_anim_mode_frames) {
		btn_anim_mode_frames->set_pressed_no_signal(anim_submode == 0);
	}
	if (btn_anim_mode_shapes) {
		btn_anim_mode_shapes->set_pressed_no_signal(anim_submode == 1);
	}
	if (anim_frames_sec) {
		anim_frames_sec->set_visible(anim_submode == 0);
	}
	if (anim_formas_sec) {
		anim_formas_sec->set_visible(anim_submode == 1);
	}
}

void ClockGearsDatabaseDialog::_refresh_shape_copy_dir_options() {
	if (!shape_copy_dir_opt || !btn_shape_copy_dir) {
		return;
	}
	shape_copy_dir_opt->clear();
	String cur_slot = _get_active_anim_name();
	String slot_prefix;
	int cur_dir = 0;
	if (!s_parse_slot_dir_and_prefix(cur_slot, slot_prefix, cur_dir)) {
		shape_copy_dir_opt->add_item(CG_DB_TR("(Ranura sin dirección)"), 0);
		shape_copy_dir_opt->set_disabled(true);
		btn_shape_copy_dir->set_disabled(true);
		return;
	}

	int dirs = get_system_movement_directions();
	static const int k_cardinals[4] = { 2, 4, 6, 8 };
	static const int k_all_dirs[8] = { 2, 4, 6, 8, 1, 3, 7, 9 };
	int n_dirs = (dirs == 8) ? 8 : 4;
	const int *dir_arr = (dirs == 8) ? k_all_dirs : k_cardinals;

	for (int i = 0; i < n_dirs; i++) {
		int d = dir_arr[i];
		if (d == cur_dir) {
			continue;
		}
		String suf = ClockGearsCharacterStateRegistry::direction_to_suffix(d);
		String other_slot = slot_prefix + "_" + suf;
		if (!current_sprite_data.animations.has(other_slot)) {
			continue;
		}
		const ClockGearsSpriteAnimation &oa = current_sprite_data.animations[other_slot];
		if (oa.get_frame_count() <= 0) {
			continue;
		}
		int idx = shape_copy_dir_opt->get_item_count();
		shape_copy_dir_opt->add_item(CG_DB_TR("Desde: ") + s_dir_code_display_name(d), d);
		shape_copy_dir_opt->set_item_metadata(idx, suf);
	}

	bool has_sources = shape_copy_dir_opt->get_item_count() > 0;
	if (!has_sources) {
		shape_copy_dir_opt->add_item(CG_DB_TR("(Sin otras direcciones)"), 0);
	} else {
		shape_copy_dir_opt->select(0);
	}
	shape_copy_dir_opt->set_disabled(!has_sources);
	btn_shape_copy_dir->set_disabled(!has_sources);
}

void ClockGearsDatabaseDialog::_sync_shape_editor_for_current_frame() {
	if (!shape_editor) {
		return;
	}
	String cur_slot = _get_active_anim_name();
	bool allow_hb = ClockGearsSpriteAnimation::is_attack_slot_name(cur_slot);
	bool show_mann = s_is_arpg_equip_category(current_category);
	int mann_sz = get_system_mannequin_size();

	shape_editor->set_mannequin(show_mann, mann_sz);
	shape_editor->set_feature_flags(allow_hb, true, true);

	if (!current_sprite_data.animations.has(cur_slot) || current_sprite_data.animations[cur_slot].get_frame_count() <= 0) {
		if (shape_no_frames_lbl) shape_no_frames_lbl->set_visible(true);
		shape_editor->set_visible(false);
		return;
	}

	if (shape_no_frames_lbl) shape_no_frames_lbl->set_visible(false);
	shape_editor->set_visible(true);

	ClockGearsSpriteAnimation &anim = current_sprite_data.animations[cur_slot];
	s_sync_anim_frames_data(anim);
	int total = anim.frames_data.size();
	shape_frame_idx = CLAMP(shape_frame_idx, 0, total - 1);

	String fallback_sheet = anim.spritesheet_path;
	if (fallback_sheet.is_empty()) {
		int rec_idx = _db_selected_record_index(record_list);
		if (rec_idx >= 0 && rec_idx < category_data[current_category].size()) {
			fallback_sheet = Dictionary(category_data[current_category][rec_idx]).get("graphic_path", "");
		}
	}

	const ClockGearsAnimationFrame &raw_fr = anim.frames_data[shape_frame_idx];
	String spath = raw_fr.sheet_path.is_empty() ? fallback_sheet : raw_fr.sheet_path;
	Ref<Texture2D> tex = _load_texture_from_path(spath);
	int tw = tex.is_valid() ? tex->get_width() : 0;
	int th = tex.is_valid() ? tex->get_height() : 0;
	ClockGearsAnimationFrame resolved_fr = anim.get_frame(shape_frame_idx, tw, th);

	updating_shape_ui = true;
	shape_editor->set_preview_texture(tex, resolved_fr.region);
	const ClockGearsHurtboxData &eff_hurt = resolved_fr.has_hurtbox_override
			? resolved_fr.hurtbox_override
			: current_sprite_data.hurtbox;
	shape_editor->set_shape_data(
			resolved_fr.hitbox.to_dict(),
			eff_hurt.to_dict(),
			resolved_fr.has_hurtbox_override,
			resolved_fr.weapon_anchor.to_dict());
	updating_shape_ui = false;
}

void ClockGearsDatabaseDialog::_refresh_shape_editor_context() {
	if (!anim_formas_sec || !shape_editor) {
		return;
	}
	if (!s_is_arpg_shape_category(current_category)) {
		return;
	}

	String cur_slot = _get_active_anim_name();
	int total = 0;
	if (current_sprite_data.animations.has(cur_slot)) {
		ClockGearsSpriteAnimation &anim = current_sprite_data.animations[cur_slot];
		s_sync_anim_frames_data(anim);
		total = anim.frames_data.size();
	}

	if (total > 0) {
		shape_frame_idx = CLAMP(shape_frame_idx, 0, total - 1);
	} else {
		shape_frame_idx = 0;
	}

	updating_shape_ui = true;
	if (shape_frame_opt) {
		shape_frame_opt->clear();
		if (total > 0) {
			for (int i = 0; i < total; i++) {
				shape_frame_opt->add_item(vformat(CG_DB_TR("Frame %d / %d"), i + 1, total), i);
			}
			shape_frame_opt->select(shape_frame_idx);
			shape_frame_opt->set_disabled(false);
		} else {
			shape_frame_opt->add_item(CG_DB_TR("Frame 0 / 0"), 0);
			shape_frame_opt->set_disabled(true);
		}
	}
	if (btn_shape_prev_frame) btn_shape_prev_frame->set_disabled(total <= 1 || shape_frame_idx <= 0);
	if (btn_shape_next_frame) btn_shape_next_frame->set_disabled(total <= 1 || shape_frame_idx >= total - 1);
	if (btn_shape_copy_next) btn_shape_copy_next->set_disabled(total <= 1 || shape_frame_idx >= total - 1);
	if (btn_shape_copy_all) btn_shape_copy_all->set_disabled(total <= 1);
	updating_shape_ui = false;

	_refresh_shape_copy_dir_options();
	_sync_shape_editor_for_current_frame();
}

void ClockGearsDatabaseDialog::_on_anim_submode_selected(int p_mode) {
	anim_submode = (p_mode == 1 && s_is_arpg_shape_category(current_category)) ? 1 : 0;
	_refresh_anim_submode_ui();
	if (anim_submode == 1) {
		_refresh_shape_editor_context();
	}
}

void ClockGearsDatabaseDialog::_on_shape_frame_opt_selected(int p_idx) {
	if (updating_shape_ui) return;
	shape_frame_idx = MAX(0, p_idx);
	_refresh_shape_editor_context();
}

void ClockGearsDatabaseDialog::_on_shape_prev_frame_pressed() {
	if (shape_frame_idx > 0) {
		shape_frame_idx--;
		_refresh_shape_editor_context();
	}
}

void ClockGearsDatabaseDialog::_on_shape_next_frame_pressed() {
	String cur_slot = _get_active_anim_name();
	if (current_sprite_data.animations.has(cur_slot)) {
		int total = current_sprite_data.animations[cur_slot].get_frame_count();
		if (shape_frame_idx + 1 < total) {
			shape_frame_idx++;
			_refresh_shape_editor_context();
		}
	}
}

void ClockGearsDatabaseDialog::_on_shape_copy_next_pressed() {
	copy_shapes_to_next_frame();
}

void ClockGearsDatabaseDialog::_on_shape_copy_all_pressed() {
	copy_shapes_to_all_frames();
}

void ClockGearsDatabaseDialog::_on_shape_copy_dir_pressed() {
	if (!shape_copy_dir_opt || shape_copy_dir_opt->get_selected() < 0) {
		return;
	}
	int sel = shape_copy_dir_opt->get_selected();
	String suf = String(shape_copy_dir_opt->get_item_metadata(sel));
	if (suf.is_empty()) {
		return;
	}
	String mode_str = "auto";
	if (shape_copy_mode_opt && shape_copy_mode_opt->get_selected() >= 0) {
		String m = String(shape_copy_mode_opt->get_selected_metadata());
		if (!m.is_empty()) {
			mode_str = m;
		}
	}
	copy_shapes_from_direction(suf, mode_str);
}

void ClockGearsDatabaseDialog::_on_shape_editor_data_changed() {
	if (updating_shape_ui || updating_ui || !shape_editor) {
		return;
	}
	String cur_slot = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_slot)) {
		return;
	}
	ClockGearsSpriteAnimation &anim = current_sprite_data.animations[cur_slot];
	s_sync_anim_frames_data(anim);
	if (anim.frames_data.is_empty()) {
		return;
	}
	shape_frame_idx = CLAMP(shape_frame_idx, 0, anim.frames_data.size() - 1);
	ClockGearsAnimationFrame &fr = anim.frames_data.write[shape_frame_idx];

	int fw = current_sprite_data.frame_w > 0 ? current_sprite_data.frame_w : 32;
	int fh = current_sprite_data.frame_h > 0 ? current_sprite_data.frame_h : 32;
	if (fr.region.size.x > 0 && fr.region.size.y > 0) {
		fw = fr.region.size.x;
		fh = fr.region.size.y;
	}

	if (ClockGearsSpriteAnimation::is_attack_slot_name(cur_slot)) {
		fr.hitbox.from_dict(shape_editor->get_hitbox_data());
	}
	fr.weapon_anchor.from_dict(shape_editor->get_weapon_anchor_data());

	bool new_override = shape_editor->has_hurtbox_override_enabled();
	bool was_override = fr.has_hurtbox_override;
	fr.has_hurtbox_override = new_override;
	if (new_override) {
		fr.hurtbox_override.from_dict(shape_editor->get_hurtbox_data(), fw, fh);
	} else if (was_override) {
		// Se acaba de desactivar la anulación por frame: restaurar en pantalla la hurtbox compartida del sprite
		_sync_shape_editor_for_current_frame();
	} else {
		current_sprite_data.hurtbox.from_dict(shape_editor->get_hurtbox_data(), fw, fh);
	}

	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_system_mannequin_size_selected(int p_idx) {
	if (updating_ui || !sys_mannequin_size_opt) {
		return;
	}
	int msz = ClockGearsDataManager::normalize_mannequin_size(sys_mannequin_size_opt->get_item_id(p_idx));
	set_system_mannequin_size(msz);
}

void ClockGearsDatabaseDialog::set_system_mannequin_size(int p_size) {
	int msz = ClockGearsDataManager::normalize_mannequin_size(p_size);
	system_config["mannequin_size"] = msz;
	if (ClockGearsDataManager::get_singleton()) {
		ClockGearsDataManager::get_singleton()->set_mannequin_size(msz);
		ClockGearsDataManager::get_singleton()->set_system_config(system_config);
	}
	if (sys_mannequin_size_opt) {
		bool prev_upd = updating_ui;
		updating_ui = true;
		sys_mannequin_size_opt->select(msz == 64 ? 2 : (msz == 48 ? 1 : 0));
		updating_ui = prev_upd;
	}
	if (shape_editor) {
		shape_editor->set_mannequin(s_is_arpg_equip_category(current_category), msz);
	}
	_mark_database_dirty();
}

int ClockGearsDatabaseDialog::get_system_mannequin_size() const {
	return ClockGearsDataManager::normalize_mannequin_size(int(system_config.get("mannequin_size", 32)));
}

void ClockGearsDatabaseDialog::set_anim_submode(int p_mode) {
	_on_anim_submode_selected(p_mode);
}

int ClockGearsDatabaseDialog::get_anim_submode() const {
	return anim_submode;
}

void ClockGearsDatabaseDialog::select_shape_frame(int p_frame_idx) {
	shape_frame_idx = MAX(0, p_frame_idx);
	_refresh_shape_editor_context();
}

bool ClockGearsDatabaseDialog::copy_shapes_to_next_frame() {
	String cur_slot = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_slot)) {
		return false;
	}
	ClockGearsSpriteAnimation &anim = current_sprite_data.animations[cur_slot];
	s_sync_anim_frames_data(anim);
	if (shape_frame_idx < 0 || shape_frame_idx + 1 >= anim.frames_data.size()) {
		return false;
	}
	const ClockGearsAnimationFrame src = anim.frames_data[shape_frame_idx];
	ClockGearsAnimationFrame &dst = anim.frames_data.write[shape_frame_idx + 1];
	if (ClockGearsSpriteAnimation::is_attack_slot_name(cur_slot)) {
		dst.hitbox = src.hitbox;
	}
	dst.has_hurtbox_override = src.has_hurtbox_override;
	if (src.has_hurtbox_override) {
		dst.hurtbox_override = src.hurtbox_override;
	}
	dst.weapon_anchor = src.weapon_anchor;

	_on_param_changed();
	_refresh_shape_editor_context();
	return true;
}

bool ClockGearsDatabaseDialog::copy_shapes_to_all_frames() {
	String cur_slot = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_slot)) {
		return false;
	}
	ClockGearsSpriteAnimation &anim = current_sprite_data.animations[cur_slot];
	s_sync_anim_frames_data(anim);
	if (anim.frames_data.size() <= 1 || shape_frame_idx < 0 || shape_frame_idx >= anim.frames_data.size()) {
		return false;
	}
	const ClockGearsAnimationFrame src = anim.frames_data[shape_frame_idx];
	bool is_atk = ClockGearsSpriteAnimation::is_attack_slot_name(cur_slot);
	for (int i = 0; i < anim.frames_data.size(); i++) {
		if (i == shape_frame_idx) continue;
		ClockGearsAnimationFrame &dst = anim.frames_data.write[i];
		if (is_atk) {
			dst.hitbox = src.hitbox;
		}
		dst.has_hurtbox_override = src.has_hurtbox_override;
		if (src.has_hurtbox_override) {
			dst.hurtbox_override = src.hurtbox_override;
		}
		dst.weapon_anchor = src.weapon_anchor;
	}
	_on_param_changed();
	_refresh_shape_editor_context();
	return true;
}

bool ClockGearsDatabaseDialog::copy_shapes_from_direction(const String &p_from_dir, const String &p_mode) {
	String cur_slot = _get_active_anim_name();
	String prefix;
	int to_dir = 0;
	if (!s_parse_slot_dir_and_prefix(cur_slot, prefix, to_dir)) {
		return false;
	}

	String q = p_from_dir.strip_edges().to_lower();
	int from_dir = 0;
	static const int k_all_dirs[8] = { 2, 4, 6, 8, 1, 3, 7, 9 };
	for (int i = 0; i < 8; i++) {
		int d = k_all_dirs[i];
		if (ClockGearsCharacterStateRegistry::direction_to_suffix(d) == q || String::num_int64(d) == q) {
			from_dir = d;
			break;
		}
	}
	if (from_dir == 0 || from_dir == to_dir) {
		return false;
	}

	String src_slot = prefix + "_" + ClockGearsCharacterStateRegistry::direction_to_suffix(from_dir);
	if (!current_sprite_data.animations.has(src_slot) || !current_sprite_data.animations.has(cur_slot)) {
		return false;
	}
	s_sync_anim_frames_data(current_sprite_data.animations[src_slot]);
	s_sync_anim_frames_data(current_sprite_data.animations[cur_slot]);

	bool has_override_in_src = false;
	const ClockGearsSpriteAnimation &sa = current_sprite_data.animations[src_slot];
	for (int i = 0; i < sa.frames_data.size(); i++) {
		if (sa.frames_data[i].has_hurtbox_override) {
			has_override_in_src = true;
			break;
		}
	}

	Dictionary copied = ClockGearsShapeGeometry::copy_slot_shapes_between_directions(
			current_sprite_data.to_dict(),
			prefix,
			from_dir,
			to_dir,
			p_mode,
			true,
			has_override_in_src);
	current_sprite_data.from_dict(copied);

	_on_param_changed();
	_refresh_shape_editor_context();
	return true;
}

// ============================================================================
// Pestaña "Cuerpo y zonas" y Pestaña "Movimiento" (9A Tanda 3B - Interfaz)
// ============================================================================

static void s_style_db_action_button(Button *p_btn, const Color &p_accent = Color(0.28f, 0.56f, 0.92f)) {
	if (!p_btn) {
		return;
	}
	auto make_sb = [&](const Color &p_bg, const Color &p_bd) {
		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_anti_aliased(true);
		sb->set_bg_color(p_bg);
		sb->set_border_width_all(1);
		sb->set_border_color(p_bd);
		sb->set_corner_radius_all(8);
		sb->set_content_margin(SIDE_LEFT, 8.0f * EDSCALE);
		sb->set_content_margin(SIDE_RIGHT, 8.0f * EDSCALE);
		sb->set_content_margin(SIDE_TOP, 4.0f * EDSCALE);
		sb->set_content_margin(SIDE_BOTTOM, 4.0f * EDSCALE);
		return sb;
	};
	p_btn->add_theme_style_override("normal", make_sb(Color(0.18f, 0.21f, 0.28f, 0.94f), Color(0.32f, 0.37f, 0.48f, 0.85f)));
	p_btn->add_theme_style_override("hover", make_sb(Color(0.24f, 0.28f, 0.36f, 0.96f), p_accent));
	p_btn->add_theme_style_override("pressed", make_sb(Color(p_accent.r, p_accent.g, p_accent.b, 0.45f), p_accent));
	p_btn->add_theme_style_override("focus", make_sb(Color(0.18f, 0.21f, 0.28f, 0.94f), p_accent));
}

static String s_format_zone_action_summary(const Dictionary &p_act) {
	String t = String(p_act.get("type", "detenerse"));
	if (t == "perseguir") return CG_DB_TR("Perseguir");
	if (t == "huir") return CG_DB_TR("Huir");
	if (t == "volver_puesto") return CG_DB_TR("Volver al puesto");
	if (t == "mirar_jugador") return CG_DB_TR("Mirar al jugador");
	if (t == "detenerse") return CG_DB_TR("Detenerse");
	if (t == "atacar") return CG_DB_TR("Atacar");
	if (t == "ejecutar_evento") {
		if (p_act.has("event_page") && int(p_act["event_page"]) >= 1) {
			return vformat(CG_DB_TR("Ejecutar evento (Pág. %d)"), int(p_act["event_page"]));
		}
		return CG_DB_TR("Ejecutar evento");
	}
	if (t == "activar_interruptor") {
		String sw = String(p_act.get("switch_name", "")).strip_edges();
		bool v = p_act.has("value") ? bool(p_act["value"]) : true;
		return vformat(CG_DB_TR("Interruptor [%s] = %s"), sw.is_empty() ? CG_DB_TR("?") : sw, v ? "ON" : "OFF");
	}
	if (t == "poner_variable") {
		String vn = String(p_act.get("var_name", "")).strip_edges();
		String op = String(p_act.get("op", "="));
		int vv = int(p_act.get("value", 0));
		return vformat(CG_DB_TR("Variable [%s] %s %d"), vn.is_empty() ? CG_DB_TR("?") : vn, op, vv);
	}
	return t;
}

static String s_format_move_type_label(const String &p_code) {
	if (p_code == "quieto") return CG_DB_TR("Quieto");
	if (p_code == "vagar") return CG_DB_TR("Vagar");
	if (p_code == "patrulla") return CG_DB_TR("Patrulla");
	if (p_code == "seguir") return CG_DB_TR("Seguir al jugador");
	if (p_code == "persigue_y_ataca") return CG_DB_TR("Persigue y ataca");
	if (p_code == "huye") return CG_DB_TR("Huye");
	if (p_code == "a_distancia") return CG_DB_TR("A distancia");
	return p_code;
}

void ClockGearsDatabaseDialog::_setup_body_zones_tab_ui() {
	if (!main_editor_tabs) {
		return;
	}
	const float row_h = 30.0f * EDSCALE;

	// Pestaña 4: "Cuerpo y zonas"
	body_zones_tab_margin = memnew(MarginContainer);
	body_zones_tab_margin->set_name(CG_DB_TR("Cuerpo y zonas"));
	body_zones_tab_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	body_zones_tab_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	body_zones_tab_margin->add_theme_constant_override("margin_left", int(10 * EDSCALE));
	body_zones_tab_margin->add_theme_constant_override("margin_top", int(10 * EDSCALE));
	body_zones_tab_margin->add_theme_constant_override("margin_right", int(10 * EDSCALE));
	body_zones_tab_margin->add_theme_constant_override("margin_bottom", int(10 * EDSCALE));
	main_editor_tabs->add_child(body_zones_tab_margin);

	bz_empty_label = memnew(Label(CG_DB_TR("Crea o selecciona una entrada para configurar su cuerpo y zonas.")));
	bz_empty_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bz_empty_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	bz_empty_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	bz_empty_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	bz_empty_label->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
	bz_empty_label->set_visible(false);
	body_zones_tab_margin->add_child(bz_empty_label);

	body_zones_editor = memnew(ClockGearsShapeEditor);
	body_zones_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	body_zones_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	body_zones_editor->set_editor_mode(ClockGearsShapeEditor::MODE_BODY_ZONES);
	body_zones_editor->set_zoom(2);
	body_zones_editor->connect("body_zones_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_bz_editor_data_changed));
	body_zones_editor->connect("body_zone_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_bz_editor_zone_selected));
	body_zones_tab_margin->add_child(body_zones_editor);

	// Controles específicos de Zona montados dentro del inspector derecho reutilizado de ShapeEditor
	VBoxContainer *host = body_zones_editor->get_zone_props_host();
	zone_props_box = memnew(VBoxContainer);
	zone_props_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_props_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	if (host) {
		host->add_child(zone_props_box);
	}
	zone_props_box->add_child(memnew(HSeparator));

	Label *lbl_zp_hdr = memnew(Label(CG_DB_TR("Propiedades de la zona")));
	lbl_zp_hdr->add_theme_color_override("font_color", Color(0.95f, 0.80f, 0.42f));
	zone_props_box->add_child(lbl_zp_hdr);

	GridContainer *zp_grid = memnew(GridContainer);
	zp_grid->set_columns(2);
	zp_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zp_grid->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	zp_grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	zone_props_box->add_child(zp_grid);

	Label *lbl_zf = memnew(Label(CG_DB_TR("Quién la activa:")));
	zp_grid->add_child(lbl_zf);

	zone_filter_opt = memnew(OptionButton);
	zone_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_filter_opt->set_custom_minimum_size(Size2(0, row_h));
	zone_filter_opt->set_clip_text(true);
	zone_filter_opt->add_item(CG_DB_TR("Jugador"), 0);
	zone_filter_opt->set_item_metadata(0, "player");
	zone_filter_opt->add_item(CG_DB_TR("Enemigos"), 1);
	zone_filter_opt->set_item_metadata(1, "enemies");
	zone_filter_opt->add_item(CG_DB_TR("NPC"), 2);
	zone_filter_opt->set_item_metadata(2, "npc");
	zone_filter_opt->add_item(CG_DB_TR("Todos"), 3);
	zone_filter_opt->set_item_metadata(3, "all");
	zone_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_prop_changed));
	zp_grid->add_child(zone_filter_opt);

	Label *lbl_zt = memnew(Label(CG_DB_TR("Disparador:")));
	zp_grid->add_child(lbl_zt);

	zone_trigger_opt = memnew(OptionButton);
	zone_trigger_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_trigger_opt->set_custom_minimum_size(Size2(0, row_h));
	zone_trigger_opt->set_clip_text(true);
	zone_trigger_opt->add_item(CG_DB_TR("Al entrar"), 0);
	zone_trigger_opt->set_item_metadata(0, "enter");
	zone_trigger_opt->add_item(CG_DB_TR("Al salir"), 1);
	zone_trigger_opt->set_item_metadata(1, "exit");
	zone_trigger_opt->add_item(CG_DB_TR("Mientras está dentro"), 2);
	zone_trigger_opt->set_item_metadata(2, "while_inside");
	zone_trigger_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_prop_changed));
	zp_grid->add_child(zone_trigger_opt);

	zone_interval_row = memnew(HBoxContainer);
	zone_interval_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_interval_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	zone_interval_row->hide();
	zone_props_box->add_child(zone_interval_row);

	Label *lbl_ziv = memnew(Label(CG_DB_TR("Repetir cada (s):")));
	lbl_ziv->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_interval_row->add_child(lbl_ziv);

	zone_interval_spin = memnew(SpinBox);
	zone_interval_spin->set_min(0.1);
	zone_interval_spin->set_max(60.0);
	zone_interval_spin->set_step(0.1);
	zone_interval_spin->set_value(0.5);
	zone_interval_spin->set_custom_minimum_size(Size2(86 * EDSCALE, row_h));
	_bind_numeric_spinbox(zone_interval_spin);
	zone_interval_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_interval_changed));
	zone_interval_row->add_child(zone_interval_spin);

	zone_props_box->add_child(memnew(HSeparator));
	Label *lbl_zact = memnew(Label(CG_DB_TR("Acciones de la zona")));
	lbl_zact->add_theme_color_override("font_color", Color(0.60f, 0.88f, 0.72f));
	zone_props_box->add_child(lbl_zact);

	zone_actions_list = memnew(ItemList);
	zone_actions_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_actions_list->set_custom_minimum_size(Size2(0, 96 * EDSCALE));
	zone_actions_list->set_allow_reselect(true);
	zone_actions_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_action_list_selected));
	zone_props_box->add_child(zone_actions_list);

	HBoxContainer *act_btns = memnew(HBoxContainer);
	act_btns->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	act_btns->add_theme_constant_override("separation", int(4 * EDSCALE));
	zone_props_box->add_child(act_btns);

	btn_zone_act_add = memnew(Button("+"));
	btn_zone_act_add->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_zone_act_add->set_custom_minimum_size(Size2(0, row_h));
	s_style_db_action_button(btn_zone_act_add, Color(0.26f, 0.72f, 0.42f));
	btn_zone_act_add->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_add_pressed));
	act_btns->add_child(btn_zone_act_add);

	btn_zone_act_remove = memnew(Button("-"));
	btn_zone_act_remove->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_zone_act_remove->set_custom_minimum_size(Size2(0, row_h));
	s_style_db_action_button(btn_zone_act_remove, Color(0.88f, 0.32f, 0.32f));
	btn_zone_act_remove->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_remove_pressed));
	act_btns->add_child(btn_zone_act_remove);

	btn_zone_act_up = memnew(Button(CG_DB_TR("↑")));
	btn_zone_act_up->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_zone_act_up->set_custom_minimum_size(Size2(0, row_h));
	s_style_db_action_button(btn_zone_act_up);
	btn_zone_act_up->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_move_pressed).bind(-1));
	act_btns->add_child(btn_zone_act_up);

	btn_zone_act_down = memnew(Button(CG_DB_TR("↓")));
	btn_zone_act_down->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_zone_act_down->set_custom_minimum_size(Size2(0, row_h));
	s_style_db_action_button(btn_zone_act_down);
	btn_zone_act_down->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_move_pressed).bind(1));
	act_btns->add_child(btn_zone_act_down);

	// Sub-editor de la acción seleccionada
	zone_act_edit_box = memnew(VBoxContainer);
	zone_act_edit_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_edit_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	zone_act_edit_box->hide();
	zone_props_box->add_child(zone_act_edit_box);

	HBoxContainer *act_type_row = memnew(HBoxContainer);
	act_type_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	act_type_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	zone_act_edit_box->add_child(act_type_row);

	act_type_row->add_child(memnew(Label(CG_DB_TR("Acción:"))));
	zone_act_type_opt = memnew(OptionButton);
	zone_act_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_type_opt->set_custom_minimum_size(Size2(0, row_h));
	zone_act_type_opt->set_clip_text(true);
	PackedStringArray z_acts = ClockGearsDataManager::get_detection_zone_actions();
	for (int i = 0; i < z_acts.size(); i++) {
		Dictionary tmp;
		tmp["type"] = z_acts[i];
		zone_act_type_opt->add_item(s_format_zone_action_summary(tmp), i);
		zone_act_type_opt->set_item_metadata(i, z_acts[i]);
	}
	zone_act_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	act_type_row->add_child(zone_act_type_opt);

	// Selector de página de evento (para "ejecutar_evento")
	zone_act_event_row = memnew(HBoxContainer);
	zone_act_event_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_event_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	zone_act_event_row->hide();
	zone_act_edit_box->add_child(zone_act_event_row);

	zone_act_event_row->add_child(memnew(Label(CG_DB_TR("Página:"))));
	zone_act_event_page_opt = memnew(OptionButton);
	zone_act_event_page_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_event_page_opt->set_custom_minimum_size(Size2(0, row_h));
	zone_act_event_page_opt->set_clip_text(true);
	zone_act_event_page_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	zone_act_event_row->add_child(zone_act_event_page_opt);

	// Selector de interruptor (para "activar_interruptor")
	zone_act_switch_box = memnew(VBoxContainer);
	zone_act_switch_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_switch_box->add_theme_constant_override("separation", int(4 * EDSCALE));
	zone_act_switch_box->hide();
	zone_act_edit_box->add_child(zone_act_switch_box);

	zone_act_switch_picker = memnew(ClockGearsSwitchVarPickerButton(false));
	zone_act_switch_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_switch_picker->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_switch_picked));
	zone_act_switch_box->add_child(zone_act_switch_picker);

	HBoxContainer *sw_row2 = memnew(HBoxContainer);
	sw_row2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sw_row2->add_theme_constant_override("separation", int(6 * EDSCALE));
	zone_act_switch_box->add_child(sw_row2);

	zone_act_switch_name_edit = memnew(LineEdit);
	zone_act_switch_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_switch_name_edit->set_placeholder(CG_DB_TR("Nombre interruptor"));
	zone_act_switch_name_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	sw_row2->add_child(zone_act_switch_name_edit);

	zone_act_switch_val_opt = memnew(OptionButton);
	zone_act_switch_val_opt->set_custom_minimum_size(Size2(74 * EDSCALE, row_h));
	zone_act_switch_val_opt->add_item("ON", 1);
	zone_act_switch_val_opt->add_item("OFF", 0);
	zone_act_switch_val_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	sw_row2->add_child(zone_act_switch_val_opt);

	// Selector de variable (para "poner_variable")
	zone_act_var_box = memnew(VBoxContainer);
	zone_act_var_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_var_box->add_theme_constant_override("separation", int(4 * EDSCALE));
	zone_act_var_box->hide();
	zone_act_edit_box->add_child(zone_act_var_box);

	zone_act_var_picker = memnew(ClockGearsSwitchVarPickerButton(true));
	zone_act_var_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_var_picker->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_var_picked));
	zone_act_var_box->add_child(zone_act_var_picker);

	zone_act_var_name_edit = memnew(LineEdit);
	zone_act_var_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_var_name_edit->set_placeholder(CG_DB_TR("Nombre variable"));
	zone_act_var_name_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	zone_act_var_box->add_child(zone_act_var_name_edit);

	HBoxContainer *var_row2 = memnew(HBoxContainer);
	var_row2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	var_row2->add_theme_constant_override("separation", int(6 * EDSCALE));
	zone_act_var_box->add_child(var_row2);

	zone_act_var_op_opt = memnew(OptionButton);
	zone_act_var_op_opt->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
	zone_act_var_op_opt->add_item("=", 0);
	zone_act_var_op_opt->set_item_metadata(0, "=");
	zone_act_var_op_opt->add_item("+", 1);
	zone_act_var_op_opt->set_item_metadata(1, "+");
	zone_act_var_op_opt->add_item("-", 2);
	zone_act_var_op_opt->set_item_metadata(2, "-");
	zone_act_var_op_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	var_row2->add_child(zone_act_var_op_opt);

	zone_act_var_val_spin = memnew(SpinBox);
	zone_act_var_val_spin->set_min(-99999);
	zone_act_var_val_spin->set_max(99999);
	zone_act_var_val_spin->set_step(1);
	zone_act_var_val_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	zone_act_var_val_spin->set_custom_minimum_size(Size2(0, row_h));
	_bind_numeric_spinbox(zone_act_var_val_spin);
	zone_act_var_val_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_zone_act_detail_changed).unbind(1));
	var_row2->add_child(zone_act_var_val_spin);
}

void ClockGearsDatabaseDialog::_setup_movement_tab_ui() {
	if (!main_editor_tabs) {
		return;
	}
	const float row_h = 30.0f * EDSCALE;

	// Pestaña 5: "Movimiento" (NPC, NPC ARPG, Enemigos ARPG)
	movement_tab_margin = memnew(MarginContainer);
	movement_tab_margin->set_name(CG_DB_TR("Movimiento"));
	movement_tab_margin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	movement_tab_margin->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	movement_tab_margin->add_theme_constant_override("margin_left", int(10 * EDSCALE));
	movement_tab_margin->add_theme_constant_override("margin_top", int(10 * EDSCALE));
	movement_tab_margin->add_theme_constant_override("margin_right", int(10 * EDSCALE));
	movement_tab_margin->add_theme_constant_override("margin_bottom", int(10 * EDSCALE));
	main_editor_tabs->add_child(movement_tab_margin);

	mov_empty_label = memnew(Label(CG_DB_TR("Crea o selecciona una entrada para configurar su movimiento.")));
	mov_empty_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_empty_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	mov_empty_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	mov_empty_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	mov_empty_label->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
	mov_empty_label->set_visible(false);
	movement_tab_margin->add_child(mov_empty_label);

	movement_tab_hbox = memnew(HBoxContainer);
	movement_tab_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	movement_tab_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	movement_tab_hbox->add_theme_constant_override("separation", int(12 * EDSCALE));
	movement_tab_margin->add_child(movement_tab_hbox);

	// Columna izquierda: Parámetros de movimiento con scroll vertical y cero scroll horizontal
	mov_left_scroll = memnew(ScrollContainer);
	mov_left_scroll->set_custom_minimum_size(Size2(304 * EDSCALE, 0));
	mov_left_scroll->set_h_size_flags(Control::SIZE_FILL);
	mov_left_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	mov_left_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	mov_left_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	movement_tab_hbox->add_child(mov_left_scroll);

	mov_left_vbox = memnew(VBoxContainer);
	mov_left_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_left_vbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	mov_left_scroll->add_child(mov_left_vbox);

	Label *lbl_mtype = memnew(Label(CG_DB_TR("Tipo de movimiento")));
	lbl_mtype->add_theme_color_override("font_color", Color(0.55f, 0.85f, 1.0f));
	mov_left_vbox->add_child(lbl_mtype);

	mov_type_opt = memnew(OptionButton);
	mov_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_type_opt->set_custom_minimum_size(Size2(0, row_h));
	mov_type_opt->set_clip_text(true);
	mov_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_type_selected));
	mov_left_vbox->add_child(mov_type_opt);

	// Sección Vagar
	mov_wander_box = memnew(VBoxContainer);
	mov_wander_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_wander_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	mov_left_vbox->add_child(mov_wander_box);

	HBoxContainer *w_row = memnew(HBoxContainer);
	w_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	w_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_wander_box->add_child(w_row);

	Label *lbl_wr = memnew(Label(CG_DB_TR("Radio de vagar (casillas):")));
	lbl_wr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_wr->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	w_row->add_child(lbl_wr);

	mov_wander_spin = memnew(SpinBox);
	mov_wander_spin->set_min(0.0);
	mov_wander_spin->set_max(50.0);
	mov_wander_spin->set_step(0.5);
	mov_wander_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	_bind_numeric_spinbox(mov_wander_spin);
	mov_wander_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed));
	w_row->add_child(mov_wander_spin);

	// Sección Patrulla
	mov_patrol_box = memnew(VBoxContainer);
	mov_patrol_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_patrol_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	mov_left_vbox->add_child(mov_patrol_box);

	HBoxContainer *pm_row = memnew(HBoxContainer);
	pm_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pm_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_patrol_box->add_child(pm_row);

	pm_row->add_child(memnew(Label(CG_DB_TR("Modo de ruta:"))));
	mov_patrol_mode_opt = memnew(OptionButton);
	mov_patrol_mode_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_patrol_mode_opt->set_custom_minimum_size(Size2(0, row_h));
	mov_patrol_mode_opt->add_item(CG_DB_TR("Ida y vuelta"), 0);
	mov_patrol_mode_opt->add_item(CG_DB_TR("Bucle cerrado"), 1);
	mov_patrol_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed).unbind(1).bind(0.0));
	pm_row->add_child(mov_patrol_mode_opt);

	mov_patrol_box->add_child(memnew(Label(CG_DB_TR("Puntos de ruta (casillas):"))));
	mov_patrol_list = memnew(ItemList);
	mov_patrol_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_patrol_list->set_custom_minimum_size(Size2(0, 112 * EDSCALE));
	mov_patrol_list->set_allow_reselect(true);
	mov_patrol_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_patrol_list_selected));
	mov_patrol_box->add_child(mov_patrol_list);

	HBoxContainer *p_btns = memnew(HBoxContainer);
	p_btns->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	p_btns->add_theme_constant_override("separation", int(4 * EDSCALE));
	mov_patrol_box->add_child(p_btns);

	btn_mov_patrol_add = memnew(Button(CG_DB_TR("+ Punto")));
	btn_mov_patrol_add->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_mov_patrol_add->set_custom_minimum_size(Size2(0, row_h));
	s_style_db_action_button(btn_mov_patrol_add, Color(0.26f, 0.72f, 0.42f));
	btn_mov_patrol_add->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_patrol_add_pressed));
	p_btns->add_child(btn_mov_patrol_add);

	btn_mov_patrol_remove = memnew(Button("-"));
	btn_mov_patrol_remove->set_custom_minimum_size(Size2(36 * EDSCALE, row_h));
	s_style_db_action_button(btn_mov_patrol_remove, Color(0.88f, 0.32f, 0.32f));
	btn_mov_patrol_remove->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_patrol_remove_pressed));
	p_btns->add_child(btn_mov_patrol_remove);

	btn_mov_patrol_up = memnew(Button(CG_DB_TR("↑")));
	btn_mov_patrol_up->set_custom_minimum_size(Size2(36 * EDSCALE, row_h));
	s_style_db_action_button(btn_mov_patrol_up);
	btn_mov_patrol_up->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_patrol_move_pressed).bind(-1));
	p_btns->add_child(btn_mov_patrol_up);

	btn_mov_patrol_down = memnew(Button(CG_DB_TR("↓")));
	btn_mov_patrol_down->set_custom_minimum_size(Size2(36 * EDSCALE, row_h));
	s_style_db_action_button(btn_mov_patrol_down);
	btn_mov_patrol_down->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_patrol_move_pressed).bind(1));
	p_btns->add_child(btn_mov_patrol_down);

	// Sección Seguir (NPC / NPC ARPG)
	mov_follow_box = memnew(VBoxContainer);
	mov_follow_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_follow_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	mov_left_vbox->add_child(mov_follow_box);

	HBoxContainer *f_row = memnew(HBoxContainer);
	f_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	f_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_follow_box->add_child(f_row);

	Label *lbl_fd = memnew(Label(CG_DB_TR("Distancia al jugador (casillas):")));
	lbl_fd->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_fd->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	f_row->add_child(lbl_fd);

	mov_follow_dist_spin = memnew(SpinBox);
	mov_follow_dist_spin->set_min(0.5);
	mov_follow_dist_spin->set_max(20.0);
	mov_follow_dist_spin->set_step(0.5);
	mov_follow_dist_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	_bind_numeric_spinbox(mov_follow_dist_spin);
	mov_follow_dist_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed));
	f_row->add_child(mov_follow_dist_spin);

	// Mirar al jugador al hablar (NPC / NPC ARPG)
	mov_face_on_event_row = memnew(HBoxContainer);
	mov_face_on_event_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_left_vbox->add_child(mov_face_on_event_row);

	mov_face_on_event_chk = memnew(CheckBox(CG_DB_TR("Mirar al jugador al hablar")));
	mov_face_on_event_chk->set_pressed(true);
	mov_face_on_event_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_bool_toggled));
	mov_face_on_event_row->add_child(mov_face_on_event_chk);

	// Sección Persecución / Huida (Enemigos ARPG)
	mov_chase_box = memnew(VBoxContainer);
	mov_chase_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_chase_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	mov_left_vbox->add_child(mov_chase_box);

	HBoxContainer *ls_row = memnew(HBoxContainer);
	ls_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ls_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_chase_box->add_child(ls_row);

	Label *lbl_ls = memnew(Label(CG_DB_TR("Perder de vista tras (s):")));
	lbl_ls->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_ls->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	ls_row->add_child(lbl_ls);

	mov_lose_sight_spin = memnew(SpinBox);
	mov_lose_sight_spin->set_min(0.0);
	mov_lose_sight_spin->set_max(60.0);
	mov_lose_sight_spin->set_step(0.5);
	mov_lose_sight_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	_bind_numeric_spinbox(mov_lose_sight_spin);
	mov_lose_sight_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed));
	ls_row->add_child(mov_lose_sight_spin);

	// Sección A distancia (Enemigos ARPG)
	mov_ranged_box = memnew(VBoxContainer);
	mov_ranged_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mov_ranged_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	mov_left_vbox->add_child(mov_ranged_box);

	HBoxContainer *mn_row = memnew(HBoxContainer);
	mn_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mn_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_ranged_box->add_child(mn_row);

	Label *lbl_mn = memnew(Label(CG_DB_TR("Distancia mínima (casillas):")));
	lbl_mn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_mn->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	mn_row->add_child(lbl_mn);

	mov_min_dist_spin = memnew(SpinBox);
	mov_min_dist_spin->set_min(0.0);
	mov_min_dist_spin->set_max(30.0);
	mov_min_dist_spin->set_step(0.5);
	mov_min_dist_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	_bind_numeric_spinbox(mov_min_dist_spin);
	mov_min_dist_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed));
	mn_row->add_child(mov_min_dist_spin);

	HBoxContainer *mx_row = memnew(HBoxContainer);
	mx_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mx_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_ranged_box->add_child(mx_row);

	Label *lbl_mx = memnew(Label(CG_DB_TR("Distancia máxima (casillas):")));
	lbl_mx->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_mx->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	mx_row->add_child(lbl_mx);

	mov_max_dist_spin = memnew(SpinBox);
	mov_max_dist_spin->set_min(0.5);
	mov_max_dist_spin->set_max(40.0);
	mov_max_dist_spin->set_step(0.5);
	mov_max_dist_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	_bind_numeric_spinbox(mov_max_dist_spin);
	mov_max_dist_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed));
	mx_row->add_child(mov_max_dist_spin);

	// Sección Navegación A*
	mov_left_vbox->add_child(memnew(HSeparator));
	Label *lbl_nav = memnew(Label(CG_DB_TR("Navegación y obstáculos")));
	lbl_nav->add_theme_color_override("font_color", Color(0.55f, 0.85f, 1.0f));
	mov_left_vbox->add_child(lbl_nav);

	mov_avoid_obstacles_chk = memnew(CheckBox(CG_DB_TR("Rodear obstáculos")));
	mov_avoid_obstacles_chk->set_pressed(true);
	mov_avoid_obstacles_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_bool_toggled));
	mov_left_vbox->add_child(mov_avoid_obstacles_chk);

	HBoxContainer *rc_row = memnew(HBoxContainer);
	rc_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rc_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	mov_left_vbox->add_child(rc_row);

	Label *lbl_rc = memnew(Label(CG_DB_TR("Intervalo de recálculo (ms):")));
	lbl_rc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_rc->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	rc_row->add_child(lbl_rc);

	mov_recalc_ms_spin = memnew(SpinBox);
	mov_recalc_ms_spin->set_min(50);
	mov_recalc_ms_spin->set_max(5000);
	mov_recalc_ms_spin->set_step(25);
	mov_recalc_ms_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	_bind_numeric_spinbox(mov_recalc_ms_spin);
	mov_recalc_ms_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_mov_field_changed));
	rc_row->add_child(mov_recalc_ms_spin);

	// Columna derecha: Lienzo reutilizado ClockGearsShapeEditor en MODE_MOVEMENT
	movement_shape_editor = memnew(ClockGearsShapeEditor);
	movement_shape_editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	movement_shape_editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	movement_shape_editor->set_editor_mode(ClockGearsShapeEditor::MODE_MOVEMENT);
	movement_shape_editor->set_zoom(1);
	movement_shape_editor->connect("movement_visual_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_movement_visual_changed));
	movement_tab_hbox->add_child(movement_shape_editor);
}

void ClockGearsDatabaseDialog::_resolve_entity_idle_preview(const Dictionary &p_rec, Ref<Texture2D> &r_tex, Rect2i &r_region) const {
	r_tex = Ref<Texture2D>();
	r_region = Rect2i(0, 0, 32, 32);

	static const char *k_idle_candidates[6] = {
		"idle", "idle_abajo", "mover_abajo", "mover_izquierda", "mover_derecha", "mover_arriba"
	};
	String chosen_slot;
	for (int i = 0; i < 6; i++) {
		String sk = k_idle_candidates[i];
		if (current_sprite_data.animations.has(sk) && current_sprite_data.animations[sk].get_frame_count() > 0) {
			chosen_slot = sk;
			break;
		}
	}
	if (chosen_slot.is_empty()) {
		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : current_sprite_data.animations) {
			if (E.value.get_frame_count() > 0) {
				chosen_slot = E.key;
				break;
			}
		}
	}
	if (!chosen_slot.is_empty()) {
		const ClockGearsSpriteAnimation &anim = current_sprite_data.animations[chosen_slot];
		String spath = anim.spritesheet_path;
		if (!anim.frames_data.is_empty() && !anim.frames_data[0].sheet_path.is_empty()) {
			spath = anim.frames_data[0].sheet_path;
		}
		if (spath.is_empty()) {
			spath = String(p_rec.get("graphic_path", ""));
		}
		r_tex = _load_texture_from_path(spath);
		int tw = r_tex.is_valid() ? r_tex->get_width() : 0;
		int th = r_tex.is_valid() ? r_tex->get_height() : 0;
		ClockGearsAnimationFrame fr = anim.get_frame(0, tw, th);
		if (fr.region.size.x > 0 && fr.region.size.y > 0) {
			r_region = fr.region;
		} else if (r_tex.is_valid()) {
			r_region = Rect2i(Point2i(), r_tex->get_size());
		}
		return;
	}

	String gp = String(p_rec.get("graphic_path", ""));
	if (!gp.is_empty()) {
		r_tex = _load_texture_from_path(gp);
		if (r_tex.is_valid()) {
			r_region = Rect2i(Point2i(), r_tex->get_size());
		}
	}
}

void ClockGearsDatabaseDialog::_sync_entity_preview_textures() {
	int idx = _db_selected_record_index(record_list);
	if (idx < 0 || current_category < 0 || current_category >= CAT_MAX || idx >= category_data[current_category].size()) {
		return;
	}
	Dictionary d = category_data[current_category][idx];
	Ref<Texture2D> tex;
	Rect2i reg;
	_resolve_entity_idle_preview(d, tex, reg);
	if (body_zones_editor) {
		body_zones_editor->set_preview_texture(tex, reg);
	}
	if (movement_shape_editor) {
		movement_shape_editor->set_preview_texture(tex, reg);
	}
}

void ClockGearsDatabaseDialog::_load_body_zones_tab_from_record(const Dictionary &p_rec) {
	if (!body_zones_editor) {
		return;
	}
	if (bz_empty_label) {
		bz_empty_label->set_visible(false);
	}
	body_zones_editor->set_visible(true);

	bool allow_zones = (current_category != CAT_ACTORS);
	body_zones_editor->set_body_zones_config(allow_zones);

	Ref<Texture2D> tex;
	Rect2i reg;
	_resolve_entity_idle_preview(p_rec, tex, reg);
	body_zones_editor->set_preview_texture(tex, reg);

	Dictionary bshape = p_rec.get("body_shape", Dictionary());
	bool bthrough = bool(p_rec.get("body_through", false));
	Array zones = allow_zones ? Array(p_rec.get("detection_zones", Array())) : Array();

	updating_bz_ui = true;
	body_zones_editor->set_body_zones_data(bshape, bthrough, zones);
	updating_bz_ui = false;
	_refresh_zone_props_ui();
}

void ClockGearsDatabaseDialog::_refresh_zone_props_ui() {
	if (!body_zones_editor || !zone_props_box) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	if (zidx < 0 || current_category == CAT_ACTORS) {
		zone_props_box->set_visible(false);
		return;
	}
	zone_props_box->set_visible(true);
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	updating_bz_ui = true;

	String tf = String(zd.get("target_filter", "player")).strip_edges().to_lower();
	if (zone_filter_opt) {
		int sel_f = 0;
		for (int i = 0; i < zone_filter_opt->get_item_count(); i++) {
			if (String(zone_filter_opt->get_item_metadata(i)) == tf) {
				sel_f = i;
				break;
			}
		}
		zone_filter_opt->select(sel_f);
	}

	String trig = String(zd.get("trigger", "enter")).strip_edges().to_lower();
	if (zone_trigger_opt) {
		int sel_t = 0;
		for (int i = 0; i < zone_trigger_opt->get_item_count(); i++) {
			if (String(zone_trigger_opt->get_item_metadata(i)) == trig) {
				sel_t = i;
				break;
			}
		}
		zone_trigger_opt->select(sel_t);
	}
	if (zone_interval_row) {
		zone_interval_row->set_visible(trig == "while_inside");
	}
	if (zone_interval_spin) {
		double iv = MAX(0.1, double(zd.get("interval_sec", 0.5)));
		zone_interval_spin->set_value(iv);
		zone_interval_spin->set_meta("cg_last_valid_val", iv);
	}

	int prev_sel_act = -1;
	if (zone_actions_list) {
		Vector<int> prev_s = zone_actions_list->get_selected_items();
		if (!prev_s.is_empty()) {
			prev_sel_act = prev_s[0];
		}
		zone_actions_list->clear();
		Array acts = zd.get("actions", Array());
		for (int i = 0; i < acts.size(); i++) {
			if (acts[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary ad = acts[i];
			zone_actions_list->add_item(vformat("%d. %s", zone_actions_list->get_item_count() + 1, s_format_zone_action_summary(ad)));
		}
		int n_acts = zone_actions_list->get_item_count();
		if (n_acts > 0) {
			int pick = CLAMP(prev_sel_act >= 0 ? prev_sel_act : 0, 0, n_acts - 1);
			zone_actions_list->select(pick);
		}
	}

	updating_bz_ui = false;
	_refresh_zone_action_detail_ui();
}

void ClockGearsDatabaseDialog::_refresh_zone_action_detail_ui() {
	if (!body_zones_editor || !zone_actions_list || !zone_act_edit_box) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	Vector<int> sel = zone_actions_list->get_selected_items();
	if (zidx < 0 || sel.is_empty()) {
		zone_act_edit_box->set_visible(false);
		if (btn_zone_act_remove) btn_zone_act_remove->set_disabled(true);
		if (btn_zone_act_up) btn_zone_act_up->set_disabled(true);
		if (btn_zone_act_down) btn_zone_act_down->set_disabled(true);
		return;
	}
	int aidx = sel[0];
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	Array acts = zd.get("actions", Array());
	if (aidx < 0 || aidx >= acts.size() || acts[aidx].get_type() != Variant::DICTIONARY) {
		zone_act_edit_box->set_visible(false);
		return;
	}
	if (btn_zone_act_remove) btn_zone_act_remove->set_disabled(false);
	if (btn_zone_act_up) btn_zone_act_up->set_disabled(aidx <= 0);
	if (btn_zone_act_down) btn_zone_act_down->set_disabled(aidx >= acts.size() - 1);

	Dictionary ad = acts[aidx];
	String atype = String(ad.get("type", "detenerse"));
	zone_act_edit_box->set_visible(true);

	updating_bz_ui = true;
	if (zone_act_type_opt) {
		int sel_t = 0;
		for (int i = 0; i < zone_act_type_opt->get_item_count(); i++) {
			if (String(zone_act_type_opt->get_item_metadata(i)) == atype) {
				sel_t = i;
				break;
			}
		}
		zone_act_type_opt->select(sel_t);
	}

	bool is_ev = (atype == "ejecutar_evento");
	bool is_sw = (atype == "activar_interruptor");
	bool is_vr = (atype == "poner_variable");

	if (zone_act_event_row) zone_act_event_row->set_visible(is_ev);
	if (zone_act_switch_box) zone_act_switch_box->set_visible(is_sw);
	if (zone_act_var_box) zone_act_var_box->set_visible(is_vr);

	if (is_ev && zone_act_event_page_opt) {
		zone_act_event_page_opt->clear();
		zone_act_event_page_opt->add_item(CG_DB_TR("Página activa (condiciones)"), 0);
		int rec_idx = _db_selected_record_index(record_list);
		if (rec_idx >= 0 && current_category >= 0 && current_category < CAT_MAX && rec_idx < category_data[current_category].size()) {
			Dictionary rec = category_data[current_category][rec_idx];
			Array pages = rec.get("event_pages", Array());
			for (int p = 0; p < pages.size(); p++) {
				zone_act_event_page_opt->add_item(vformat(CG_DB_TR("Página %d"), p + 1), p + 1);
			}
		}
		int ep = int(ad.get("event_page", 0));
		int sel_ep = 0;
		for (int i = 0; i < zone_act_event_page_opt->get_item_count(); i++) {
			if (zone_act_event_page_opt->get_item_id(i) == ep) {
				sel_ep = i;
				break;
			}
		}
		zone_act_event_page_opt->select(sel_ep);
	}

	if (is_sw) {
		String sname = String(ad.get("switch_name", ""));
		if (zone_act_switch_name_edit && zone_act_switch_name_edit->get_text() != sname) {
			zone_act_switch_name_edit->set_text(sname);
		}
		if (zone_act_switch_picker) {
			zone_act_switch_picker->set_selected_name(sname);
		}
		if (zone_act_switch_val_opt) {
			bool sval = ad.has("value") ? bool(ad["value"]) : true;
			zone_act_switch_val_opt->select(sval ? 0 : 1);
		}
	}

	if (is_vr) {
		String vname = String(ad.get("var_name", ""));
		if (zone_act_var_name_edit && zone_act_var_name_edit->get_text() != vname) {
			zone_act_var_name_edit->set_text(vname);
		}
		if (zone_act_var_picker) {
			zone_act_var_picker->set_selected_name(vname);
		}
		if (zone_act_var_op_opt) {
			String op = String(ad.get("op", "="));
			int sel_op = 0;
			for (int i = 0; i < zone_act_var_op_opt->get_item_count(); i++) {
				if (String(zone_act_var_op_opt->get_item_metadata(i)) == op) {
					sel_op = i;
					break;
				}
			}
			zone_act_var_op_opt->select(sel_op);
		}
		if (zone_act_var_val_spin) {
			double vv = double(int(ad.get("value", 0)));
			zone_act_var_val_spin->set_value(vv);
			zone_act_var_val_spin->set_meta("cg_last_valid_val", vv);
		}
	}

	updating_bz_ui = false;
}

void ClockGearsDatabaseDialog::_commit_zone_metadata_from_ui() {
	if (!body_zones_editor) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	if (zidx < 0) {
		return;
	}
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	if (zone_filter_opt && zone_filter_opt->get_selected() >= 0) {
		zd["target_filter"] = String(zone_filter_opt->get_selected_metadata());
	}
	if (zone_trigger_opt && zone_trigger_opt->get_selected() >= 0) {
		String trig = String(zone_trigger_opt->get_selected_metadata());
		zd["trigger"] = trig;
		if (zone_interval_row) {
			zone_interval_row->set_visible(trig == "while_inside");
		}
	}
	if (zone_interval_spin) {
		zd["interval_sec"] = MAX(0.1, double(zone_interval_spin->get_value()));
	}
	updating_bz_ui = true;
	body_zones_editor->set_selected_zone_metadata(zidx, zd);
	updating_bz_ui = false;
	_commit_body_zones_to_record();
}

void ClockGearsDatabaseDialog::_commit_body_zones_to_record() {
	if (!body_zones_editor || !s_has_body_zones_tab_category(current_category)) {
		return;
	}
	int rec_idx = _db_selected_record_index(record_list);
	if (rec_idx < 0 || rec_idx >= category_data[current_category].size()) {
		return;
	}
	Dictionary d = category_data[current_category][rec_idx];
	d["body_shape"] = body_zones_editor->get_body_shape_data();
	d["body_through"] = body_zones_editor->is_body_through();
	if (current_category != CAT_ACTORS) {
		d["detection_zones"] = body_zones_editor->get_zones_data();
	}
	ClockGearsDataManager::normalize_record_defaults(current_category, d);
	category_data[current_category][rec_idx] = d;
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_bz_editor_data_changed() {
	if (updating_bz_ui || updating_ui) {
		return;
	}
	_commit_body_zones_to_record();
}

void ClockGearsDatabaseDialog::_on_bz_editor_zone_selected(int p_zone_idx) {
	if (updating_bz_ui) {
		return;
	}
	_refresh_zone_props_ui();
}

void ClockGearsDatabaseDialog::_on_zone_prop_changed(int p_unused) {
	if (updating_bz_ui || updating_ui) {
		return;
	}
	_commit_zone_metadata_from_ui();
}

void ClockGearsDatabaseDialog::_on_zone_interval_changed(double p_val) {
	if (updating_bz_ui || updating_ui) {
		return;
	}
	_commit_zone_metadata_from_ui();
}

void ClockGearsDatabaseDialog::_on_zone_action_list_selected(int p_idx) {
	if (updating_bz_ui) {
		return;
	}
	_refresh_zone_action_detail_ui();
}

void ClockGearsDatabaseDialog::_on_zone_act_add_pressed() {
	if (!body_zones_editor || !zone_actions_list) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	if (zidx < 0) {
		return;
	}
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	Array acts = Array(zd.get("actions", Array())).duplicate(true);
	Dictionary na;
	na["type"] = "perseguir";
	acts.push_back(na);
	zd["actions"] = acts;
	updating_bz_ui = true;
	body_zones_editor->set_selected_zone_metadata(zidx, zd);
	updating_bz_ui = false;
	_refresh_zone_props_ui();
	if (zone_actions_list->get_item_count() > 0) {
		zone_actions_list->select(zone_actions_list->get_item_count() - 1);
		_refresh_zone_action_detail_ui();
	}
	_commit_body_zones_to_record();
}

void ClockGearsDatabaseDialog::_on_zone_act_remove_pressed() {
	if (!body_zones_editor || !zone_actions_list) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	Vector<int> sel = zone_actions_list->get_selected_items();
	if (zidx < 0 || sel.is_empty()) {
		return;
	}
	int aidx = sel[0];
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	Array acts = Array(zd.get("actions", Array())).duplicate(true);
	if (aidx < 0 || aidx >= acts.size()) {
		return;
	}
	acts.remove_at(aidx);
	zd["actions"] = acts;
	updating_bz_ui = true;
	body_zones_editor->set_selected_zone_metadata(zidx, zd);
	updating_bz_ui = false;
	_refresh_zone_props_ui();
	if (zone_actions_list->get_item_count() > 0) {
		zone_actions_list->select(CLAMP(aidx, 0, zone_actions_list->get_item_count() - 1));
		_refresh_zone_action_detail_ui();
	}
	_commit_body_zones_to_record();
}

void ClockGearsDatabaseDialog::_on_zone_act_move_pressed(int p_delta) {
	if (!body_zones_editor || !zone_actions_list) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	Vector<int> sel = zone_actions_list->get_selected_items();
	if (zidx < 0 || sel.is_empty()) {
		return;
	}
	int aidx = sel[0];
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	Array acts = Array(zd.get("actions", Array())).duplicate(true);
	int target = aidx + p_delta;
	if (aidx < 0 || aidx >= acts.size() || target < 0 || target >= acts.size()) {
		return;
	}
	Variant tmp = acts[aidx];
	acts[aidx] = acts[target];
	acts[target] = tmp;
	zd["actions"] = acts;
	updating_bz_ui = true;
	body_zones_editor->set_selected_zone_metadata(zidx, zd);
	updating_bz_ui = false;
	_refresh_zone_props_ui();
	if (target >= 0 && target < zone_actions_list->get_item_count()) {
		zone_actions_list->select(target);
		_refresh_zone_action_detail_ui();
	}
	_commit_body_zones_to_record();
}

void ClockGearsDatabaseDialog::_on_zone_act_detail_changed() {
	if (updating_bz_ui || updating_ui || !body_zones_editor || !zone_actions_list) {
		return;
	}
	int zidx = body_zones_editor->get_selected_bz_index();
	Vector<int> sel = zone_actions_list->get_selected_items();
	if (zidx < 0 || sel.is_empty()) {
		return;
	}
	int aidx = sel[0];
	Dictionary zd = body_zones_editor->get_selected_zone_metadata();
	Array acts = Array(zd.get("actions", Array())).duplicate(true);
	if (aidx < 0 || aidx >= acts.size()) {
		return;
	}
	Dictionary ad = (acts[aidx].get_type() == Variant::DICTIONARY) ? Dictionary(acts[aidx]).duplicate(true) : Dictionary();
	String prev_type = String(ad.get("type", ""));
	String new_type = (zone_act_type_opt && zone_act_type_opt->get_selected() >= 0)
			? String(zone_act_type_opt->get_selected_metadata())
			: prev_type;
	ad["type"] = new_type;

	if (new_type == "ejecutar_evento" && zone_act_event_page_opt && zone_act_event_page_opt->get_selected() >= 0) {
		int pid = zone_act_event_page_opt->get_selected_id();
		if (pid > 0) {
			ad["event_page"] = pid;
		} else {
			ad.erase("event_page");
		}
	} else if (new_type == "activar_interruptor") {
		if (zone_act_switch_name_edit) {
			ad["switch_name"] = zone_act_switch_name_edit->get_text();
		}
		if (zone_act_switch_val_opt) {
			ad["value"] = (zone_act_switch_val_opt->get_selected_id() != 0);
		}
	} else if (new_type == "poner_variable") {
		if (zone_act_var_name_edit) {
			ad["var_name"] = zone_act_var_name_edit->get_text();
		}
		if (zone_act_var_op_opt && zone_act_var_op_opt->get_selected() >= 0) {
			ad["op"] = String(zone_act_var_op_opt->get_selected_metadata());
		}
		if (zone_act_var_val_spin) {
			ad["value"] = int(zone_act_var_val_spin->get_value());
		}
	}

	acts[aidx] = ad;
	zd["actions"] = acts;
	updating_bz_ui = true;
	body_zones_editor->set_selected_zone_metadata(zidx, zd);
	if (zone_actions_list && aidx >= 0 && aidx < zone_actions_list->get_item_count()) {
		zone_actions_list->set_item_text(aidx, vformat("%d. %s", aidx + 1, s_format_zone_action_summary(ad)));
	}
	updating_bz_ui = false;
	if (prev_type != new_type) {
		_refresh_zone_action_detail_ui();
	}
	_commit_body_zones_to_record();
}

void ClockGearsDatabaseDialog::_on_zone_act_switch_picked(int p_id, const String &p_name) {
	if (updating_bz_ui) {
		return;
	}
	String chosen = p_name.strip_edges();
	if (chosen.is_empty() && p_id > 0) {
		chosen = itos(p_id);
	}
	if (zone_act_switch_name_edit) {
		zone_act_switch_name_edit->set_text(chosen);
	}
	_on_zone_act_detail_changed();
}

void ClockGearsDatabaseDialog::_on_zone_act_var_picked(int p_id, const String &p_name) {
	if (updating_bz_ui) {
		return;
	}
	String chosen = p_name.strip_edges();
	if (chosen.is_empty() && p_id > 0) {
		chosen = itos(p_id);
	}
	if (zone_act_var_name_edit) {
		zone_act_var_name_edit->set_text(chosen);
	}
	_on_zone_act_detail_changed();
}

void ClockGearsDatabaseDialog::_load_movement_tab_from_record(const Dictionary &p_rec) {
	if (!movement_tab_hbox || !mov_type_opt) {
		return;
	}
	if (mov_empty_label) {
		mov_empty_label->set_visible(false);
	}
	movement_tab_hbox->set_visible(true);

	updating_mov_ui = true;
	bool is_enemy = (current_category == CAT_ARPG_ENEMIES);
	PackedStringArray modes = is_enemy
			? ClockGearsDataManager::get_arpg_ai_types()
			: ClockGearsDataManager::get_npc_move_types();

	mov_type_opt->clear();
	String cur_mode = is_enemy
			? String(p_rec.get("ai_type", "persigue_y_ataca")).strip_edges().to_lower()
			: String(p_rec.get("npc_move_type", "quieto")).strip_edges().to_lower();
	int sel_m = 0;
	for (int i = 0; i < modes.size(); i++) {
		mov_type_opt->add_item(s_format_move_type_label(modes[i]), i);
		mov_type_opt->set_item_metadata(i, modes[i]);
		if (modes[i] == cur_mode) {
			sel_m = i;
		}
	}
	if (mov_type_opt->get_item_count() > 0) {
		mov_type_opt->select(sel_m);
	}

	if (mov_wander_spin) {
		double wr = MAX(0.0, double(p_rec.get("wander_radius", 3.0)));
		mov_wander_spin->set_value(wr);
		mov_wander_spin->set_meta("cg_last_valid_val", wr);
	}
	if (mov_patrol_mode_opt) {
		mov_patrol_mode_opt->select(bool(p_rec.get("patrol_loop", false)) ? 1 : 0);
	}
	if (mov_follow_dist_spin) {
		double fd = MAX(0.5, double(p_rec.get("follow_distance", 2.0)));
		mov_follow_dist_spin->set_value(fd);
		mov_follow_dist_spin->set_meta("cg_last_valid_val", fd);
	}
	if (mov_face_on_event_chk) {
		mov_face_on_event_chk->set_pressed(p_rec.has("face_player_on_event") ? bool(p_rec["face_player_on_event"]) : true);
	}
	if (mov_lose_sight_spin) {
		double ls = MAX(0.0, double(p_rec.get("lose_sight_sec", 4.0)));
		mov_lose_sight_spin->set_value(ls);
		mov_lose_sight_spin->set_meta("cg_last_valid_val", ls);
	}
	if (mov_min_dist_spin) {
		double mn = MAX(0.0, double(p_rec.get("min_distance", 3.0)));
		mov_min_dist_spin->set_value(mn);
		mov_min_dist_spin->set_meta("cg_last_valid_val", mn);
	}
	if (mov_max_dist_spin) {
		double mn = MAX(0.0, double(p_rec.get("min_distance", 3.0)));
		double mx = MAX(mn, double(p_rec.get("max_distance", 5.0)));
		mov_max_dist_spin->set_value(mx);
		mov_max_dist_spin->set_meta("cg_last_valid_val", mx);
	}
	if (mov_avoid_obstacles_chk) {
		mov_avoid_obstacles_chk->set_pressed(p_rec.has("avoid_obstacles") ? bool(p_rec["avoid_obstacles"]) : true);
	}
	if (mov_recalc_ms_spin) {
		double rc = MAX(50.0, double(p_rec.get("path_recalc_ms", 250)));
		mov_recalc_ms_spin->set_value(rc);
		mov_recalc_ms_spin->set_meta("cg_last_valid_val", rc);
	}

	if (movement_shape_editor) {
		Ref<Texture2D> tex;
		Rect2i reg;
		_resolve_entity_idle_preview(p_rec, tex, reg);
		movement_shape_editor->set_preview_texture(tex, reg);
		movement_shape_editor->set_movement_visual_data(
				cur_mode,
				double(p_rec.get("wander_radius", 3.0)),
				p_rec.get("patrol_points", Array()),
				bool(p_rec.get("patrol_loop", false)));
	}

	updating_mov_ui = false;
	_update_movement_contextual_visibility(cur_mode);
	_refresh_patrol_points_list();
}

void ClockGearsDatabaseDialog::_update_movement_contextual_visibility(const String &p_mode) {
	bool is_enemy = (current_category == CAT_ARPG_ENEMIES);
	bool is_npc = (current_category == CAT_NPCS || current_category == CAT_ARPG_NPCS);
	if (mov_wander_box) mov_wander_box->set_visible(p_mode == "vagar");
	if (mov_patrol_box) mov_patrol_box->set_visible(p_mode == "patrulla");
	if (mov_follow_box) mov_follow_box->set_visible(is_npc && p_mode == "seguir");
	if (mov_face_on_event_row) mov_face_on_event_row->set_visible(is_npc);
	if (mov_chase_box) mov_chase_box->set_visible(is_enemy && (p_mode == "persigue_y_ataca" || p_mode == "huye" || p_mode == "a_distancia"));
	if (mov_ranged_box) mov_ranged_box->set_visible(is_enemy && p_mode == "a_distancia");
}

void ClockGearsDatabaseDialog::_refresh_patrol_points_list() {
	if (!mov_patrol_list || !movement_shape_editor) {
		return;
	}
	bool prev = updating_mov_ui;
	updating_mov_ui = true;
	int prev_sel = movement_shape_editor->get_selected_patrol_index();
	mov_patrol_list->clear();
	Array pts = movement_shape_editor->get_patrol_points_data();
	for (int i = 0; i < pts.size(); i++) {
		Dictionary d = pts[i];
		double px = double(d.get("x", 0.0));
		double py = double(d.get("y", 0.0));
		mov_patrol_list->add_item(vformat(CG_DB_TR("Punto %d: (%.2f, %.2f)"), i + 1, px, py));
	}
	if (prev_sel >= 0 && prev_sel < mov_patrol_list->get_item_count()) {
		mov_patrol_list->select(prev_sel);
	}
	bool has_sel = (prev_sel >= 0 && prev_sel < pts.size());
	if (btn_mov_patrol_remove) btn_mov_patrol_remove->set_disabled(!has_sel);
	if (btn_mov_patrol_up) btn_mov_patrol_up->set_disabled(!has_sel || prev_sel <= 0);
	if (btn_mov_patrol_down) btn_mov_patrol_down->set_disabled(!has_sel || prev_sel >= pts.size() - 1);
	updating_mov_ui = prev;
}

void ClockGearsDatabaseDialog::_sync_movement_canvas() {
	if (!movement_shape_editor || !mov_type_opt) {
		return;
	}
	String move_mode = (mov_type_opt->get_selected() >= 0)
			? String(mov_type_opt->get_selected_metadata())
			: String("quieto");
	double wr = mov_wander_spin ? double(mov_wander_spin->get_value()) : 3.0;
	bool loop = mov_patrol_mode_opt ? (mov_patrol_mode_opt->get_selected() == 1) : false;
	Array pts = movement_shape_editor->get_patrol_points_data();
	movement_shape_editor->set_movement_visual_data(move_mode, wr, pts, loop);
}

void ClockGearsDatabaseDialog::_commit_movement_to_record() {
	if (!s_has_movement_tab_category(current_category)) {
		return;
	}
	int rec_idx = _db_selected_record_index(record_list);
	if (rec_idx < 0 || rec_idx >= category_data[current_category].size()) {
		return;
	}
	Dictionary d = category_data[current_category][rec_idx];
	String move_mode = (mov_type_opt && mov_type_opt->get_selected() >= 0)
			? String(mov_type_opt->get_selected_metadata())
			: String("quieto");

	if (current_category == CAT_ARPG_ENEMIES) {
		d["ai_type"] = move_mode;
		if (arpg_ai_type_opt) {
			bool prev_ui = updating_ui;
			updating_ui = true;
			for (int i = 0; i < arpg_ai_type_opt->get_item_count(); i++) {
				if (String(arpg_ai_type_opt->get_item_metadata(i)) == move_mode) {
					arpg_ai_type_opt->select(i);
					break;
				}
			}
			updating_ui = prev_ui;
		}
		if (mov_lose_sight_spin) d["lose_sight_sec"] = MAX(0.0, double(mov_lose_sight_spin->get_value()));
		if (mov_min_dist_spin) d["min_distance"] = MAX(0.0, double(mov_min_dist_spin->get_value()));
		if (mov_max_dist_spin) {
			double mn = double(d.get("min_distance", 3.0));
			d["max_distance"] = MAX(mn, double(mov_max_dist_spin->get_value()));
		}
	} else {
		d["npc_move_type"] = move_mode;
		if (mov_follow_dist_spin) d["follow_distance"] = MAX(0.5, double(mov_follow_dist_spin->get_value()));
		if (mov_face_on_event_chk) d["face_player_on_event"] = mov_face_on_event_chk->is_pressed();
	}

	if (mov_wander_spin) d["wander_radius"] = MAX(0.0, double(mov_wander_spin->get_value()));
	if (mov_patrol_mode_opt) d["patrol_loop"] = (mov_patrol_mode_opt->get_selected() == 1);
	if (movement_shape_editor) d["patrol_points"] = movement_shape_editor->get_patrol_points_data();
	if (mov_avoid_obstacles_chk) d["avoid_obstacles"] = mov_avoid_obstacles_chk->is_pressed();
	if (mov_recalc_ms_spin) d["path_recalc_ms"] = MAX(50, int(mov_recalc_ms_spin->get_value()));

	ClockGearsDataManager::normalize_record_defaults(current_category, d);
	category_data[current_category][rec_idx] = d;
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_mov_type_selected(int p_idx) {
	if (updating_mov_ui || updating_ui || !mov_type_opt) {
		return;
	}
	String move_mode = (p_idx >= 0) ? String(mov_type_opt->get_item_metadata(p_idx)) : String("quieto");
	_update_movement_contextual_visibility(move_mode);
	_sync_movement_canvas();
	_commit_movement_to_record();
}

void ClockGearsDatabaseDialog::_on_mov_field_changed(double p_unused) {
	if (updating_mov_ui || updating_ui) {
		return;
	}
	_sync_movement_canvas();
	_commit_movement_to_record();
}

void ClockGearsDatabaseDialog::_on_mov_bool_toggled(bool p_pressed) {
	if (updating_mov_ui || updating_ui) {
		return;
	}
	_sync_movement_canvas();
	_commit_movement_to_record();
}

void ClockGearsDatabaseDialog::_on_mov_patrol_list_selected(int p_idx) {
	if (updating_mov_ui || !movement_shape_editor) {
		return;
	}
	movement_shape_editor->select_patrol_point(p_idx);
	_refresh_patrol_points_list();
}

void ClockGearsDatabaseDialog::_on_mov_patrol_add_pressed() {
	if (!movement_shape_editor) {
		return;
	}
	Array pts = movement_shape_editor->get_patrol_points_data();
	Vector2 next_pt(2.0f, 0.0f);
	if (!pts.is_empty()) {
		Dictionary last = pts[pts.size() - 1];
		next_pt = Vector2(float(last.get("x", 0.0)) + 2.0f, float(last.get("y", 0.0)));
	}
	movement_shape_editor->add_patrol_point(next_pt);
	_refresh_patrol_points_list();
	_commit_movement_to_record();
}

void ClockGearsDatabaseDialog::_on_mov_patrol_remove_pressed() {
	if (!movement_shape_editor) {
		return;
	}
	if (movement_shape_editor->remove_selected_patrol_point()) {
		_refresh_patrol_points_list();
		_commit_movement_to_record();
	}
}

void ClockGearsDatabaseDialog::_on_mov_patrol_move_pressed(int p_delta) {
	if (!movement_shape_editor) {
		return;
	}
	int sel = movement_shape_editor->get_selected_patrol_index();
	Array pts = movement_shape_editor->get_patrol_points_data();
	int target = sel + p_delta;
	if (sel < 0 || sel >= pts.size() || target < 0 || target >= pts.size()) {
		return;
	}
	Variant tmp = pts[sel];
	pts[sel] = pts[target];
	pts[target] = tmp;
	String move_mode = (mov_type_opt && mov_type_opt->get_selected() >= 0) ? String(mov_type_opt->get_selected_metadata()) : String("patrulla");
	double wr = mov_wander_spin ? double(mov_wander_spin->get_value()) : 3.0;
	bool loop = mov_patrol_mode_opt ? (mov_patrol_mode_opt->get_selected() == 1) : false;
	movement_shape_editor->set_movement_visual_data(move_mode, wr, pts, loop);
	movement_shape_editor->select_patrol_point(target);
	_refresh_patrol_points_list();
	_commit_movement_to_record();
}

void ClockGearsDatabaseDialog::_on_movement_visual_changed() {
	if (updating_mov_ui || updating_ui || !movement_shape_editor) {
		return;
	}
	updating_mov_ui = true;
	if (mov_wander_spin) {
		double wr = movement_shape_editor->get_wander_radius();
		mov_wander_spin->set_value(wr);
		mov_wander_spin->set_meta("cg_last_valid_val", wr);
	}
	updating_mov_ui = false;
	_refresh_patrol_points_list();
	_commit_movement_to_record();
}

bool ClockGearsDatabaseDialog::select_main_tab_by_name(const String &p_name) {
	if (!main_editor_tabs) {
		return false;
	}
	String q = p_name.strip_edges().to_lower();
	for (int i = 0; i < main_editor_tabs->get_tab_count(); i++) {
		if (main_editor_tabs->is_tab_hidden(i)) {
			continue;
		}
		if (main_editor_tabs->get_tab_title(i).strip_edges().to_lower() == q) {
			main_editor_tabs->set_current_tab(i);
			return true;
		}
	}
	return false;
}

Dictionary ClockGearsDatabaseDialog::check_all_tabs_layout(const Vector2i &p_window_size) {
	Dictionary out;
	set_size(p_window_size);
	Array tabs_report;
	bool all_ok = true;
	Rect2 win_rect{Vector2(), Vector2(p_window_size)};

	if (main_editor_tabs) {
		int prev_tab = main_editor_tabs->get_current_tab();
		for (int i = 0; i < main_editor_tabs->get_tab_count(); i++) {
			if (main_editor_tabs->is_tab_hidden(i)) {
				continue;
			}
			main_editor_tabs->set_current_tab(i);
			Control *page = main_editor_tabs->get_tab_control(i);
			Dictionary tr;
			tr["title"] = main_editor_tabs->get_tab_title(i);
			bool has_hscroll = false;
			bool clipped = false;
			if (page) {
				Rect2 pr = page->get_global_rect();
				tr["rect"] = pr;
				if (pr.size.x > win_rect.size.x + 2.0f || pr.position.x + pr.size.x > p_window_size.x + 2.0f) {
					clipped = true;
				}
				Vector<Node *> stack;
				stack.push_back(page);
				while (!stack.is_empty()) {
					Node *cur = stack[stack.size() - 1];
					stack.remove_at(stack.size() - 1);
					ScrollContainer *sc = Object::cast_to<ScrollContainer>(cur);
					if (sc && sc->is_visible_in_tree()) {
						if (sc->get_horizontal_scroll_mode() != ScrollContainer::SCROLL_MODE_DISABLED) {
							HScrollBar *hs = sc->get_h_scroll_bar();
							if (hs && hs->is_visible()) {
								has_hscroll = true;
							}
						}
					}
					for (int c = 0; c < cur->get_child_count(); c++) {
						stack.push_back(cur->get_child(c));
					}
				}
			}
			tr["has_hscroll"] = has_hscroll;
			tr["clipped"] = clipped;
			if (has_hscroll || clipped) {
				all_ok = false;
			}
			tabs_report.push_back(tr);
		}
		if (prev_tab >= 0 && prev_tab < main_editor_tabs->get_tab_count()) {
			main_editor_tabs->set_current_tab(prev_tab);
		}
	}
	out["window_size"] = p_window_size;
	out["tabs"] = tabs_report;
	out["all_ok"] = all_ok;
	return out;
}

void ClockGearsDatabaseDialog::_add_realtime_slider_spin_row(GridContainer *p_grid, const String &p_label, double p_min, double p_max, double p_step, double p_default, const String &p_suffix, HSlider *&r_slider, SpinBox *&r_spin) {
	if (!p_grid) {
		return;
	}
	Label *lbl = memnew(Label(p_label));
	lbl->set_custom_minimum_size(Size2(185 * EDSCALE, 0));
	p_grid->add_child(lbl);

	HBoxContainer *hb = memnew(HBoxContainer);
	hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hb->add_theme_constant_override("separation", int(10 * EDSCALE));
	p_grid->add_child(hb);

	r_slider = memnew(HSlider);
	r_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	r_slider->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	r_slider->set_custom_minimum_size(Size2(120 * EDSCALE, 24 * EDSCALE));
	r_slider->set_min(p_min);
	r_slider->set_max(p_max);
	r_slider->set_step(p_step);
	r_slider->set_value(p_default);
	hb->add_child(r_slider);

	r_spin = memnew(SpinBox);
	r_spin->set_custom_minimum_size(Size2(96 * EDSCALE, 30 * EDSCALE));
	r_spin->set_min(p_min);
	r_spin->set_max(p_max);
	r_spin->set_step(p_step);
	r_spin->set_value(p_default);
	if (!p_suffix.is_empty()) {
		r_spin->set_suffix(p_suffix);
	}
	_bind_numeric_spinbox(r_spin);
	hb->add_child(r_spin);

	r_slider->connect("value_changed", callable_mp(static_cast<Range *>(r_spin), &Range::set_value));
	r_spin->connect("value_changed", callable_mp(static_cast<Range *>(r_slider), &Range::set_value));
	r_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
}

void ClockGearsDatabaseDialog::_on_menu_section_toggled(bool p_expanded, Node *p_body, Button *p_btn, const String &p_title) {
	if (Control *body_ctrl = Object::cast_to<Control>(p_body)) {
		body_ctrl->set_visible(p_expanded);
	}
	if (p_btn) {
		p_btn->set_text(String::utf8(p_expanded ? "▾ " : "▸ ") + p_title);
	}
}

PanelContainer *ClockGearsDatabaseDialog::_make_collapsible_menu_card(Control *p_parent, const String &p_title, VBoxContainer *&r_body, bool p_expanded) {
	PanelContainer *card = memnew(PanelContainer);
	card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	if (p_parent) {
		p_parent->add_child(card);
	}

	VBoxContainer *outer = memnew(VBoxContainer);
	outer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	outer->add_theme_constant_override("separation", int(6 * EDSCALE));
	card->add_child(outer);

	Button *hdr = memnew(Button(String::utf8(p_expanded ? "▾ " : "▸ ") + p_title));
	hdr->set_toggle_mode(true);
	hdr->set_pressed(p_expanded);
	hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hdr->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	hdr->set_clip_text(true);
	hdr->set_custom_minimum_size(Size2(0, 28 * EDSCALE));
	hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	hdr->add_theme_color_override("font_pressed_color", ClockGearsTheme::color_accent_amber());
	hdr->add_theme_color_override("font_hover_color", Color(1.0f, 0.85f, 0.45f));
	{
		Ref<StyleBoxFlat> sb = memnew(StyleBoxFlat);
		sb->set_bg_color(Color(0.16f, 0.18f, 0.24f, 0.85f));
		sb->set_corner_radius_all(int(6 * EDSCALE));
		sb->set_content_margin_individual(int(8 * EDSCALE), int(4 * EDSCALE), int(8 * EDSCALE), int(4 * EDSCALE));
		hdr->add_theme_style_override("normal", sb);
		hdr->add_theme_style_override("pressed", sb);
		Ref<StyleBoxFlat> hov = sb->duplicate();
		hov->set_bg_color(Color(0.22f, 0.25f, 0.32f, 0.95f));
		hdr->add_theme_style_override("hover", hov);
	}
	outer->add_child(hdr);

	r_body = memnew(VBoxContainer);
	r_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	r_body->add_theme_constant_override("separation", int(8 * EDSCALE));
	r_body->set_visible(p_expanded);
	outer->add_child(r_body);

	hdr->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_section_toggled).bind(r_body, hdr, p_title));
	return card;
}

void ClockGearsDatabaseDialog::_apply_responsive_menu_split(BoxContainer *p_split, Control *p_settings_col, Control *p_preview_col, bool p_vertical) {
	if (!p_split) {
		return;
	}
	p_split->set_vertical(p_vertical);
	if (p_settings_col && p_preview_col && p_settings_col->get_parent() == p_split && p_preview_col->get_parent() == p_split) {
		if (p_vertical) {
			p_split->move_child(p_preview_col, 0);
			p_split->move_child(p_settings_col, 1);
			p_preview_col->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		} else {
			p_split->move_child(p_settings_col, 0);
			p_split->move_child(p_preview_col, 1);
			p_preview_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		}
	}
}

void ClockGearsDatabaseDialog::_populate_common_preview_res_option(OptionButton *p_opt) {
	if (!p_opt) {
		return;
	}
	p_opt->clear();
	p_opt->add_item("960 x 540 (Base)", 0);
	p_opt->set_item_metadata(0, Vector2i(960, 540));
	p_opt->add_item("1366 x 768 (HD)", 1);
	p_opt->set_item_metadata(1, Vector2i(1366, 768));
	p_opt->add_item("1920 x 1080 (Full HD)", 2);
	p_opt->set_item_metadata(2, Vector2i(1920, 1080));
	p_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_common_preview_res_option_selected).bind(p_opt));
}

void ClockGearsDatabaseDialog::_sync_common_preview_res_options() {
	bool prev_upd = updating_ui;
	updating_ui = true;
	OptionButton *opts[5] = { menu_preview_res_opt, gm_preview_res_opt, cred_preview_res_opt, hud_preview_res_opt, go_preview_res_opt };
	for (int k = 0; k < 5; k++) {
		if (!opts[k]) {
			continue;
		}
		for (int i = 0; i < opts[k]->get_item_count(); i++) {
			if (Vector2i(opts[k]->get_item_metadata(i)) == menus_common_preview_res) {
				opts[k]->select(i);
				break;
			}
		}
	}
	if (ws_preview_size_opt) {
		for (int i = 0; i < ws_preview_size_opt->get_item_count(); i++) {
			if (ws_preview_size_opt->get_item_id(i) == menus_common_preview_res.x) {
				ws_preview_size_opt->select(i);
				break;
			}
		}
	}
	String hdr_txt = vformat(CG_DB_TR("Vista previa (%dx%d)"), menus_common_preview_res.x, menus_common_preview_res.y);
	if (menu_prev_title_lbl) menu_prev_title_lbl->set_text(hdr_txt);
	if (gm_prev_title_lbl) gm_prev_title_lbl->set_text(hdr_txt);
	if (cred_prev_title_lbl) cred_prev_title_lbl->set_text(hdr_txt);
	if (hud_prev_title_lbl) hud_prev_title_lbl->set_text(hdr_txt);
	if (go_prev_title_lbl) go_prev_title_lbl->set_text(hdr_txt);
	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_on_common_preview_res_option_selected(int p_idx, OptionButton *p_src) {
	if (updating_ui || !p_src || p_idx < 0) {
		return;
	}
	Vector2i res = p_src->get_item_metadata(p_idx);
	if (res.x >= 320 && res.y >= 180) {
		set_menus_preview_resolution(res.x, res.y);
	}
}

void ClockGearsDatabaseDialog::_on_16_9_holder_resized(Control *p_holder, Control *p_aspect) {
	if (!p_holder || !p_aspect) {
		return;
	}
	float avail_w = p_holder->get_size().x;
	if (avail_w < 32.0f) {
		return;
	}
	float max_h = 380.0f * EDSCALE;
	float target_w = MIN(avail_w, max_h * (16.0f / 9.0f));
	target_w = MAX(160.0f * EDSCALE, target_w);
	float target_h = Math::round(target_w * (9.0f / 16.0f));
	Size2 target_sz(Math::round(target_w), target_h);
	if (p_aspect->get_custom_minimum_size().distance_to(target_sz) > 1.5f) {
		p_aspect->set_custom_minimum_size(target_sz);
		p_aspect->set_size(target_sz);
	}
}

Control *ClockGearsDatabaseDialog::_build_16_9_viewport_preview(Control *p_parent, SubViewport *&r_vp, Label *&r_hdr_lbl, OptionButton *&r_res_opt, Button *p_test_btn) {
	const float row_h = 28.0f * EDSCALE;
	HFlowContainer *prev_hdr = memnew(HFlowContainer);
	prev_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_hdr->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	prev_hdr->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	p_parent->add_child(prev_hdr);

	r_hdr_lbl = memnew(Label(vformat(CG_DB_TR("Vista previa (%dx%d)"), menus_common_preview_res.x, menus_common_preview_res.y)));
	r_hdr_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	r_hdr_lbl->set_clip_text(true);
	r_hdr_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	prev_hdr->add_child(r_hdr_lbl);

	r_res_opt = memnew(OptionButton);
	r_res_opt->set_custom_minimum_size(Size2(130 * EDSCALE, row_h));
	_populate_common_preview_res_option(r_res_opt);
	prev_hdr->add_child(r_res_opt);

	if (p_test_btn) {
		prev_hdr->add_child(p_test_btn);
	}

	VBoxContainer *holder = memnew(VBoxContainer);
	holder->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	p_parent->add_child(holder);

	AspectRatioContainer *aspect = memnew(AspectRatioContainer);
	aspect->set_ratio(16.0f / 9.0f);
	aspect->set_stretch_mode(AspectRatioContainer::STRETCH_FIT);
	aspect->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	aspect->set_custom_minimum_size(Size2(320 * EDSCALE, 180 * EDSCALE));
	holder->add_child(aspect);

	holder->connect("resized", callable_mp(this, &ClockGearsDatabaseDialog::_on_16_9_holder_resized).bind(holder, aspect));
	callable_mp(this, &ClockGearsDatabaseDialog::_on_16_9_holder_resized).call_deferred(holder, aspect);

	r_vp = memnew(SubViewport);
	r_vp->set_size(menus_common_preview_res);
	r_vp->set_disable_3d(true);
	r_vp->set_update_mode(SubViewport::UPDATE_WHEN_PARENT_VISIBLE);
	aspect->add_child(r_vp);

	TextureRect *disp = memnew(TextureRect);
	disp->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	disp->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	disp->set_stretch_mode(TextureRect::STRETCH_SCALE);
	disp->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	disp->set_texture(r_vp->get_texture());
	aspect->add_child(disp);
	return aspect;
}

void ClockGearsDatabaseDialog::set_menus_preview_resolution(int p_width, int p_height) {
	int w = MAX(320, p_width);
	int h = MAX(180, p_height);
	menus_common_preview_res = Vector2i(w, h);
	_sync_common_preview_res_options();

	if (menu_preview_viewport) {
		menu_preview_viewport->set_size(menus_common_preview_res);
		_update_main_menu_live_preview();
	}
	if (gm_preview_viewport) {
		gm_preview_viewport->set_size(menus_common_preview_res);
		_update_game_menu_live_preview();
	}
	if (cred_preview_viewport) {
		cred_preview_viewport->set_size(menus_common_preview_res);
		_update_credits_live_preview();
	}
	if (hud_preview_viewport) {
		hud_preview_viewport->set_size(menus_common_preview_res);
		if (hud_live_preview) {
			hud_live_preview->set_viewport_size_override(Size2(w, h));
			hud_live_preview->refresh_all_widgets("preview_resolution");
		}
	}
	if (go_preview_viewport) {
		go_preview_viewport->set_size(menus_common_preview_res);
		_update_game_over_live_preview();
	}
	_update_window_style_live_preview();
	if (ui_design_editor_panel && ui_design_editor_panel->get_preview_resolution() != menus_common_preview_res) {
		ui_design_editor_panel->set_preview_resolution(w, h, false);
	}
}

void ClockGearsDatabaseDialog::adapt_to_window_size(const Vector2i &p_window_size) {
	if (p_window_size.x <= 0 || p_window_size.y <= 0) {
		return;
	}
	if (col2_scroll_container) {
		col2_scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	}
	if (main_editor_tabs) {
		for (int i = 0; i < main_editor_tabs->get_tab_count(); i++) {
			ScrollContainer *sc = Object::cast_to<ScrollContainer>(main_editor_tabs->get_tab_control(i));
			if (sc && sc != col2_scroll_container) {
				sc->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			}
		}
	}
	const bool use_vertical = (p_window_size.x < int(1400 * EDSCALE));
	_apply_responsive_menu_split(menu_main_split, menu_left_col, menu_right_col, use_vertical);
	_apply_responsive_menu_split(ws_main_split, ws_left_col, ws_right_col, use_vertical);
	_apply_responsive_menu_split(gm_main_split, gm_left_col, gm_prev_card, use_vertical);
	_apply_responsive_menu_split(cred_main_split, cred_left_col, cred_prev_card, use_vertical);
	_apply_responsive_menu_split(hud_main_split, hud_left_col, hud_right_col, use_vertical);
	_apply_responsive_menu_split(go_main_split, go_left_col, go_right_col, use_vertical);
	if (ui_design_editor_panel) {
		ui_design_editor_panel->adapt_to_width(p_window_size.x);
	}
}

bool ClockGearsDatabaseDialog::is_realtime_combat_section_visible() const {
	return sys_arpg_realtime_panel && sys_arpg_realtime_panel->is_visible();
}

void ClockGearsDatabaseDialog::set_system_combat_mode(int p_mode) {
	if (!combat_mode_selector) {
		return;
	}
	int target_idx = (p_mode == COMBAT_MODE_ACTION_RPG) ? 1 : 0;
	combat_mode_selector->select(target_idx);
	_on_combat_mode_selected(target_idx);
}

Dictionary ClockGearsDatabaseDialog::get_system_config_copy() const {
	return system_config.duplicate(true);
}

void ClockGearsDatabaseDialog::set_system_realtime_value(const String &p_key, const Variant &p_val) {
	if (p_key == "arpg_invuln_seconds" && sys_arpg_invuln_spin) {
		sys_arpg_invuln_spin->set_value(double(p_val));
	} else if (p_key == "arpg_block_reduction_pct" && sys_arpg_block_red_spin) {
		sys_arpg_block_red_spin->set_value(double(p_val));
	} else if (p_key == "arpg_dodge_type" && sys_arpg_dodge_type_opt) {
		int idx = (String(p_val).to_lower() == "dash") ? 1 : 0;
		sys_arpg_dodge_type_opt->select(idx);
		_on_system_field_changed();
	} else if (p_key == "arpg_dodge_duration" && sys_arpg_dodge_dur_spin) {
		sys_arpg_dodge_dur_spin->set_value(double(p_val));
	} else if (p_key == "arpg_dodge_distance" && sys_arpg_dodge_dist_spin) {
		sys_arpg_dodge_dist_spin->set_value(double(p_val));
	} else if (p_key == "arpg_dodge_cooldown" && sys_arpg_dodge_cd_spin) {
		sys_arpg_dodge_cd_spin->set_value(double(p_val));
	} else if (p_key == "arpg_show_damage_numbers" && sys_arpg_show_dmg_chk) {
		sys_arpg_show_dmg_chk->set_pressed(bool(p_val));
	}
}

static String s_menu_entry_default_label(const String &p_id) {
	if (p_id == "new_game") return CG_DB_TR("Nueva partida");
	if (p_id == "continue") return CG_DB_TR("Continuar");
	if (p_id == "options") return CG_DB_TR("Opciones");
	if (p_id == "credits") return CG_DB_TR("Créditos");
	if (p_id == "quit") return CG_DB_TR("Salir");
	return p_id;
}

String ClockGearsDatabaseDialog::_import_menu_asset_file(const String &p_src_path, const String &p_subfolder, bool p_pixel_preset) {
	String src = p_src_path.strip_edges();
	if (src.is_empty()) {
		return String();
	}
	String final_path = src;
	String dest_dir = ClockGearsProjectStructure::resolve_project_folder_global(p_subfolder);
	if (!dest_dir.is_empty()) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			da->make_dir_recursive(dest_dir);
			String file_name = src.get_file();
			String dest_global = dest_dir.path_join(file_name);
			if (dest_global != src && FileAccess::exists(src)) {
				da->copy(src, dest_global);
			}
			if (p_pixel_preset) {
				ClockGearsProjectStructure::write_pixel_art_import_preset(dest_global);
			}
			final_path = String("res://").path_join(p_subfolder).path_join(file_name);
		}
	}
#ifdef TOOLS_ENABLED
	if (EditorFileSystem::get_singleton() && final_path.begins_with("res://")) {
		EditorFileSystem *efs = EditorFileSystem::get_singleton();
		efs->update_file(final_path);
		if (!efs->is_scanning()) {
			efs->scan_changes();
		}
	}
#endif
	return final_path;
}

void ClockGearsDatabaseDialog::_setup_main_menu_editor_ui() {
	if (main_menu_editor_panel || !col2_box) {
		return;
	}
	const float row_h = 30.0f * EDSCALE;
	menu_title_config = ClockGearsDataManager::get_default_title_menu_config();
	if (ClockGearsDataManager::get_singleton()) {
		menu_title_config = ClockGearsDataManager::normalize_title_menu_config(ClockGearsDataManager::get_singleton()->get_title_menu_config());
	}

	main_menu_editor_panel = memnew(VBoxContainer);
	main_menu_editor_panel->set_name("MainMenuEditorPanel");
	main_menu_editor_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_menu_editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_menu_editor_panel->add_theme_constant_override("separation", int(12 * EDSCALE));
	main_menu_editor_panel->hide();
	col2_box->add_child(main_menu_editor_panel);

	menu_main_split = memnew(BoxContainer);
	menu_main_split->set_vertical(false);
	menu_main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	menu_main_split->add_theme_constant_override("separation", int(14 * EDSCALE));
	main_menu_editor_panel->add_child(menu_main_split);

	menu_left_col = memnew(VBoxContainer);
	menu_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_left_col->set_stretch_ratio(1.15f);
	menu_left_col->add_theme_constant_override("separation", int(12 * EDSCALE));
	menu_main_split->add_child(menu_left_col);

	auto make_card = [&](const String &p_title, VBoxContainer *&r_body) {
		_make_collapsible_menu_card(menu_left_col, p_title, r_body, true);
	};

	// 1) Entradas del menú
	VBoxContainer *entries_card = nullptr;
	make_card(CG_DB_TR("Entradas del menú"), entries_card);

	Label *entries_hint = memnew(Label(CG_DB_TR("Activa, renombra o reordena las opciones de la pantalla de título.")));
	entries_hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	entries_hint->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	entries_card->add_child(entries_hint);

	menu_entries_list_vbox = memnew(VBoxContainer);
	menu_entries_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_entries_list_vbox->add_theme_constant_override("separation", int(6 * EDSCALE));
	entries_card->add_child(menu_entries_list_vbox);

	// 2) Fondo
	VBoxContainer *bg_card = nullptr;
	make_card(CG_DB_TR("Fondo"), bg_card);

	GridContainer *bg_grid = memnew(GridContainer);
	bg_grid->set_columns(2);
	bg_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_grid->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	bg_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	bg_card->add_child(bg_grid);

	auto add_grid_label = [&](GridContainer *p_grid, const String &p_text) {
		Label *l = memnew(Label(p_text));
		l->set_custom_minimum_size(Size2(118 * EDSCALE, 0));
		p_grid->add_child(l);
	};

	add_grid_label(bg_grid, CG_DB_TR("Tipo:"));
	menu_bg_type_opt = memnew(OptionButton);
	menu_bg_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_type_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_bg_type_opt->add_item(CG_DB_TR("Color sólido"), 0);
	menu_bg_type_opt->add_item(CG_DB_TR("Imagen"), 1);
	menu_bg_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_bg_field_changed));
	bg_grid->add_child(menu_bg_type_opt);

	add_grid_label(bg_grid, CG_DB_TR("Color:"));
	menu_bg_color_btn = memnew(ColorPickerButton);
	menu_bg_color_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_color_btn->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_bg_color_btn->set_edit_alpha(false);
	menu_bg_color_btn->set_pick_color(Color::html("#12151e"));
	menu_bg_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_bg_color_changed));
	bg_grid->add_child(menu_bg_color_btn);

	add_grid_label(bg_grid, CG_DB_TR("Imagen:"));
	HFlowContainer *bg_img_row = memnew(HFlowContainer);
	bg_img_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_img_row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	bg_img_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	bg_grid->add_child(bg_img_row);

	menu_bg_image_edit = memnew(LineEdit);
	menu_bg_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_image_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	menu_bg_image_edit->set_placeholder("res://img/titles/fondo.png");
	menu_bg_image_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_bg_field_changed).unbind(1));
	bg_img_row->add_child(menu_bg_image_edit);

	menu_bg_import_btn = memnew(Button(CG_DB_TR("Importar...")));
	menu_bg_import_btn->set_custom_minimum_size(Size2(0, row_h));
	menu_bg_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_import_asset_pressed).bind(0));
	bg_img_row->add_child(menu_bg_import_btn);

	menu_bg_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	menu_bg_clear_btn->set_custom_minimum_size(Size2(0, row_h));
	menu_bg_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_clear_asset_pressed).bind(0));
	bg_img_row->add_child(menu_bg_clear_btn);

	add_grid_label(bg_grid, CG_DB_TR("Modo:"));
	menu_bg_scale_opt = memnew(OptionButton);
	menu_bg_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_scale_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_bg_scale_opt->add_item(CG_DB_TR("Ajustar"), 0);
	menu_bg_scale_opt->add_item(CG_DB_TR("Rellenar"), 1);
	menu_bg_scale_opt->add_item(CG_DB_TR("Estirar"), 2);
	menu_bg_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_bg_field_changed));
	bg_grid->add_child(menu_bg_scale_opt);

	add_grid_label(bg_grid, CG_DB_TR("Filtro:"));
	menu_bg_filter_opt = memnew(OptionButton);
	menu_bg_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_filter_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_bg_filter_opt->add_item(CG_DB_TR("Nearest (Pixel Art)"), 0);
	menu_bg_filter_opt->add_item(CG_DB_TR("Lineal"), 1);
	menu_bg_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_bg_field_changed));
	bg_grid->add_child(menu_bg_filter_opt);

	// 3) Título y Música
	VBoxContainer *tm_card = nullptr;
	make_card(CG_DB_TR("Título y música"), tm_card);

	GridContainer *tm_grid = memnew(GridContainer);
	tm_grid->set_columns(2);
	tm_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tm_grid->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	tm_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	tm_card->add_child(tm_grid);

	add_grid_label(tm_grid, CG_DB_TR("Tipo de título:"));
	menu_title_type_opt = memnew(OptionButton);
	menu_title_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_title_type_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_title_type_opt->add_item(CG_DB_TR("Texto"), 0);
	menu_title_type_opt->add_item(CG_DB_TR("Imagen"), 1);
	menu_title_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_title_field_changed));
	tm_grid->add_child(menu_title_type_opt);

	add_grid_label(tm_grid, CG_DB_TR("Texto:"));
	menu_title_text_edit = memnew(LineEdit);
	menu_title_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_title_text_edit->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_title_text_edit->set_placeholder(CG_DB_TR("Vacío = usar Título del Proyecto"));
	menu_title_text_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_title_text_changed));
	tm_grid->add_child(menu_title_text_edit);

	add_grid_label(tm_grid, CG_DB_TR("Imagen:"));
	HFlowContainer *t_img_row = memnew(HFlowContainer);
	t_img_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	t_img_row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	t_img_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	tm_grid->add_child(t_img_row);

	menu_title_image_edit = memnew(LineEdit);
	menu_title_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_title_image_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	menu_title_image_edit->set_placeholder("res://img/titles/logo.png");
	menu_title_image_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_title_field_changed).unbind(1));
	t_img_row->add_child(menu_title_image_edit);

	menu_title_import_btn = memnew(Button(CG_DB_TR("Importar...")));
	menu_title_import_btn->set_custom_minimum_size(Size2(0, row_h));
	menu_title_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_import_asset_pressed).bind(1));
	t_img_row->add_child(menu_title_import_btn);

	menu_title_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	menu_title_clear_btn->set_custom_minimum_size(Size2(0, row_h));
	menu_title_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_clear_asset_pressed).bind(1));
	t_img_row->add_child(menu_title_clear_btn);

	add_grid_label(tm_grid, CG_DB_TR("Filtro logo:"));
	menu_title_filter_opt = memnew(OptionButton);
	menu_title_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_title_filter_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	menu_title_filter_opt->add_item(CG_DB_TR("Nearest (Pixel Art)"), 0);
	menu_title_filter_opt->add_item(CG_DB_TR("Lineal"), 1);
	menu_title_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_title_field_changed));
	tm_grid->add_child(menu_title_filter_opt);

	add_grid_label(tm_grid, CG_DB_TR("Música (BGM):"));
	HFlowContainer *bgm_row = memnew(HFlowContainer);
	bgm_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bgm_row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	bgm_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	tm_grid->add_child(bgm_row);

	menu_bgm_edit = memnew(LineEdit);
	menu_bgm_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bgm_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	menu_bgm_edit->set_placeholder("res://audio/bgm/title.ogg");
	menu_bgm_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_bgm_text_changed));
	bgm_row->add_child(menu_bgm_edit);

	menu_bgm_import_btn = memnew(Button(CG_DB_TR("Elegir...")));
	menu_bgm_import_btn->set_custom_minimum_size(Size2(0, row_h));
	menu_bgm_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_import_asset_pressed).bind(2));
	bgm_row->add_child(menu_bgm_import_btn);

	menu_bgm_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	menu_bgm_clear_btn->set_custom_minimum_size(Size2(0, row_h));
	menu_bgm_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_clear_asset_pressed).bind(2));
	bgm_row->add_child(menu_bgm_clear_btn);

	// Columna derecha: Vista previa en vivo + Probar
	menu_right_col = memnew(VBoxContainer);
	menu_right_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_right_col->set_stretch_ratio(1.0f);
	menu_right_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	menu_main_split->add_child(menu_right_col);

	PanelContainer *prev_card = memnew(PanelContainer);
	prev_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	menu_right_col->add_child(prev_card);

	VBoxContainer *prev_vb = memnew(VBoxContainer);
	prev_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_vb->add_theme_constant_override("separation", int(10 * EDSCALE));
	prev_card->add_child(prev_vb);

	menu_test_btn = memnew(Button(CG_DB_TR("Probar")));
	menu_test_btn->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	menu_test_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::open_title_menu_test_popup));

	_build_16_9_viewport_preview(prev_vb, menu_preview_viewport, menu_prev_title_lbl, menu_preview_res_opt, menu_test_btn);
	_sync_common_preview_res_options();

	menu_live_preview = memnew(ClockGearsTitleScreen);
	menu_live_preview->set_preview_mode(true);
	menu_preview_viewport->add_child(menu_live_preview);

	menu_asset_file_dialog = memnew(FileDialog);
	menu_asset_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	menu_asset_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	menu_asset_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_asset_file_selected));
	add_child(menu_asset_file_dialog);
	if (menus_common_preview_res != Vector2i(960, 540)) {
		set_menus_preview_resolution(menus_common_preview_res.x, menus_common_preview_res.y);
	}
}

void ClockGearsDatabaseDialog::_ensure_menu_test_window() {
	if (menu_test_window && menu_test_screen) {
		return;
	}
	menu_test_window = memnew(Window);
	menu_test_window->set_title(CG_DB_TR("Probar menú principal (960x540)"));
	menu_test_window->set_visible(false);
	menu_test_window->set_transient(true);
	menu_test_window->set_content_scale_size(Size2i(960, 540));
	menu_test_window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
	menu_test_window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
	menu_test_window->connect("close_requested", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_test_popup_close_requested));
	add_child(menu_test_window);

	menu_test_screen = memnew(ClockGearsTitleScreen);
	menu_test_screen->set_editor_test_popup_mode(true);
	menu_test_screen->connect("quit_requested", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_test_popup_close_requested));
	menu_test_window->add_child(menu_test_screen);
}

void ClockGearsDatabaseDialog::_rebuild_main_menu_entries_rows() {
	if (!menu_entries_list_vbox) {
		return;
	}
	for (int i = menu_entries_list_vbox->get_child_count() - 1; i >= 0; i--) {
		Node *ch = menu_entries_list_vbox->get_child(i);
		menu_entries_list_vbox->remove_child(ch);
		memdelete(ch);
	}

	menu_title_config = ClockGearsDataManager::normalize_title_menu_config(menu_title_config);
	Array entries = menu_title_config.get("entries", Array());
	Vector<Dictionary> sorted;
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() == Variant::DICTIONARY) {
			sorted.push_back(Dictionary(entries[i]));
		}
	}
	struct Cmp {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("order", 0)) < int(b.get("order", 0));
		}
	};
	sorted.sort_custom<Cmp>();

	const float row_h = 30.0f * EDSCALE;
	for (int i = 0; i < sorted.size(); i++) {
		const Dictionary &e = sorted[i];
		String eid = String(e.get("id", ""));
		String etxt = String(e.get("text", s_menu_entry_default_label(eid)));
		bool enabled = bool(e.get("enabled", true));

		HFlowContainer *row = memnew(HFlowContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->set_meta("cg_entry_id", eid);
		row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
		row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
		menu_entries_list_vbox->add_child(row);

		CheckBox *chk = memnew(CheckBox(CG_DB_TR("Activa")));
		chk->set_pressed_no_signal(enabled);
		chk->set_meta("cg_entry_id", eid);
		chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_entry_toggled).bind(eid));
		row->add_child(chk);

		LineEdit *edit = memnew(LineEdit);
		edit->set_text(etxt);
		edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		edit->set_custom_minimum_size(Size2(96 * EDSCALE, row_h));
		edit->set_meta("cg_entry_id", eid);
		edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_entry_text_changed).bind(eid));
		row->add_child(edit);

		Label *tag = memnew(Label(vformat("[%s]", s_menu_entry_default_label(eid))));
		tag->set_custom_minimum_size(Size2(74 * EDSCALE, 0));
		tag->set_clip_text(true);
		tag->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
		row->add_child(tag);

		Button *btn_up = memnew(Button(String::utf8("↑")));
		btn_up->set_tooltip_text(CG_DB_TR("Subir"));
		btn_up->set_custom_minimum_size(Size2(32 * EDSCALE, row_h));
		btn_up->set_disabled(i == 0);
		btn_up->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_entry_move_pressed).bind(eid, -1));
		row->add_child(btn_up);

		Button *btn_down = memnew(Button(String::utf8("↓")));
		btn_down->set_tooltip_text(CG_DB_TR("Bajar"));
		btn_down->set_custom_minimum_size(Size2(32 * EDSCALE, row_h));
		btn_down->set_disabled(i == sorted.size() - 1);
		btn_down->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_menu_entry_move_pressed).bind(eid, 1));
		row->add_child(btn_down);
	}
}

void ClockGearsDatabaseDialog::_update_main_menu_live_preview() {
	menu_title_config = ClockGearsDataManager::normalize_title_menu_config(menu_title_config);
	if (menu_live_preview) {
		menu_live_preview->apply_title_config_preview(menu_title_config, system_config);
	}
	if (menu_test_window && menu_test_window->is_visible() && menu_test_screen) {
		menu_test_screen->apply_title_config_preview(menu_title_config, system_config);
	}
}

void ClockGearsDatabaseDialog::_refresh_main_menu_editor_ui() {
	if (!main_menu_editor_panel) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;
	menu_title_config = ClockGearsDataManager::normalize_title_menu_config(menu_title_config);

	_rebuild_main_menu_entries_rows();

	Dictionary bg = menu_title_config.get("background", Dictionary());
	if (menu_bg_type_opt) {
		menu_bg_type_opt->select(String(bg.get("type", "color")) == "image" ? 1 : 0);
	}
	if (menu_bg_color_btn) {
		String col_str = String(bg.get("color", "#12151e"));
		menu_bg_color_btn->set_pick_color(Color::html_is_valid(col_str) ? Color::html(col_str) : Color::html("#12151e"));
	}
	if (menu_bg_image_edit) {
		menu_bg_image_edit->set_text(String(bg.get("image_path", "")));
	}
	if (menu_bg_scale_opt) {
		String sc = String(bg.get("scale_mode", "fit"));
		menu_bg_scale_opt->select(sc == "fill" ? 1 : (sc == "stretch" ? 2 : 0));
	}
	if (menu_bg_filter_opt) {
		menu_bg_filter_opt->select(String(bg.get("filter", "nearest")) == "linear" ? 1 : 0);
	}

	Dictionary tcfg = menu_title_config.get("title", Dictionary());
	if (menu_title_type_opt) {
		menu_title_type_opt->select(String(tcfg.get("type", "text")) == "image" ? 1 : 0);
	}
	if (menu_title_text_edit) {
		menu_title_text_edit->set_text(String(tcfg.get("text", "")));
	}
	if (menu_title_image_edit) {
		menu_title_image_edit->set_text(String(tcfg.get("image_path", "")));
	}
	if (menu_title_filter_opt) {
		menu_title_filter_opt->select(String(tcfg.get("filter", "nearest")) == "linear" ? 1 : 0);
	}
	if (menu_bgm_edit) {
		menu_bgm_edit->set_text(String(menu_title_config.get("bgm_path", "")));
	}

	_update_main_menu_live_preview();
	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_on_menu_entry_toggled(bool p_enabled, String p_entry_id) {
	if (updating_ui) {
		return;
	}
	Array entries = menu_title_config.get("entries", Array());
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_entry_id) {
			e["enabled"] = p_enabled;
			entries[i] = e;
			break;
		}
	}
	menu_title_config["entries"] = entries;
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_entry_text_changed(const String &p_text, String p_entry_id) {
	if (updating_ui) {
		return;
	}
	Array entries = menu_title_config.get("entries", Array());
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_entry_id) {
			e["text"] = p_text;
			entries[i] = e;
			break;
		}
	}
	menu_title_config["entries"] = entries;
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_entry_move_pressed(String p_entry_id, int p_delta) {
	if (move_title_menu_entry(p_entry_id, p_delta)) {
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_menu_bg_field_changed(int p_unused) {
	if (updating_ui) {
		return;
	}
	Dictionary bg = menu_title_config.get("background", Dictionary());
	if (menu_bg_type_opt) {
		bg["type"] = (menu_bg_type_opt->get_selected() == 1) ? "image" : "color";
	}
	if (menu_bg_color_btn) {
		bg["color"] = "#" + menu_bg_color_btn->get_pick_color().to_html(false);
	}
	if (menu_bg_image_edit) {
		bg["image_path"] = menu_bg_image_edit->get_text().strip_edges();
	}
	if (menu_bg_scale_opt) {
		int s = menu_bg_scale_opt->get_selected();
		bg["scale_mode"] = (s == 1) ? "fill" : (s == 2 ? "stretch" : "fit");
	}
	if (menu_bg_filter_opt) {
		bg["filter"] = (menu_bg_filter_opt->get_selected() == 1) ? "linear" : "nearest";
	}
	menu_title_config["background"] = bg;
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_bg_color_changed(const Color &p_color) {
	if (updating_ui) {
		return;
	}
	Dictionary bg = menu_title_config.get("background", Dictionary());
	bg["color"] = "#" + p_color.to_html(false);
	menu_title_config["background"] = bg;
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_title_field_changed(int p_unused) {
	if (updating_ui) {
		return;
	}
	Dictionary tcfg = menu_title_config.get("title", Dictionary());
	if (menu_title_type_opt) {
		tcfg["type"] = (menu_title_type_opt->get_selected() == 1) ? "image" : "text";
	}
	if (menu_title_text_edit) {
		tcfg["text"] = menu_title_text_edit->get_text();
	}
	if (menu_title_image_edit) {
		tcfg["image_path"] = menu_title_image_edit->get_text().strip_edges();
	}
	if (menu_title_filter_opt) {
		tcfg["filter"] = (menu_title_filter_opt->get_selected() == 1) ? "linear" : "nearest";
	}
	menu_title_config["title"] = tcfg;
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_title_text_changed(const String &p_text) {
	if (updating_ui) {
		return;
	}
	Dictionary tcfg = menu_title_config.get("title", Dictionary());
	tcfg["text"] = p_text;
	menu_title_config["title"] = tcfg;
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_bgm_text_changed(const String &p_text) {
	if (updating_ui) {
		return;
	}
	menu_title_config["bgm_path"] = p_text.strip_edges();
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_import_asset_pressed(int p_target) {
	if (!menu_asset_file_dialog) {
		return;
	}
	menu_asset_pick_target = p_target;
	menu_asset_file_dialog->clear_filters();
	if (p_target == 2) {
		menu_asset_file_dialog->set_title(CG_DB_TR("Seleccionar música de título (BGM)"));
		menu_asset_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; " + CG_DB_TR("Archivos de Audio"));
	} else {
		menu_asset_file_dialog->set_title(p_target == 0 ? CG_DB_TR("Importar imagen de fondo") : CG_DB_TR("Importar imagen de título"));
		menu_asset_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes"));
	}
	menu_asset_file_dialog->popup_centered_ratio(0.60f);
}

void ClockGearsDatabaseDialog::_on_menu_clear_asset_pressed(int p_target) {
	if (p_target == 0) {
		Dictionary bg = menu_title_config.get("background", Dictionary());
		bg["image_path"] = "";
		if (bg.get("type", "color") == "image") {
			bg["type"] = "color";
		}
		menu_title_config["background"] = bg;
		if (menu_bg_image_edit) menu_bg_image_edit->set_text("");
		if (menu_bg_type_opt) menu_bg_type_opt->select(0);
	} else if (p_target == 1) {
		Dictionary tcfg = menu_title_config.get("title", Dictionary());
		tcfg["image_path"] = "";
		if (tcfg.get("type", "text") == "image") {
			tcfg["type"] = "text";
		}
		menu_title_config["title"] = tcfg;
		if (menu_title_image_edit) menu_title_image_edit->set_text("");
		if (menu_title_type_opt) menu_title_type_opt->select(0);
	} else if (p_target == 2) {
		menu_title_config["bgm_path"] = "";
		if (menu_bgm_edit) menu_bgm_edit->set_text("");
	}
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_asset_file_selected(const String &p_path) {
	if (menu_asset_pick_target == 0) {
		bool pixel = menu_bg_filter_opt ? (menu_bg_filter_opt->get_selected() == 0) : true;
		String final_path = _import_menu_asset_file(p_path, "img/titles", pixel);
		Dictionary bg = menu_title_config.get("background", Dictionary());
		bg["type"] = "image";
		bg["image_path"] = final_path;
		menu_title_config["background"] = bg;
		if (menu_bg_type_opt) menu_bg_type_opt->select(1);
		if (menu_bg_image_edit) menu_bg_image_edit->set_text(final_path);
	} else if (menu_asset_pick_target == 1) {
		bool pixel = menu_title_filter_opt ? (menu_title_filter_opt->get_selected() == 0) : true;
		String final_path = _import_menu_asset_file(p_path, "img/titles", pixel);
		Dictionary tcfg = menu_title_config.get("title", Dictionary());
		tcfg["type"] = "image";
		tcfg["image_path"] = final_path;
		menu_title_config["title"] = tcfg;
		if (menu_title_type_opt) menu_title_type_opt->select(1);
		if (menu_title_image_edit) menu_title_image_edit->set_text(final_path);
	} else if (menu_asset_pick_target == 2) {
		String final_path = _import_menu_asset_file(p_path, "audio/bgm", false);
		menu_title_config["bgm_path"] = final_path;
		if (menu_bgm_edit) menu_bgm_edit->set_text(final_path);
	}
	_update_main_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_menu_test_popup_close_requested() {
	if (menu_test_window) {
		menu_test_window->hide();
	}
}

Dictionary ClockGearsDatabaseDialog::get_title_menu_draft_copy() const {
	return ClockGearsDataManager::normalize_title_menu_config(menu_title_config);
}

void ClockGearsDatabaseDialog::set_title_menu_entry_state(const String &p_id, bool p_enabled, const String &p_text) {
	Array entries = menu_title_config.get("entries", Array());
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_id) {
			e["enabled"] = p_enabled;
			if (!p_text.is_empty()) {
				e["text"] = p_text;
			}
			entries[i] = e;
			break;
		}
	}
	menu_title_config["entries"] = entries;
	_refresh_main_menu_editor_ui();
	_mark_database_dirty();
}

bool ClockGearsDatabaseDialog::move_title_menu_entry(const String &p_id, int p_delta) {
	if (p_delta == 0) {
		return false;
	}
	menu_title_config = ClockGearsDataManager::normalize_title_menu_config(menu_title_config);
	Array entries = menu_title_config.get("entries", Array());
	Vector<Dictionary> sorted;
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() == Variant::DICTIONARY) {
			sorted.push_back(Dictionary(entries[i]));
		}
	}
	struct Cmp {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("order", 0)) < int(b.get("order", 0));
		}
	};
	sorted.sort_custom<Cmp>();

	int found = -1;
	for (int i = 0; i < sorted.size(); i++) {
		if (String(sorted[i].get("id", "")) == p_id) {
			found = i;
			break;
		}
	}
	int step = (p_delta > 0) ? 1 : -1;
	int target = found + step;
	if (found < 0 || target < 0 || target >= sorted.size()) {
		return false;
	}
	Dictionary tmp = sorted[found];
	sorted.write[found] = sorted[target];
	sorted.write[target] = tmp;

	Array out_arr;
	for (int i = 0; i < sorted.size(); i++) {
		Dictionary d = sorted[i];
		d["order"] = i;
		out_arr.push_back(d);
	}
	menu_title_config["entries"] = out_arr;
	_rebuild_main_menu_entries_rows();
	_update_main_menu_live_preview();
	_mark_database_dirty();
	return true;
}

void ClockGearsDatabaseDialog::set_title_menu_background(const String &p_type, const String &p_color_or_path, const String &p_scale_mode, const String &p_filter) {
	Dictionary bg = menu_title_config.get("background", Dictionary());
	String tp = (p_type == "image") ? "image" : "color";
	bg["type"] = tp;
	if (tp == "image") {
		bg["image_path"] = p_color_or_path.strip_edges();
	} else {
		bg["color"] = Color::html_is_valid(p_color_or_path) ? ("#" + Color::html(p_color_or_path).to_html(false)) : "#12151e";
	}
	if (!p_scale_mode.is_empty()) {
		bg["scale_mode"] = p_scale_mode;
	}
	if (!p_filter.is_empty()) {
		bg["filter"] = p_filter;
	}
	menu_title_config["background"] = bg;
	_refresh_main_menu_editor_ui();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::set_title_menu_title_config(const String &p_type, const String &p_text_or_path, const String &p_filter) {
	Dictionary tcfg = menu_title_config.get("title", Dictionary());
	String tp = (p_type == "image") ? "image" : "text";
	tcfg["type"] = tp;
	if (tp == "image") {
		tcfg["image_path"] = p_text_or_path.strip_edges();
	} else {
		tcfg["text"] = p_text_or_path;
	}
	if (!p_filter.is_empty()) {
		tcfg["filter"] = p_filter;
	}
	menu_title_config["title"] = tcfg;
	_refresh_main_menu_editor_ui();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::set_title_menu_bgm(const String &p_path) {
	menu_title_config["bgm_path"] = p_path.strip_edges();
	_refresh_main_menu_editor_ui();
	_mark_database_dirty();
}

Window *ClockGearsDatabaseDialog::open_title_menu_test_popup() {
	_ensure_menu_test_window();
	if (!menu_test_window || !menu_test_screen) {
		return nullptr;
	}
	menu_test_screen->apply_title_config_preview(ClockGearsDataManager::normalize_title_menu_config(menu_title_config), system_config);
	menu_test_screen->close_submenus();
	Size2i target(960, 540);
	if (DisplayServer::get_singleton()) {
		int scr = DisplayServer::get_singleton()->window_get_current_screen();
		Size2i usable = DisplayServer::get_singleton()->screen_get_usable_rect(scr).size;
		if (usable.x > 0 && usable.y > 0) {
			target = target.min(usable);
		}
	}
	menu_test_window->popup_centered(target);
	return menu_test_window;
}

// ============================================================================
// ClockGearsNineSlicePreview (9B Tanda 2 UI)
// ============================================================================

void ClockGearsNineSlicePreview::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_cut_lines_info"), &ClockGearsNineSlicePreview::get_cut_lines_info);
}

ClockGearsNineSlicePreview::ClockGearsNineSlicePreview() {
	set_custom_minimum_size(Size2(160 * EDSCALE, 120 * EDSCALE));
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_clip_contents(true);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
}

void ClockGearsNineSlicePreview::set_preview_data(const Ref<Texture2D> &p_tex, int p_left, int p_top, int p_right, int p_bottom, bool p_pixel_art) {
	texture = p_tex;
	margin_left = MAX(0, p_left);
	margin_top = MAX(0, p_top);
	margin_right = MAX(0, p_right);
	margin_bottom = MAX(0, p_bottom);
	pixel_art = p_pixel_art;
	set_texture_filter(pixel_art ? CanvasItem::TEXTURE_FILTER_NEAREST : CanvasItem::TEXTURE_FILTER_LINEAR);
	queue_redraw();
}

Rect2 ClockGearsNineSlicePreview::_compute_draw_rect() const {
	Size2 sz = get_size();
	if (sz.x <= 4.0f || sz.y <= 4.0f) {
		sz = get_custom_minimum_size();
	}
	const float pad = 8.0f * EDSCALE;
	Rect2 avail(pad, pad, MAX(16.0f, sz.x - pad * 2.0f), MAX(16.0f, sz.y - pad * 2.0f));
	Size2 tex_sz = texture.is_valid() ? texture->get_size() : Size2(64.0f, 64.0f);
	if (tex_sz.x <= 0.0f || tex_sz.y <= 0.0f) {
		tex_sz = Size2(64.0f, 64.0f);
	}
	float scale = MIN(avail.size.x / tex_sz.x, avail.size.y / tex_sz.y);
	if (pixel_art && scale >= 1.0f) {
		scale = MAX(1.0f, Math::floor(scale));
	}
	Size2 dst_sz = tex_sz * scale;
	Point2 dst_pos = avail.position + (avail.size - dst_sz) * 0.5f;
	return Rect2(dst_pos, dst_sz);
}

Dictionary ClockGearsNineSlicePreview::get_cut_lines_info() const {
	Rect2 dr = _compute_draw_rect();
	Vector2 dr_end = dr.get_end();
	Size2 tex_sz = texture.is_valid() ? texture->get_size() : Size2(64.0f, 64.0f);
	float sx = dr.size.x / MAX(1.0f, tex_sz.x);
	float sy = dr.size.y / MAX(1.0f, tex_sz.y);
	float x_l = CLAMP(dr.position.x + float(margin_left) * sx, dr.position.x, dr_end.x);
	float x_r = CLAMP(dr_end.x - float(margin_right) * sx, dr.position.x, dr_end.x);
	float y_t = CLAMP(dr.position.y + float(margin_top) * sy, dr.position.y, dr_end.y);
	float y_b = CLAMP(dr_end.y - float(margin_bottom) * sy, dr.position.y, dr_end.y);

	Dictionary out;
	out["has_texture"] = texture.is_valid();
	out["tex_size"] = tex_sz;
	out["draw_rect"] = dr;
	Dictionary m;
	m["left"] = margin_left;
	m["top"] = margin_top;
	m["right"] = margin_right;
	m["bottom"] = margin_bottom;
	out["margins"] = m;
	Dictionary lines;
	lines["left_x"] = x_l;
	lines["right_x"] = x_r;
	lines["top_y"] = y_t;
	lines["bottom_y"] = y_b;
	out["lines"] = lines;
	return out;
}

void ClockGearsNineSlicePreview::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW) {
		return;
	}
	Size2 sz = get_size();
	if (sz.x <= 1.0f || sz.y <= 1.0f) {
		return;
	}
	draw_rect(Rect2(Vector2(), sz), Color(0.08f, 0.09f, 0.12f, 0.96f), true);
	draw_rect(Rect2(Vector2(), sz), Color(1.0f, 1.0f, 1.0f, 0.14f), false, 1.0f);

	Rect2 dr = _compute_draw_rect();
	Vector2 dr_end = dr.get_end();
	const float cell = 8.0f * EDSCALE;
	int cols = MAX(1, int(Math::ceil(dr.size.x / cell)));
	int rows = MAX(1, int(Math::ceil(dr.size.y / cell)));
	for (int ry = 0; ry < rows; ry++) {
		for (int rx = 0; rx < cols; rx++) {
			Color c = ((rx + ry) % 2 == 0) ? Color(0.14f, 0.15f, 0.19f, 0.9f) : Color(0.19f, 0.21f, 0.26f, 0.9f);
			Rect2 cr(dr.position.x + rx * cell, dr.position.y + ry * cell,
					MIN(cell, dr_end.x - (dr.position.x + rx * cell)),
					MIN(cell, dr_end.y - (dr.position.y + ry * cell)));
			draw_rect(cr, c, true);
		}
	}

	if (texture.is_valid()) {
		draw_texture_rect(texture, dr, false);
	}
	draw_rect(dr, Color(1.0f, 1.0f, 1.0f, 0.25f), false, 1.0f);

	Size2 tex_sz = texture.is_valid() ? texture->get_size() : Size2(64.0f, 64.0f);
	float sx = dr.size.x / MAX(1.0f, tex_sz.x);
	float sy = dr.size.y / MAX(1.0f, tex_sz.y);
	float x_l = CLAMP(dr.position.x + float(margin_left) * sx, dr.position.x, dr_end.x);
	float x_r = CLAMP(dr_end.x - float(margin_right) * sx, dr.position.x, dr_end.x);
	float y_t = CLAMP(dr.position.y + float(margin_top) * sy, dr.position.y, dr_end.y);
	float y_b = CLAMP(dr_end.y - float(margin_bottom) * sy, dr.position.y, dr_end.y);

	auto draw_cut = [&](const Vector2 &p_a, const Vector2 &p_b, const Color &p_col) {
		draw_line(p_a, p_b, Color(0.0f, 0.0f, 0.0f, 0.78f), 4.0f);
		draw_line(p_a, p_b, p_col, 2.0f);
	};
	Color col_v(0.25f, 0.90f, 1.0f, 0.95f);
	Color col_h(1.0f, 0.76f, 0.28f, 0.95f);
	draw_cut(Vector2(x_l, dr.position.y), Vector2(x_l, dr_end.y), col_v);
	draw_cut(Vector2(x_r, dr.position.y), Vector2(x_r, dr_end.y), col_v);
	draw_cut(Vector2(dr.position.x, y_t), Vector2(dr_end.x, y_t), col_h);
	draw_cut(Vector2(dr.position.x, y_b), Vector2(dr_end.x, y_b), col_h);
}

// ============================================================================
// Menús -> Estilos de ventana (9B Tanda 2 UI)
// ============================================================================

const char *ClockGearsDatabaseDialog::_ws_btn_state_key(int p_tab_idx) {
	static const char *k_keys[4] = { "normal", "hover", "pressed", "disabled" };
	return k_keys[CLAMP(p_tab_idx, 0, 3)];
}

static int s_ws_border_type_to_idx(const String &p_bt) {
	if (p_bt == "rect") return 1;
	if (p_bt == "none") return 2;
	if (p_bt == "image") return 3;
	return 0; // rounded
}

static String s_ws_idx_to_border_type(int p_idx) {
	if (p_idx == 1) return "rect";
	if (p_idx == 2) return "none";
	if (p_idx == 3) return "image";
	return "rounded";
}

void ClockGearsDatabaseDialog::_setup_window_styles_editor_ui() {
	if (window_styles_editor_panel || !col2_box) {
		return;
	}
	const float row_h = 28.0f * EDSCALE;
	window_styles_config_draft = ClockGearsDataManager::get_default_window_styles_config();
	if (ClockGearsDataManager::get_singleton()) {
		window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(
				ClockGearsDataManager::get_singleton()->get_window_styles_config());
	}

	window_styles_editor_panel = memnew(VBoxContainer);
	window_styles_editor_panel->set_name("WindowStylesEditorPanel");
	window_styles_editor_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	window_styles_editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	window_styles_editor_panel->add_theme_constant_override("separation", int(10 * EDSCALE));
	window_styles_editor_panel->hide();
	col2_box->add_child(window_styles_editor_panel);

	ws_main_split = memnew(BoxContainer);
	ws_main_split->set_vertical(false);
	ws_main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	ws_main_split->add_theme_constant_override("separation", int(12 * EDSCALE));
	window_styles_editor_panel->add_child(ws_main_split);

	ws_left_col = memnew(VBoxContainer);
	ws_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_left_col->set_stretch_ratio(1.25f);
	ws_left_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	ws_main_split->add_child(ws_left_col);

	ws_right_col = memnew(VBoxContainer);
	ws_right_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_right_col->set_stretch_ratio(1.0f);
	ws_right_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	ws_main_split->add_child(ws_right_col);

	VBoxContainer *left_col = ws_left_col;
	VBoxContainer *right_col = ws_right_col;

	auto make_ws_card = [&](VBoxContainer *p_col, const String &p_title, VBoxContainer *&r_body) {
		_make_collapsible_menu_card(p_col, p_title, r_body, true);
	};

	// 1) Estilo seleccionado (Nombre + Ventana)
	VBoxContainer *style_card = nullptr;
	make_ws_card(left_col, CG_DB_TR("Apariencia de ventana"), style_card);

	HFlowContainer *name_row = memnew(HFlowContainer);
	name_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	name_row->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	name_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	style_card->add_child(name_row);

	Label *nl = memnew(Label(CG_DB_TR("Nombre:")));
	nl->set_custom_minimum_size(Size2(92 * EDSCALE, 0));
	name_row->add_child(nl);

	ws_name_edit = memnew(LineEdit);
	ws_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_name_edit->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	ws_name_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_name_changed));
	name_row->add_child(ws_name_edit);

	ws_builtin_badge = memnew(Label(CG_DB_TR("Incluido (protegido)")));
	ws_builtin_badge->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	ws_builtin_badge->hide();
	name_row->add_child(ws_builtin_badge);

	ws_surface_host[0] = memnew(VBoxContainer);
	ws_surface_host[0]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_surface_host[0]->add_theme_constant_override("separation", int(6 * EDSCALE));
	style_card->add_child(ws_surface_host[0]);
	_build_ws_surface_ui(0, ws_surface_host[0]);

	// 2) Pestañas de botones (Normal, Resaltado, Pulsado, Desactivado)
	VBoxContainer *btn_card = nullptr;
	make_ws_card(left_col, CG_DB_TR("Botones del estilo"), btn_card);

	ws_btn_state_tabs = memnew(TabBar);
	ws_btn_state_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_btn_state_tabs->set_clip_tabs(true);
	ws_btn_state_tabs->set_tab_alignment(TabBar::ALIGNMENT_LEFT);
	ws_btn_state_tabs->add_tab(CG_DB_TR("Normal"));
	ws_btn_state_tabs->add_tab(CG_DB_TR("Resaltado"));
	ws_btn_state_tabs->add_tab(CG_DB_TR("Pulsado"));
	ws_btn_state_tabs->add_tab(CG_DB_TR("Desactivado"));
	{
		Ref<StyleBoxFlat> tb_sel = memnew(StyleBoxFlat);
		tb_sel->set_bg_color(Color(0.18f, 0.21f, 0.27f, 1.0f));
		tb_sel->set_border_color(Color(0.88f, 0.63f, 0.25f, 0.90f));
		tb_sel->set_border_width(SIDE_BOTTOM, 2);
		tb_sel->set_corner_radius_all(int(7 * EDSCALE));
		tb_sel->set_content_margin_individual(int(10 * EDSCALE), int(5 * EDSCALE), int(10 * EDSCALE), int(5 * EDSCALE));

		Ref<StyleBoxFlat> tb_unsel = memnew(StyleBoxFlat);
		tb_unsel->set_bg_color(Color(0.12f, 0.13f, 0.17f, 0.92f));
		tb_unsel->set_corner_radius_all(int(7 * EDSCALE));
		tb_unsel->set_content_margin_individual(int(10 * EDSCALE), int(5 * EDSCALE), int(10 * EDSCALE), int(5 * EDSCALE));

		ws_btn_state_tabs->add_theme_style_override("tab_selected", tb_sel);
		ws_btn_state_tabs->add_theme_style_override("tab_hovered", tb_sel);
		ws_btn_state_tabs->add_theme_style_override("tab_unselected", tb_unsel);
		ws_btn_state_tabs->add_theme_style_override("tab_focus", tb_sel);
	}
	ws_btn_state_tabs->connect("tab_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_btn_tab_changed));
	btn_card->add_child(ws_btn_state_tabs);

	ws_surface_host[1] = memnew(VBoxContainer);
	ws_surface_host[1]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_surface_host[1]->add_theme_constant_override("separation", int(6 * EDSCALE));
	btn_card->add_child(ws_surface_host[1]);
	_build_ws_surface_ui(1, ws_surface_host[1]);

	// 3) Columna derecha: Vista previa en vivo
	VBoxContainer *prev_card = nullptr;
	make_ws_card(right_col, CG_DB_TR("Vista previa en vivo"), prev_card);

	HFlowContainer *prev_bar = memnew(HFlowContainer);
	prev_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_bar->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	prev_bar->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	prev_card->add_child(prev_bar);

	Label *sz_lbl = memnew(Label(CG_DB_TR("Tamaño / escala:")));
	prev_bar->add_child(sz_lbl);

	ws_preview_size_opt = memnew(OptionButton);
	ws_preview_size_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_preview_size_opt->set_custom_minimum_size(Size2(150 * EDSCALE, row_h));
	ws_preview_size_opt->add_item(CG_DB_TR("Base 960x540 (1x)"), 960);
	ws_preview_size_opt->add_item(CG_DB_TR("HD 1366x768 (1.4x)"), 1366);
	ws_preview_size_opt->add_item(CG_DB_TR("Full HD 1920x1080 (2x)"), 1920);
	ws_preview_size_opt->add_item(CG_DB_TR("QHD 2560x1440 (3x)"), 2560);
	ws_preview_size_opt->select(0);
	ws_preview_size_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_preview_size_selected));
	prev_bar->add_child(ws_preview_size_opt);

	ws_preview_backdrop = memnew(PanelContainer);
	ws_preview_backdrop->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_preview_backdrop->set_custom_minimum_size(Size2(220 * EDSCALE, 230 * EDSCALE));
	ws_preview_backdrop->set_clip_contents(true);
	{
		Ref<StyleBoxFlat> bg_sb = memnew(StyleBoxFlat);
		bg_sb->set_bg_color(Color(0.06f, 0.07f, 0.10f, 1.0f));
		bg_sb->set_border_width_all(1);
		bg_sb->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.10f));
		bg_sb->set_corner_radius_all(int(8 * EDSCALE));
		bg_sb->set_content_margin_all(int(14 * EDSCALE));
		ws_preview_backdrop->add_theme_style_override("panel", bg_sb);
	}
	prev_card->add_child(ws_preview_backdrop);

	VBoxContainer *prev_inner = memnew(VBoxContainer);
	prev_inner->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_inner->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	prev_inner->add_theme_constant_override("separation", int(12 * EDSCALE));
	ws_preview_backdrop->add_child(prev_inner);

	ws_preview_window = memnew(PanelContainer);
	ws_preview_window->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_preview_window->set_custom_minimum_size(Size2(0, 96 * EDSCALE));
	prev_inner->add_child(ws_preview_window);

	VBoxContainer *win_txt_vb = memnew(VBoxContainer);
	win_txt_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	win_txt_vb->add_theme_constant_override("separation", int(4 * EDSCALE));
	ws_preview_window->add_child(win_txt_vb);

	ws_preview_window_title = memnew(Label(CG_DB_TR("Ventana de ejemplo")));
	ws_preview_window_title->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	win_txt_vb->add_child(ws_preview_window_title);

	ws_preview_window_body = memnew(Label(CG_DB_TR("Texto de diálogo y paneles de interfaz.")));
	ws_preview_window_body->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	win_txt_vb->add_child(ws_preview_window_body);

	Label *btns_lbl = memnew(Label(CG_DB_TR("Botones (4 estados):")));
	btns_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	prev_inner->add_child(btns_lbl);

	HFlowContainer *btns_flow = memnew(HFlowContainer);
	btns_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btns_flow->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	btns_flow->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	prev_inner->add_child(btns_flow);

	static const char *k_btn_labels[4] = { "Normal", "Resaltado", "Pulsado", "Desactivado" };
	for (int i = 0; i < 4; i++) {
		Button *pb = memnew(Button(CG_DB_TR(k_btn_labels[i])));
		pb->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
		pb->set_focus_mode(Control::FOCUS_NONE);
		if (i == 3) {
			pb->set_disabled(true);
		}
		btns_flow->add_child(pb);
		ws_preview_btns[i] = pb;
	}

	// 4) Columna derecha: Asignación
	VBoxContainer *assign_card = nullptr;
	make_ws_card(right_col, CG_DB_TR("Asignación"), assign_card);

	HFlowContainer *def_row = memnew(HFlowContainer);
	def_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	def_row->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	def_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	assign_card->add_child(def_row);

	Label *def_lbl = memnew(Label(CG_DB_TR("Estilo por defecto:")));
	def_lbl->set_custom_minimum_size(Size2(130 * EDSCALE, 0));
	def_row->add_child(def_lbl);

	ws_default_style_opt = memnew(OptionButton);
	ws_default_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_default_style_opt->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	ws_default_style_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_default_style_selected));
	def_row->add_child(ws_default_style_opt);

	assign_card->add_child(memnew(HSeparator));

	Label *wt_hdr = memnew(Label(CG_DB_TR("Por tipo de ventana:")));
	wt_hdr->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	assign_card->add_child(wt_hdr);

	ws_assignments_grid = memnew(GridContainer);
	ws_assignments_grid->set_columns(2);
	ws_assignments_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ws_assignments_grid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	ws_assignments_grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	assign_card->add_child(ws_assignments_grid);

	ws_image_file_dialog = memnew(FileDialog);
	ws_image_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	ws_image_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	ws_image_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_image_file_selected));
	add_child(ws_image_file_dialog);
}

void ClockGearsDatabaseDialog::_build_ws_surface_ui(int p_slot_idx, VBoxContainer *p_parent) {
	StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	ui.slot_idx = p_slot_idx;
	ui.root = p_parent;
	const float row_h = 28.0f * EDSCALE;

	if (p_slot_idx == 1) {
		ui.inherit_normal_chk = memnew(CheckBox(CG_DB_TR("Heredar del estado Normal")));
		ui.inherit_normal_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_surface_changed_v).bind(p_slot_idx));
		p_parent->add_child(ui.inherit_normal_chk);
	}

	ui.editable_body = memnew(VBoxContainer);
	ui.editable_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.editable_body->add_theme_constant_override("separation", int(6 * EDSCALE));
	p_parent->add_child(ui.editable_body);

	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(2);
	grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	ui.editable_body->add_child(grid);

	auto add_lbl = [&](const String &p_text) {
		Label *l = memnew(Label(p_text));
		l->set_custom_minimum_size(Size2(106 * EDSCALE, 0));
		grid->add_child(l);
	};

	auto add_slider_spin = [&](const String &p_lbl, double p_min, double p_max, double p_step, double p_def, const String &p_sfx, HSlider *&r_sl, SpinBox *&r_sp, const Callable &p_cb) {
		add_lbl(p_lbl);
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		grid->add_child(hb);

		r_sl = memnew(HSlider);
		r_sl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_sl->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		r_sl->set_custom_minimum_size(Size2(72 * EDSCALE, 22 * EDSCALE));
		r_sl->set_min(p_min);
		r_sl->set_max(p_max);
		r_sl->set_step(p_step);
		r_sl->set_value(p_def);
		hb->add_child(r_sl);

		r_sp = memnew(SpinBox);
		r_sp->set_custom_minimum_size(Size2(76 * EDSCALE, row_h));
		r_sp->set_min(p_min);
		r_sp->set_max(p_max);
		r_sp->set_step(p_step);
		r_sp->set_value(p_def);
		if (!p_sfx.is_empty()) {
			r_sp->set_suffix(p_sfx);
		}
		_bind_numeric_spinbox(r_sp);
		hb->add_child(r_sp);

		r_sl->connect("value_changed", callable_mp(static_cast<Range *>(r_sp), &Range::set_value));
		r_sp->connect("value_changed", callable_mp(static_cast<Range *>(r_sl), &Range::set_value));
		r_sp->connect("value_changed", p_cb);
	};

	add_lbl(CG_DB_TR("Tipo de borde:"));
	ui.border_type_opt = memnew(OptionButton);
	ui.border_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.border_type_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	ui.border_type_opt->add_item(CG_DB_TR("Redondeado"), 0);
	ui.border_type_opt->add_item(CG_DB_TR("Recto"), 1);
	ui.border_type_opt->add_item(CG_DB_TR("Sin borde"), 2);
	ui.border_type_opt->add_item(CG_DB_TR("Imagen"), 3);
	ui.border_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_surface_changed_v).bind(p_slot_idx));
	grid->add_child(ui.border_type_opt);

	Callable surf_cb = callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_surface_changed_v).bind(p_slot_idx);
	add_slider_spin(CG_DB_TR("Grosor borde:"), 0, 24, 1, 2, "px", ui.border_width_slider, ui.border_width_spin, surf_cb);
	add_slider_spin(CG_DB_TR("Radio esquinas:"), 0, 48, 1, 8, "px", ui.corner_radius_slider, ui.corner_radius_spin, surf_cb);

	add_lbl(CG_DB_TR("Colores:"));
	HFlowContainer *cols_flow = memnew(HFlowContainer);
	cols_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cols_flow->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	cols_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	grid->add_child(cols_flow);

	auto add_color_chip = [&](const String &p_caption, bool p_alpha, ColorPickerButton *&r_btn) {
		HBoxContainer *cb = memnew(HBoxContainer);
		cb->add_theme_constant_override("separation", int(4 * EDSCALE));
		cols_flow->add_child(cb);
		Label *l = memnew(Label(p_caption));
		cb->add_child(l);
		r_btn = memnew(ColorPickerButton);
		r_btn->set_edit_alpha(p_alpha);
		r_btn->set_custom_minimum_size(Size2(42 * EDSCALE, row_h));
		cb->add_child(r_btn);
	};
	add_color_chip(CG_DB_TR("Borde"), true, ui.border_color_btn);
	ui.border_color_btn->connect("color_changed", surf_cb);

	add_color_chip(CG_DB_TR("Relleno"), true, ui.fill_color_btn);
	ui.fill_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_fill_color_changed).bind(p_slot_idx));

	add_color_chip(CG_DB_TR("Texto"), false, ui.font_color_btn);
	ui.font_color_btn->connect("color_changed", surf_cb);

	add_lbl(CG_DB_TR("Relleno:"));
	ui.fill_type_opt = memnew(OptionButton);
	ui.fill_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.fill_type_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	ui.fill_type_opt->add_item(CG_DB_TR("Color"), 0);
	ui.fill_type_opt->add_item(CG_DB_TR("Imagen"), 1);
	ui.fill_type_opt->connect("item_selected", surf_cb);
	grid->add_child(ui.fill_type_opt);

	add_slider_spin(CG_DB_TR("Opacidad:"), 0, 100, 1, 94, "%", ui.opacity_slider, ui.opacity_spin,
			callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_opacity_changed).bind(p_slot_idx));

	add_lbl(CG_DB_TR("Img. relleno:"));
	HBoxContainer *fimg_row = memnew(HBoxContainer);
	fimg_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	fimg_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	grid->add_child(fimg_row);
	ui.fill_image_row = fimg_row;

	ui.fill_image_edit = memnew(LineEdit);
	ui.fill_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.fill_image_edit->set_custom_minimum_size(Size2(70 * EDSCALE, row_h));
	ui.fill_image_edit->set_placeholder("res://img/ui/relleno.png");
	ui.fill_image_edit->connect("text_changed", surf_cb);
	fimg_row->add_child(ui.fill_image_edit);

	ui.fill_image_import_btn = memnew(Button(CG_DB_TR("Importar...")));
	ui.fill_image_import_btn->set_custom_minimum_size(Size2(0, row_h));
	ui.fill_image_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_import_image_pressed).bind(p_slot_idx, 1));
	fimg_row->add_child(ui.fill_image_import_btn);

	ui.fill_image_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	ui.fill_image_clear_btn->set_custom_minimum_size(Size2(0, row_h));
	ui.fill_image_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_clear_image_pressed).bind(p_slot_idx, 1));
	fimg_row->add_child(ui.fill_image_clear_btn);

	add_slider_spin(CG_DB_TR("Margen interno:"), 0, 64, 1, 12, "px", ui.padding_slider, ui.padding_spin,
			callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_uniform_padding_changed).bind(p_slot_idx));

	add_lbl(CG_DB_TR("Lados (I/A/D/B):"));
	HFlowContainer *pad_flow = memnew(HFlowContainer);
	pad_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pad_flow->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	pad_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	grid->add_child(pad_flow);
	static const char *k_side_tags[4] = { "I:", "A:", "D:", "B:" };
	for (int s = 0; s < 4; s++) {
		Label *sl = memnew(Label(CG_DB_TR(k_side_tags[s])));
		pad_flow->add_child(sl);
		SpinBox *sp = memnew(SpinBox);
		sp->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
		sp->set_min(0);
		sp->set_max(64);
		sp->set_step(1);
		_bind_numeric_spinbox(sp);
		sp->connect("value_changed", surf_cb);
		pad_flow->add_child(sp);
		ui.padding_side_spins[s] = sp;
	}

	// Sección Tipo Imagen (9 porciones + líneas de corte)
	ui.image_section = memnew(VBoxContainer);
	ui.image_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.image_section->add_theme_constant_override("separation", int(6 * EDSCALE));
	ui.image_section->hide();
	ui.editable_body->add_child(ui.image_section);

	ui.image_section->add_child(memnew(HSeparator));
	Label *img_sec_lbl = memnew(Label(CG_DB_TR("Marco por imagen (9 porciones)")));
	img_sec_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	ui.image_section->add_child(img_sec_lbl);

	GridContainer *img_grid = memnew(GridContainer);
	img_grid->set_columns(2);
	img_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	img_grid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	img_grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	ui.image_section->add_child(img_grid);

	auto add_img_lbl = [&](const String &p_t) {
		Label *l = memnew(Label(p_t));
		l->set_custom_minimum_size(Size2(106 * EDSCALE, 0));
		img_grid->add_child(l);
	};

	add_img_lbl(CG_DB_TR("Imagen marco:"));
	HBoxContainer *mimg_row = memnew(HBoxContainer);
	mimg_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mimg_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	img_grid->add_child(mimg_row);

	ui.image_path_edit = memnew(LineEdit);
	ui.image_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.image_path_edit->set_custom_minimum_size(Size2(70 * EDSCALE, row_h));
	ui.image_path_edit->set_placeholder("res://img/ui/marco.png");
	ui.image_path_edit->connect("text_changed", surf_cb);
	mimg_row->add_child(ui.image_path_edit);

	ui.image_import_btn = memnew(Button(CG_DB_TR("Importar...")));
	ui.image_import_btn->set_custom_minimum_size(Size2(0, row_h));
	ui.image_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_import_image_pressed).bind(p_slot_idx, 0));
	mimg_row->add_child(ui.image_import_btn);

	ui.image_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	ui.image_clear_btn->set_custom_minimum_size(Size2(0, row_h));
	ui.image_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_clear_image_pressed).bind(p_slot_idx, 0));
	mimg_row->add_child(ui.image_clear_btn);

	add_img_lbl(CG_DB_TR("Partes centrales:"));
	ui.center_mode_opt = memnew(OptionButton);
	ui.center_mode_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ui.center_mode_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	ui.center_mode_opt->add_item(CG_DB_TR("Estirar"), 0);
	ui.center_mode_opt->add_item(CG_DB_TR("Repetir"), 1);
	ui.center_mode_opt->connect("item_selected", surf_cb);
	img_grid->add_child(ui.center_mode_opt);

	add_img_lbl(CG_DB_TR("Pixel art:"));
	ui.pixel_art_chk = memnew(CheckBox(CG_DB_TR("Nearest y escala entera")));
	ui.pixel_art_chk->set_pressed(true);
	ui.pixel_art_chk->connect("toggled", surf_cb);
	img_grid->add_child(ui.pixel_art_chk);

	static const char *k_slice_names[4] = { "Corte Izq:", "Corte Arr:", "Corte Der:", "Corte Abj:" };
	for (int s = 0; s < 4; s++) {
		add_img_lbl(CG_DB_TR(k_slice_names[s]));
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		img_grid->add_child(hb);

		HSlider *sl = memnew(HSlider);
		sl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sl->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		sl->set_custom_minimum_size(Size2(72 * EDSCALE, 22 * EDSCALE));
		sl->set_min(0);
		sl->set_max(256);
		sl->set_step(1);
		sl->set_value(8);
		hb->add_child(sl);

		SpinBox *sp = memnew(SpinBox);
		sp->set_custom_minimum_size(Size2(76 * EDSCALE, row_h));
		sp->set_min(0);
		sp->set_max(256);
		sp->set_step(1);
		sp->set_value(8);
		sp->set_suffix("px");
		_bind_numeric_spinbox(sp);
		hb->add_child(sp);

		sl->connect("value_changed", callable_mp(static_cast<Range *>(sp), &Range::set_value));
		sp->connect("value_changed", callable_mp(static_cast<Range *>(sl), &Range::set_value));
		sp->connect("value_changed", surf_cb);
		ui.slice_sliders[s] = sl;
		ui.slice_spins[s] = sp;
	}

	ui.slice_preview = memnew(ClockGearsNineSlicePreview);
	ui.slice_preview->set_custom_minimum_size(Size2(180 * EDSCALE, 130 * EDSCALE));
	ui.image_section->add_child(ui.slice_preview);
}

void ClockGearsDatabaseDialog::_populate_ws_surface_ui(int p_slot_idx, const Dictionary &p_surface, bool p_allow_inherit_toggle) {
	StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	bool prev_upd = updating_ui;
	updating_ui = true;

	bool inherit = bool(p_surface.get("inherit_normal", false));
	if (ui.inherit_normal_chk) {
		ui.inherit_normal_chk->set_visible(p_allow_inherit_toggle);
		ui.inherit_normal_chk->set_pressed_no_signal(p_allow_inherit_toggle && inherit);
	}

	String bt = String(p_surface.get("border_type", "rounded")).to_lower();
	if (ui.border_type_opt) {
		ui.border_type_opt->select(s_ws_border_type_to_idx(bt));
	}
	int bw = int(p_surface.get("border_width", 2));
	int cr = int(p_surface.get("corner_radius", 8));
	if (ui.border_width_slider) ui.border_width_slider->set_value_no_signal(bw);
	if (ui.border_width_spin) ui.border_width_spin->set_value_no_signal(bw);
	if (ui.corner_radius_slider) ui.corner_radius_slider->set_value_no_signal(cr);
	if (ui.corner_radius_spin) ui.corner_radius_spin->set_value_no_signal(cr);

	Color bc = Color::html_is_valid(String(p_surface.get("border_color", ""))) ? Color::html(String(p_surface["border_color"])) : Color(0.88f, 0.63f, 0.25f, 1.0f);
	Color fc = Color::html_is_valid(String(p_surface.get("fill_color", ""))) ? Color::html(String(p_surface["fill_color"])) : Color(0.08f, 0.09f, 0.12f, 0.94f);
	Color tc = Color::html_is_valid(String(p_surface.get("font_color", ""))) ? Color::html(String(p_surface["font_color"])) : Color(0.93f, 0.95f, 0.98f, 1.0f);
	if (ui.border_color_btn) ui.border_color_btn->set_pick_color(bc);
	if (ui.fill_color_btn) ui.fill_color_btn->set_pick_color(fc);
	if (ui.font_color_btn) ui.font_color_btn->set_pick_color(tc);

	int op_pct = CLAMP(int(Math::round(fc.a * 100.0f)), 0, 100);
	if (ui.opacity_slider) ui.opacity_slider->set_value_no_signal(op_pct);
	if (ui.opacity_spin) ui.opacity_spin->set_value_no_signal(op_pct);

	String ft = String(p_surface.get("fill_type", "color")).to_lower();
	if (ui.fill_type_opt) {
		ui.fill_type_opt->select(ft == "image" ? 1 : 0);
	}
	if (ui.fill_image_edit) {
		ui.fill_image_edit->set_text(String(p_surface.get("fill_image_path", "")));
	}
	if (ui.fill_image_row) {
		ui.fill_image_row->set_visible(ft == "image");
	}

	Dictionary pad = (p_surface.has("padding") && p_surface["padding"].get_type() == Variant::DICTIONARY) ? Dictionary(p_surface["padding"]) : Dictionary();
	int p_vals[4] = {
		int(pad.get("left", 12)),
		int(pad.get("top", 10)),
		int(pad.get("right", 12)),
		int(pad.get("bottom", 10)),
	};
	int p_avg = (p_vals[0] + p_vals[1] + p_vals[2] + p_vals[3]) / 4;
	if (ui.padding_slider) ui.padding_slider->set_value_no_signal(p_avg);
	if (ui.padding_spin) ui.padding_spin->set_value_no_signal(p_avg);
	for (int s = 0; s < 4; s++) {
		if (ui.padding_side_spins[s]) {
			ui.padding_side_spins[s]->set_value_no_signal(p_vals[s]);
		}
	}

	if (ui.image_path_edit) {
		ui.image_path_edit->set_text(String(p_surface.get("image_path", "")));
	}
	String cm = String(p_surface.get("center_mode", "stretch")).to_lower();
	if (ui.center_mode_opt) {
		ui.center_mode_opt->select(cm == "repeat" ? 1 : 0);
	}
	bool pa = p_surface.has("pixel_art") ? bool(p_surface["pixel_art"]) : true;
	if (ui.pixel_art_chk) {
		ui.pixel_art_chk->set_pressed_no_signal(pa);
	}
	Dictionary slice = (p_surface.has("nine_slice") && p_surface["nine_slice"].get_type() == Variant::DICTIONARY) ? Dictionary(p_surface["nine_slice"]) : Dictionary();
	int s_vals[4] = {
		int(slice.get("left", 8)),
		int(slice.get("top", 8)),
		int(slice.get("right", 8)),
		int(slice.get("bottom", 8)),
	};
	for (int s = 0; s < 4; s++) {
		if (ui.slice_sliders[s]) ui.slice_sliders[s]->set_value_no_signal(s_vals[s]);
		if (ui.slice_spins[s]) ui.slice_spins[s]->set_value_no_signal(s_vals[s]);
	}
	if (ui.image_section) {
		ui.image_section->set_visible(bt == "image");
	}
	bool lock_fields = (p_allow_inherit_toggle && inherit);
	if (ui.editable_body) {
		ui.editable_body->set_modulate(lock_fields ? Color(1, 1, 1, 0.55f) : Color(1, 1, 1, 1));
	}
	if (ui.border_type_opt) ui.border_type_opt->set_disabled(lock_fields);
	if (ui.border_width_slider) ui.border_width_slider->set_editable(!lock_fields && bt != "none" && bt != "image");
	if (ui.border_width_spin) ui.border_width_spin->set_editable(!lock_fields && bt != "none" && bt != "image");
	if (ui.corner_radius_slider) ui.corner_radius_slider->set_editable(!lock_fields && bt == "rounded");
	if (ui.corner_radius_spin) ui.corner_radius_spin->set_editable(!lock_fields && bt == "rounded");
	if (ui.border_color_btn) ui.border_color_btn->set_disabled(lock_fields);
	if (ui.fill_type_opt) ui.fill_type_opt->set_disabled(lock_fields);
	if (ui.opacity_slider) ui.opacity_slider->set_editable(!lock_fields);
	if (ui.opacity_spin) ui.opacity_spin->set_editable(!lock_fields);
	if (ui.fill_color_btn) ui.fill_color_btn->set_disabled(lock_fields);
	if (ui.font_color_btn) ui.font_color_btn->set_disabled(lock_fields);
	if (ui.fill_image_edit) ui.fill_image_edit->set_editable(!lock_fields);
	if (ui.fill_image_import_btn) ui.fill_image_import_btn->set_disabled(lock_fields);
	if (ui.fill_image_clear_btn) ui.fill_image_clear_btn->set_disabled(lock_fields);
	if (ui.padding_slider) ui.padding_slider->set_editable(!lock_fields);
	if (ui.padding_spin) ui.padding_spin->set_editable(!lock_fields);
	for (int s = 0; s < 4; s++) {
		if (ui.padding_side_spins[s]) ui.padding_side_spins[s]->set_editable(!lock_fields);
		if (ui.slice_sliders[s]) ui.slice_sliders[s]->set_editable(!lock_fields);
		if (ui.slice_spins[s]) ui.slice_spins[s]->set_editable(!lock_fields);
	}
	if (ui.image_path_edit) ui.image_path_edit->set_editable(!lock_fields);
	if (ui.image_import_btn) ui.image_import_btn->set_disabled(lock_fields);
	if (ui.image_clear_btn) ui.image_clear_btn->set_disabled(lock_fields);
	if (ui.center_mode_opt) ui.center_mode_opt->set_disabled(lock_fields);
	if (ui.pixel_art_chk) ui.pixel_art_chk->set_disabled(lock_fields);

	_update_ws_slice_preview(p_slot_idx);
	updating_ui = prev_upd;
}

Dictionary ClockGearsDatabaseDialog::_read_ws_surface_ui(int p_slot_idx, const Dictionary &p_base) const {
	const StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	Dictionary d = p_base.duplicate(true);
	if (ui.inherit_normal_chk && ui.inherit_normal_chk->is_visible()) {
		d["inherit_normal"] = ui.inherit_normal_chk->is_pressed();
	}
	if (ui.border_type_opt) {
		d["border_type"] = s_ws_idx_to_border_type(ui.border_type_opt->get_selected());
	}
	if (ui.border_width_spin) {
		d["border_width"] = int(Math::round(ui.border_width_spin->get_value()));
	}
	if (ui.corner_radius_spin) {
		d["corner_radius"] = int(Math::round(ui.corner_radius_spin->get_value()));
	}
	if (ui.border_color_btn) {
		d["border_color"] = "#" + ui.border_color_btn->get_pick_color().to_html(true);
	}
	if (ui.fill_type_opt) {
		d["fill_type"] = (ui.fill_type_opt->get_selected() == 1) ? "image" : "color";
	}
	if (ui.fill_color_btn) {
		Color fc = ui.fill_color_btn->get_pick_color();
		if (ui.opacity_spin) {
			fc.a = CLAMP(float(ui.opacity_spin->get_value() / 100.0), 0.0f, 1.0f);
		}
		d["fill_color"] = "#" + fc.to_html(true);
	}
	if (ui.font_color_btn) {
		d["font_color"] = "#" + ui.font_color_btn->get_pick_color().to_html(true);
	}
	if (ui.fill_image_edit) {
		d["fill_image_path"] = ui.fill_image_edit->get_text().strip_edges();
	}
	Dictionary pad;
	pad["left"] = ui.padding_side_spins[0] ? int(Math::round(ui.padding_side_spins[0]->get_value())) : 12;
	pad["top"] = ui.padding_side_spins[1] ? int(Math::round(ui.padding_side_spins[1]->get_value())) : 10;
	pad["right"] = ui.padding_side_spins[2] ? int(Math::round(ui.padding_side_spins[2]->get_value())) : 12;
	pad["bottom"] = ui.padding_side_spins[3] ? int(Math::round(ui.padding_side_spins[3]->get_value())) : 10;
	d["padding"] = pad;

	if (ui.image_path_edit) {
		d["image_path"] = ui.image_path_edit->get_text().strip_edges();
	}
	if (ui.center_mode_opt) {
		d["center_mode"] = (ui.center_mode_opt->get_selected() == 1) ? "repeat" : "stretch";
	}
	if (ui.pixel_art_chk) {
		d["pixel_art"] = ui.pixel_art_chk->is_pressed();
	}
	Dictionary slice;
	slice["left"] = ui.slice_spins[0] ? int(Math::round(ui.slice_spins[0]->get_value())) : 8;
	slice["top"] = ui.slice_spins[1] ? int(Math::round(ui.slice_spins[1]->get_value())) : 8;
	slice["right"] = ui.slice_spins[2] ? int(Math::round(ui.slice_spins[2]->get_value())) : 8;
	slice["bottom"] = ui.slice_spins[3] ? int(Math::round(ui.slice_spins[3]->get_value())) : 8;
	d["nine_slice"] = slice;
	return d;
}

void ClockGearsDatabaseDialog::_update_ws_slice_preview(int p_slot_idx) {
	const StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	if (!ui.slice_preview) {
		return;
	}
	String img_path = ui.image_path_edit ? ui.image_path_edit->get_text().strip_edges() : String();
	Ref<Texture2D> tex = img_path.is_empty() ? Ref<Texture2D>() : ClockGearsWindowStyleManager::load_texture_safe(img_path);
	int sl = ui.slice_spins[0] ? int(Math::round(ui.slice_spins[0]->get_value())) : 8;
	int st = ui.slice_spins[1] ? int(Math::round(ui.slice_spins[1]->get_value())) : 8;
	int sr = ui.slice_spins[2] ? int(Math::round(ui.slice_spins[2]->get_value())) : 8;
	int sb = ui.slice_spins[3] ? int(Math::round(ui.slice_spins[3]->get_value())) : 8;
	bool pa = ui.pixel_art_chk ? ui.pixel_art_chk->is_pressed() : true;
	ui.slice_preview->set_preview_data(tex, sl, st, sr, sb, pa);
}

void ClockGearsDatabaseDialog::_refresh_window_styles_editor_ui() {
	if (!window_styles_editor_panel) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;

	window_styles_config_draft = ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
	Array styles = window_styles_config_draft.get("styles", Array());
	if (styles.is_empty()) {
		updating_ui = prev_upd;
		return;
	}
	ws_selected_index = CLAMP(ws_selected_index, 0, styles.size() - 1);
	Dictionary st = styles[ws_selected_index];
	bool is_bi = bool(st.get("built_in", false));

	if (btn_remove_record && current_category == DB_CAT_WINDOW_STYLES) {
		btn_remove_record->set_disabled(is_bi || styles.size() <= 1);
		btn_remove_record->set_tooltip_text(is_bi
				? CG_DB_TR("Los estilos incluidos no se pueden borrar")
				: CG_DB_TR("Eliminar estilo seleccionado"));
	}
	if (ws_name_edit) {
		ws_name_edit->set_text(String(st.get("name", "")));
	}
	if (ws_builtin_badge) {
		ws_builtin_badge->set_visible(is_bi);
	}
	_populate_ws_surface_ui(0, st, false);

	ws_active_btn_tab = CLAMP(ws_active_btn_tab, 0, 3);
	if (ws_btn_state_tabs && ws_btn_state_tabs->get_current_tab() != ws_active_btn_tab) {
		ws_btn_state_tabs->set_current_tab(ws_active_btn_tab);
	}
	Dictionary btns = (st.has("buttons") && st["buttons"].get_type() == Variant::DICTIONARY) ? Dictionary(st["buttons"]) : Dictionary();
	String sk = _ws_btn_state_key(ws_active_btn_tab);
	Dictionary btn_surf = (btns.has(sk) && btns[sk].get_type() == Variant::DICTIONARY) ? Dictionary(btns[sk]) : st;
	_populate_ws_surface_ui(1, btn_surf, ws_active_btn_tab > 0);

	_refresh_ws_assignments_ui();
	_update_window_style_live_preview();

	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_refresh_ws_assignments_ui() {
	if (!ws_default_style_opt || !ws_assignments_grid) {
		return;
	}
	ws_rebuilding_assignments = true;
	Array styles = window_styles_config_draft.get("styles", Array());
	String def_id = String(window_styles_config_draft.get("default_style_id", "rounded"));
	Dictionary assigns = window_styles_config_draft.get("assignments", Dictionary());

	ws_default_style_opt->clear();
	int def_sel = 0;
	for (int i = 0; i < styles.size(); i++) {
		Dictionary st = styles[i];
		String sid = String(st.get("id", ""));
		String sname = String(st.get("name", sid));
		ws_default_style_opt->add_item(sname, i);
		ws_default_style_opt->set_item_metadata(i, sid);
		if (sid == def_id) {
			def_sel = i;
		}
	}
	if (ws_default_style_opt->get_item_count() > 0) {
		ws_default_style_opt->select(def_sel);
	}

	for (int i = ws_assignments_grid->get_child_count() - 1; i >= 0; i--) {
		Node *ch = ws_assignments_grid->get_child(i);
		ws_assignments_grid->remove_child(ch);
		memdelete(ch);
	}
	ws_assignment_opts.clear();
	ws_assignment_type_ids.clear();

	Array wtypes = ClockGearsDataManager::get_registered_window_types();
	const float row_h = 28.0f * EDSCALE;
	for (int r = 0; r < wtypes.size(); r++) {
		if (wtypes[r].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary wt = wtypes[r];
		String tid = String(wt.get("id", ""));
		String tlabel = String(wt.get("label", tid));
		String assigned_id = String(assigns.get(tid, ""));

		Label *lbl = memnew(Label(tlabel + ":"));
		lbl->set_custom_minimum_size(Size2(130 * EDSCALE, 0));
		ws_assignments_grid->add_child(lbl);

		OptionButton *opt = memnew(OptionButton);
		opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
		opt->add_item(CG_DB_TR("(Usar estilo por defecto)"), 0);
		opt->set_item_metadata(0, "");
		int sel_idx = 0;
		for (int i = 0; i < styles.size(); i++) {
			Dictionary st = styles[i];
			String sid = String(st.get("id", ""));
			String sname = String(st.get("name", sid));
			opt->add_item(sname, i + 1);
			opt->set_item_metadata(i + 1, sid);
			if (sid == assigned_id && !assigned_id.is_empty()) {
				sel_idx = i + 1;
			}
		}
		opt->select(sel_idx);
		int row_index = ws_assignment_opts.size();
		opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_ws_assignment_selected).bind(row_index));
		ws_assignments_grid->add_child(opt);
		ws_assignment_opts.push_back(opt);
		ws_assignment_type_ids.push_back(tid);
	}
	ws_rebuilding_assignments = false;
}

void ClockGearsDatabaseDialog::_sync_style_editor_to_draft() {
	Array styles = window_styles_config_draft.get("styles", Array());
	if (ws_selected_index < 0 || ws_selected_index >= styles.size()) {
		return;
	}
	Dictionary st = styles[ws_selected_index];
	if (ws_name_edit) {
		String nm = ws_name_edit->get_text().strip_edges();
		if (!nm.is_empty()) {
			st["name"] = nm;
		}
	}
	st = _read_ws_surface_ui(0, st);

	Dictionary btns = (st.has("buttons") && st["buttons"].get_type() == Variant::DICTIONARY) ? Dictionary(st["buttons"]) : Dictionary();
	String sk = _ws_btn_state_key(ws_active_btn_tab);
	Dictionary cur_btn = (btns.has(sk) && btns[sk].get_type() == Variant::DICTIONARY) ? Dictionary(btns[sk]) : Dictionary();
	btns[sk] = _read_ws_surface_ui(1, cur_btn);
	st["buttons"] = btns;

	st = ClockGearsDataManager::normalize_window_style(st);
	styles[ws_selected_index] = st;
	window_styles_config_draft["styles"] = styles;
}

void ClockGearsDatabaseDialog::_update_window_style_live_preview() {
	Array styles = window_styles_config_draft.get("styles", Array());
	if (styles.is_empty() || ws_selected_index < 0 || ws_selected_index >= styles.size()) {
		return;
	}
	Dictionary st = ClockGearsDataManager::normalize_window_style(styles[ws_selected_index]);
	String style_id = String(st.get("id", "rounded"));

	int vp_w = ws_preview_size_opt ? ws_preview_size_opt->get_selected_id() : 960;
	if (vp_w <= 0) {
		vp_w = 960;
	}
	float raw_scale = float(vp_w) / 960.0f;
	auto compute_surf_scale = [&](const Dictionary &p_surf) -> float {
		bool pa = p_surf.has("pixel_art") ? bool(p_surf["pixel_art"]) : true;
		return pa ? MAX(1.0f, float(Math::round(raw_scale))) : MAX(0.5f, raw_scale);
	};

	if (ws_preview_window) {
		float win_scale = compute_surf_scale(st);
		Ref<ClockGearsWindowStyleBox> win_sb;
		win_sb.instantiate();
		if (!win_sb->configure_from_surface(st, win_scale, style_id, "message")) {
			Dictionary fb = ClockGearsDataManager::normalize_window_style(Dictionary());
			win_sb->configure_from_surface(fb, win_scale, style_id, "message");
			win_sb->set_fallback_used(true);
		}
		ws_preview_window->add_theme_style_override("panel", win_sb);

		Color fc = win_sb->get_font_color();
		int title_fs = CLAMP(int(Math::round(15.0f * CLAMP(win_scale * 0.72f, 0.9f, 1.7f) * EDSCALE)), 13, 30);
		int body_fs = CLAMP(int(Math::round(13.0f * CLAMP(win_scale * 0.72f, 0.9f, 1.7f) * EDSCALE)), 12, 26);
		if (ws_preview_window_title) {
			ws_preview_window_title->add_theme_color_override("font_color", fc);
			ws_preview_window_title->add_theme_font_size_override("font_size", title_fs);
		}
		if (ws_preview_window_body) {
			ws_preview_window_body->add_theme_color_override("font_color", fc);
			ws_preview_window_body->add_theme_font_size_override("font_size", body_fs);
		}
	}

	Dictionary btns = st.get("buttons", Dictionary());
	for (int i = 0; i < 4; i++) {
		Button *pb = ws_preview_btns[i];
		if (!pb) {
			continue;
		}
		String sk = _ws_btn_state_key(i);
		Dictionary bsurf = (btns.has(sk) && btns[sk].get_type() == Variant::DICTIONARY) ? Dictionary(btns[sk]) : st;
		float bscale = compute_surf_scale(bsurf);
		Ref<ClockGearsWindowStyleBox> bsb;
		bsb.instantiate();
		if (!bsb->configure_from_surface(bsurf, bscale, style_id, "title_buttons")) {
			Dictionary fb = ClockGearsDataManager::normalize_window_style(Dictionary());
			Dictionary fb_btns = fb.get("buttons", Dictionary());
			bsb->configure_from_surface(Dictionary(fb_btns.get(sk, fb)), bscale, style_id, "title_buttons");
			bsb->set_fallback_used(true);
		}
		pb->add_theme_style_override("normal", bsb);
		pb->add_theme_style_override("hover", bsb);
		pb->add_theme_style_override("pressed", bsb);
		pb->add_theme_style_override("disabled", bsb);
		pb->add_theme_style_override("focus", bsb);

		Color tc = bsb->get_font_color();
		pb->add_theme_color_override("font_color", tc);
		pb->add_theme_color_override("font_hover_color", tc);
		pb->add_theme_color_override("font_pressed_color", tc);
		pb->add_theme_color_override("font_disabled_color", tc);
		pb->add_theme_color_override("font_focus_color", tc);
		int bfs = CLAMP(int(Math::round(13.0f * CLAMP(bscale * 0.70f, 0.88f, 1.55f) * EDSCALE)), 11, 24);
		pb->add_theme_font_size_override("font_size", bfs);
	}
}

void ClockGearsDatabaseDialog::_on_ws_name_changed(const String &p_text) {
	if (updating_ui) {
		return;
	}
	_sync_style_editor_to_draft();
	_refresh_record_list();
	if (record_list && ws_selected_index >= 0 && ws_selected_index < record_list->get_item_count()) {
		record_list->select(ws_selected_index);
	}
	_refresh_ws_assignments_ui();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_ws_btn_tab_changed(int p_tab_idx) {
	if (updating_ui) {
		return;
	}
	ws_active_btn_tab = CLAMP(p_tab_idx, 0, 3);
	_refresh_window_styles_editor_ui();
}

void ClockGearsDatabaseDialog::_on_ws_surface_changed_v(const Variant &p_unused, int p_slot_idx) {
	_on_ws_surface_changed(p_slot_idx);
}

void ClockGearsDatabaseDialog::_on_ws_surface_changed(int p_slot_idx) {
	if (updating_ui) {
		return;
	}
	_sync_style_editor_to_draft();
	Array styles = window_styles_config_draft.get("styles", Array());
	if (ws_selected_index >= 0 && ws_selected_index < styles.size()) {
		Dictionary st = styles[ws_selected_index];
		if (p_slot_idx == 0) {
			_populate_ws_surface_ui(0, st, false);
		} else {
			Dictionary btns = st.get("buttons", Dictionary());
			String sk = _ws_btn_state_key(ws_active_btn_tab);
			_populate_ws_surface_ui(1, Dictionary(btns.get(sk, st)), ws_active_btn_tab > 0);
		}
	}
	_update_window_style_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_ws_opacity_changed(double p_val, int p_slot_idx) {
	if (updating_ui) {
		return;
	}
	StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	if (ui.fill_color_btn) {
		Color c = ui.fill_color_btn->get_pick_color();
		c.a = CLAMP(float(p_val / 100.0), 0.0f, 1.0f);
		ui.fill_color_btn->set_pick_color(c);
	}
	_on_ws_surface_changed(p_slot_idx);
}

void ClockGearsDatabaseDialog::_on_ws_uniform_padding_changed(double p_val, int p_slot_idx) {
	if (updating_ui) {
		return;
	}
	StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	bool prev_upd = updating_ui;
	updating_ui = true;
	for (int s = 0; s < 4; s++) {
		if (ui.padding_side_spins[s]) {
			ui.padding_side_spins[s]->set_value_no_signal(p_val);
		}
	}
	updating_ui = prev_upd;
	_on_ws_surface_changed(p_slot_idx);
}

void ClockGearsDatabaseDialog::_on_ws_fill_color_changed(const Color &p_col, int p_slot_idx) {
	if (updating_ui) {
		return;
	}
	StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	bool prev_upd = updating_ui;
	updating_ui = true;
	int pct = CLAMP(int(Math::round(p_col.a * 100.0f)), 0, 100);
	if (ui.opacity_slider) ui.opacity_slider->set_value_no_signal(pct);
	if (ui.opacity_spin) ui.opacity_spin->set_value_no_signal(pct);
	updating_ui = prev_upd;
	_on_ws_surface_changed(p_slot_idx);
}

void ClockGearsDatabaseDialog::_on_ws_import_image_pressed(int p_slot_idx, int p_kind) {
	if (!ws_image_file_dialog) {
		return;
	}
	ws_image_pick_slot = p_slot_idx;
	ws_image_pick_kind = p_kind;
	ws_image_file_dialog->clear_filters();
	ws_image_file_dialog->set_title(p_kind == 0 ? CG_DB_TR("Importar imagen de marco (9 porciones)") : CG_DB_TR("Importar imagen de relleno"));
	ws_image_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes"));
	ws_image_file_dialog->popup_centered_ratio(0.60f);
}

void ClockGearsDatabaseDialog::_on_ws_clear_image_pressed(int p_slot_idx, int p_kind) {
	StyleSurfaceUI &ui = ws_surface_ui[p_slot_idx];
	if (p_kind == 0) {
		if (ui.image_path_edit) ui.image_path_edit->set_text("");
	} else {
		if (ui.fill_image_edit) ui.fill_image_edit->set_text("");
		if (ui.fill_type_opt) ui.fill_type_opt->select(0);
	}
	_on_ws_surface_changed(p_slot_idx);
}

void ClockGearsDatabaseDialog::_on_ws_image_file_selected(const String &p_path) {
	StyleSurfaceUI &ui = ws_surface_ui[ws_image_pick_slot];
	bool pa = ui.pixel_art_chk ? ui.pixel_art_chk->is_pressed() : true;
	String final_path = _import_menu_asset_file(p_path, "img/ui", pa);
	if (ws_image_pick_kind == 0) {
		if (ui.image_path_edit) ui.image_path_edit->set_text(final_path);
		if (ui.border_type_opt) ui.border_type_opt->select(3);
	} else {
		if (ui.fill_image_edit) ui.fill_image_edit->set_text(final_path);
		if (ui.fill_type_opt) ui.fill_type_opt->select(1);
	}
	_on_ws_surface_changed(ws_image_pick_slot);
}

void ClockGearsDatabaseDialog::_on_ws_preview_size_selected(int p_idx) {
	if (!updating_ui && ws_preview_size_opt && p_idx >= 0) {
		int ref_w = ws_preview_size_opt->get_item_id(p_idx);
		if (ref_w == 960) {
			set_menus_preview_resolution(960, 540);
			return;
		} else if (ref_w == 1366) {
			set_menus_preview_resolution(1366, 768);
			return;
		} else if (ref_w == 1920) {
			set_menus_preview_resolution(1920, 1080);
			return;
		}
	}
	_update_window_style_live_preview();
}

void ClockGearsDatabaseDialog::_on_ws_default_style_selected(int p_idx) {
	if (ws_rebuilding_assignments || updating_ui || !ws_default_style_opt) {
		return;
	}
	String sid = String(ws_default_style_opt->get_item_metadata(p_idx));
	if (!sid.is_empty()) {
		window_styles_config_draft["default_style_id"] = sid;
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_ws_assignment_selected(int p_idx, int p_row_idx) {
	if (ws_rebuilding_assignments || updating_ui || p_row_idx < 0 || p_row_idx >= ws_assignment_opts.size() || p_row_idx >= ws_assignment_type_ids.size()) {
		return;
	}
	OptionButton *opt = ws_assignment_opts[p_row_idx];
	String tid = ws_assignment_type_ids[p_row_idx];
	if (!opt || tid.is_empty()) {
		return;
	}
	Dictionary assigns = window_styles_config_draft.get("assignments", Dictionary());
	assigns[tid] = String(opt->get_item_metadata(p_idx));
	window_styles_config_draft["assignments"] = assigns;
	_mark_database_dirty();
}

Dictionary ClockGearsDatabaseDialog::get_window_styles_draft_copy() const {
	return ClockGearsDataManager::normalize_window_styles_config(window_styles_config_draft);
}

void ClockGearsDatabaseDialog::select_window_style(int p_index) {
	if (current_category != DB_CAT_WINDOW_STYLES) {
		_on_category_selected(DB_CAT_WINDOW_STYLES);
	}
	if (record_list && p_index >= 0 && p_index < record_list->get_item_count()) {
		record_list->select(p_index);
		_on_record_selected(p_index);
	}
}

Dictionary ClockGearsDatabaseDialog::get_window_styles_ui_handles() {
	_setup_window_styles_editor_ui();
	Dictionary out;
	out["panel"] = window_styles_editor_panel;
	out["main_split"] = ws_main_split;
	out["name_edit"] = ws_name_edit;
	out["btn_state_tabs"] = ws_btn_state_tabs;
	out["preview_size_opt"] = ws_preview_size_opt;
	out["default_style_opt"] = ws_default_style_opt;
	out["record_list"] = record_list;
	out["btn_add"] = btn_add_record;
	out["btn_remove"] = btn_remove_record;
	out["btn_copy"] = btn_copy_record;
	out["btn_paste"] = btn_paste_record;
	out["btn_max"] = btn_change_max;

	Dictionary assigns;
	for (int i = 0; i < ws_assignment_opts.size() && i < ws_assignment_type_ids.size(); i++) {
		assigns[ws_assignment_type_ids[i]] = ws_assignment_opts[i];
	}
	out["assignment_opts"] = assigns;

	auto pack_surf = [&](int s) -> Dictionary {
		const StyleSurfaceUI &u = ws_surface_ui[s];
		Dictionary d;
		d["inherit_normal_chk"] = u.inherit_normal_chk;
		d["border_type_opt"] = u.border_type_opt;
		d["border_width_slider"] = u.border_width_slider;
		d["border_width_spin"] = u.border_width_spin;
		d["corner_radius_slider"] = u.corner_radius_slider;
		d["corner_radius_spin"] = u.corner_radius_spin;
		d["opacity_slider"] = u.opacity_slider;
		d["opacity_spin"] = u.opacity_spin;
		d["padding_slider"] = u.padding_slider;
		d["padding_spin"] = u.padding_spin;
		d["border_color_btn"] = u.border_color_btn;
		d["fill_color_btn"] = u.fill_color_btn;
		d["font_color_btn"] = u.font_color_btn;
		d["fill_type_opt"] = u.fill_type_opt;
		d["fill_image_edit"] = u.fill_image_edit;
		d["image_path_edit"] = u.image_path_edit;
		d["center_mode_opt"] = u.center_mode_opt;
		d["pixel_art_chk"] = u.pixel_art_chk;
		d["slice_preview"] = u.slice_preview;
		Array sl, sp;
		for (int k = 0; k < 4; k++) {
			sl.push_back(u.slice_sliders[k]);
			sp.push_back(u.slice_spins[k]);
		}
		d["slice_sliders"] = sl;
		d["slice_spins"] = sp;
		return d;
	};
	out["window_surface"] = pack_surf(0);
	out["button_surface"] = pack_surf(1);
	return out;
}

Dictionary ClockGearsDatabaseDialog::get_window_style_preview_info() {
	_setup_window_styles_editor_ui();
	Dictionary out;
	if (ws_preview_window) {
		Ref<StyleBox> sb = ws_preview_window->get_theme_stylebox("panel");
		ClockGearsWindowStyleBox *wsb = Object::cast_to<ClockGearsWindowStyleBox>(sb.ptr());
		if (wsb) {
			out["window"] = ClockGearsWindowStyleManager::inspect_control_style(ws_preview_window);
			out["window_rect"] = ws_preview_window->get_global_rect();
		}
	}
	Array btns;
	for (int i = 0; i < 4; i++) {
		Button *pb = ws_preview_btns[i];
		if (!pb) {
			continue;
		}
		Dictionary bi = ClockGearsWindowStyleManager::inspect_control_style(pb);
		bi["state"] = String(_ws_btn_state_key(i));
		bi["disabled"] = pb->is_disabled();
		bi["rect"] = pb->get_global_rect();
		btns.push_back(bi);
	}
	out["buttons"] = btns;
	out["preview_size_id"] = ws_preview_size_opt ? ws_preview_size_opt->get_selected_id() : 960;
	return out;
}

// ============================================================================
// Menús -> Menú del juego Editor (9B Tanda 3 UI)
// ============================================================================

static String _gm_entry_default_label(const String &p_id) {
	if (p_id == "items") return CG_DB_TR("Objetos");
	if (p_id == "equip") return CG_DB_TR("Equipo");
	if (p_id == "status") return CG_DB_TR("Estado");
	if (p_id == "save") return CG_DB_TR("Guardar");
	if (p_id == "options") return CG_DB_TR("Opciones");
	if (p_id == "to_title") return CG_DB_TR("Salir al título");
	return p_id;
}

const char *ClockGearsDatabaseDialog::_gm_bg_screen_key(int p_tab_idx) {
	switch (p_tab_idx) {
		case 1: return "items";
		case 2: return "equip";
		case 3: return "status";
		default: return "menu";
	}
}

void ClockGearsDatabaseDialog::_setup_game_menu_editor_ui() {
	if (game_menu_editor_panel || !col2_box) {
		return;
	}
	const float row_h = 30.0f * EDSCALE;
	game_menu_config_draft = ClockGearsDataManager::get_default_game_menu_config();
	if (ClockGearsDataManager::get_singleton()) {
		game_menu_config_draft = ClockGearsDataManager::normalize_game_menu_config(ClockGearsDataManager::get_singleton()->get_game_menu_config());
	}
	gm_active_bg_screen = "menu";

	game_menu_editor_panel = memnew(VBoxContainer);
	game_menu_editor_panel->set_name("GameMenuEditorPanel");
	game_menu_editor_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	game_menu_editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	game_menu_editor_panel->add_theme_constant_override("separation", int(12 * EDSCALE));
	game_menu_editor_panel->hide();
	col2_box->add_child(game_menu_editor_panel);

	gm_main_split = memnew(BoxContainer);
	gm_main_split->set_vertical(false);
	gm_main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	gm_main_split->add_theme_constant_override("separation", int(14 * EDSCALE));
	game_menu_editor_panel->add_child(gm_main_split);

	gm_left_col = memnew(VBoxContainer);
	gm_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_left_col->set_stretch_ratio(1.1f);
	gm_left_col->add_theme_constant_override("separation", int(12 * EDSCALE));
	gm_main_split->add_child(gm_left_col);

	auto make_card = [&](const String &p_title, VBoxContainer *&r_body) {
		_make_collapsible_menu_card(gm_left_col, p_title, r_body, true);
	};

	// 1) Entradas del menú del juego
	VBoxContainer *entries_card = nullptr;
	make_card(CG_DB_TR("Entradas del menú del juego"), entries_card);

	Label *entries_hint = memnew(Label(CG_DB_TR("Activa, renombra o reordena las opciones del menú de pausa en el mapa.")));
	entries_hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	entries_hint->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	entries_card->add_child(entries_hint);

	gm_entries_list_vbox = memnew(VBoxContainer);
	gm_entries_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_entries_list_vbox->add_theme_constant_override("separation", int(6 * EDSCALE));
	entries_card->add_child(gm_entries_list_vbox);

	// 2) Fondo por pantalla
	VBoxContainer *bg_card = nullptr;
	make_card(CG_DB_TR("Fondo por pantalla"), bg_card);

	gm_bg_screen_tabs = memnew(TabBar);
	gm_bg_screen_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_bg_screen_tabs->set_clip_tabs(true);
	gm_bg_screen_tabs->add_tab(CG_DB_TR("Menú"));
	gm_bg_screen_tabs->add_tab(CG_DB_TR("Objetos"));
	gm_bg_screen_tabs->add_tab(CG_DB_TR("Equipo"));
	gm_bg_screen_tabs->add_tab(CG_DB_TR("Estado"));
	gm_bg_screen_tabs->set_current_tab(0);
	gm_bg_screen_tabs->connect("tab_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_screen_tab_changed));
	bg_card->add_child(gm_bg_screen_tabs);

	GridContainer *bg_grid = memnew(GridContainer);
	bg_grid->set_columns(2);
	bg_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_grid->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	bg_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	bg_card->add_child(bg_grid);

	auto add_grid_label = [&](GridContainer *p_grid, const String &p_text) {
		Label *l = memnew(Label(p_text));
		l->set_custom_minimum_size(Size2(110 * EDSCALE, 0));
		p_grid->add_child(l);
	};

	add_grid_label(bg_grid, CG_DB_TR("Tipo:"));
	gm_bg_type_opt = memnew(OptionButton);
	gm_bg_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_bg_type_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_bg_type_opt->add_item(CG_DB_TR("Color sólido"), 0);
	gm_bg_type_opt->add_item(CG_DB_TR("Imagen"), 1);
	gm_bg_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_field_changed));
	bg_grid->add_child(gm_bg_type_opt);

	add_grid_label(bg_grid, CG_DB_TR("Color:"));
	gm_bg_color_btn = memnew(ColorPickerButton);
	gm_bg_color_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_bg_color_btn->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	gm_bg_color_btn->set_edit_alpha(false);
	gm_bg_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_color_changed));
	bg_grid->add_child(gm_bg_color_btn);

	add_grid_label(bg_grid, CG_DB_TR("Imagen:"));
	HFlowContainer *bg_img_hb = memnew(HFlowContainer);
	bg_img_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_img_hb->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	bg_img_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	gm_bg_image_edit = memnew(LineEdit);
	gm_bg_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_bg_image_edit->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	gm_bg_image_edit->set_placeholder("res://img/menus/...");
	gm_bg_image_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_image_text_changed));
	bg_img_hb->add_child(gm_bg_image_edit);

	gm_bg_import_btn = memnew(Button(CG_DB_TR("Importar")));
	gm_bg_import_btn->set_custom_minimum_size(Size2(72 * EDSCALE, row_h));
	gm_bg_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_import_bg_pressed));
	bg_img_hb->add_child(gm_bg_import_btn);

	gm_bg_clear_btn = memnew(Button(CG_DB_TR("Limpiar")));
	gm_bg_clear_btn->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
	gm_bg_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_clear_bg_pressed));
	bg_img_hb->add_child(gm_bg_clear_btn);
	bg_grid->add_child(bg_img_hb);

	add_grid_label(bg_grid, CG_DB_TR("Modo imagen:"));
	gm_bg_scale_opt = memnew(OptionButton);
	gm_bg_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_bg_scale_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_bg_scale_opt->add_item(CG_DB_TR("Ajustar (Fit)"), 0);
	gm_bg_scale_opt->add_item(CG_DB_TR("Rellenar (Fill)"), 1);
	gm_bg_scale_opt->add_item(CG_DB_TR("Estirar (Stretch)"), 2);
	gm_bg_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_field_changed));
	bg_grid->add_child(gm_bg_scale_opt);

	add_grid_label(bg_grid, CG_DB_TR("Filtro:"));
	gm_bg_filter_opt = memnew(OptionButton);
	gm_bg_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_bg_filter_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_bg_filter_opt->add_item(CG_DB_TR("Nearest (Pixel Art)"), 0);
	gm_bg_filter_opt->add_item(CG_DB_TR("Linear (Suavizado)"), 1);
	gm_bg_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_field_changed));
	bg_grid->add_child(gm_bg_filter_opt);

	// 3) Vista previa de objeto
	VBoxContainer *prev_obj_card = nullptr;
	make_card(CG_DB_TR("Vista previa de objeto"), prev_obj_card);

	Label *prev_obj_hint = memnew(Label(CG_DB_TR("Configura el panel lateral de vista previa al pasar el ratón o navegar con teclado en Objetos y Equipo.")));
	prev_obj_hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	prev_obj_hint->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	prev_obj_card->add_child(prev_obj_hint);

	GridContainer *prev_grid = memnew(GridContainer);
	prev_grid->set_columns(2);
	prev_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_grid->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	prev_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	prev_obj_card->add_child(prev_grid);

	add_grid_label(prev_grid, CG_DB_TR("Activar vista previa:"));
	gm_item_prev_enabled_chk = memnew(CheckBox);
	gm_item_prev_enabled_chk->set_text(CG_DB_TR("Activada"));
	gm_item_prev_enabled_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	prev_grid->add_child(gm_item_prev_enabled_chk);

	add_grid_label(prev_grid, CG_DB_TR("Posición:"));
	gm_item_prev_pos_opt = memnew(OptionButton);
	gm_item_prev_pos_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_item_prev_pos_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_item_prev_pos_opt->add_item(CG_DB_TR("Derecha"), 0);
	gm_item_prev_pos_opt->add_item(CG_DB_TR("Izquierda"), 1);
	gm_item_prev_pos_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	prev_grid->add_child(gm_item_prev_pos_opt);

	add_grid_label(prev_grid, CG_DB_TR("Mostrar elementos:"));
	HFlowContainer *elem_hb = memnew(HFlowContainer);
	elem_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	elem_hb->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	elem_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	gm_item_prev_show_icon_chk = memnew(CheckBox);
	gm_item_prev_show_icon_chk->set_text(CG_DB_TR("Icono"));
	gm_item_prev_show_icon_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	elem_hb->add_child(gm_item_prev_show_icon_chk);

	gm_item_prev_show_name_chk = memnew(CheckBox);
	gm_item_prev_show_name_chk->set_text(CG_DB_TR("Nombre"));
	gm_item_prev_show_name_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	elem_hb->add_child(gm_item_prev_show_name_chk);

	gm_item_prev_show_desc_chk = memnew(CheckBox);
	gm_item_prev_show_desc_chk->set_text(String::utf8("Descripción"));
	gm_item_prev_show_desc_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	elem_hb->add_child(gm_item_prev_show_desc_chk);
	prev_grid->add_child(elem_hb);

	add_grid_label(prev_grid, CG_DB_TR("Tamaño icono:"));
	gm_item_prev_scale_opt = memnew(OptionButton);
	gm_item_prev_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_item_prev_scale_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_item_prev_scale_opt->add_item(CG_DB_TR("x2 (64x64)"), 2);
	gm_item_prev_scale_opt->add_item(CG_DB_TR("x3 (96x96)"), 3);
	gm_item_prev_scale_opt->add_item(CG_DB_TR("x4 (128x128)"), 4);
	gm_item_prev_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	prev_grid->add_child(gm_item_prev_scale_opt);

	add_grid_label(prev_grid, String::utf8("Rotación por arrastre:"));
	HFlowContainer *rot_hb = memnew(HFlowContainer);
	rot_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rot_hb->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	rot_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	gm_item_prev_rot_chk = memnew(CheckBox);
	gm_item_prev_rot_chk->set_text(CG_DB_TR("Permitir"));
	gm_item_prev_rot_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	rot_hb->add_child(gm_item_prev_rot_chk);

	Label *step_lbl = memnew(Label(String::utf8("Paso (°):")));
	rot_hb->add_child(step_lbl);

	gm_item_prev_step_spin = memnew(SpinBox);
	gm_item_prev_step_spin->set_min(5);
	gm_item_prev_step_spin->set_max(45);
	gm_item_prev_step_spin->set_step(1);
	gm_item_prev_step_spin->set_value(15);
	gm_item_prev_step_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	rot_hb->add_child(gm_item_prev_step_spin);
	prev_grid->add_child(rot_hb);

	add_grid_label(prev_grid, CG_DB_TR("Estilo icono:"));
	gm_item_prev_style_opt = memnew(OptionButton);
	gm_item_prev_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_item_prev_style_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_item_prev_style_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	prev_grid->add_child(gm_item_prev_style_opt);

	add_grid_label(prev_grid, String::utf8("Estilo descripción:"));
	gm_item_desc_style_opt = memnew(OptionButton);
	gm_item_desc_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_item_desc_style_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	gm_item_desc_style_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed));
	prev_grid->add_child(gm_item_desc_style_opt);

	// Columna derecha: Vista previa en vivo + Probar
	gm_prev_card = memnew(PanelContainer);
	gm_prev_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gm_prev_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	gm_prev_card->set_stretch_ratio(1.0f);
	gm_prev_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	gm_main_split->add_child(gm_prev_card);

	VBoxContainer *prev_vb = memnew(VBoxContainer);
	prev_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	prev_vb->add_theme_constant_override("separation", int(10 * EDSCALE));
	gm_prev_card->add_child(prev_vb);

	gm_test_btn = memnew(Button(CG_DB_TR("Probar")));
	gm_test_btn->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	gm_test_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::open_game_menu_test_popup));

	_build_16_9_viewport_preview(prev_vb, gm_preview_viewport, gm_prev_title_lbl, gm_preview_res_opt, gm_test_btn);
	_sync_common_preview_res_options();

	gm_live_preview = memnew(ClockGearsGameMenu);
	gm_live_preview->set_preview_mode(true);
	gm_preview_viewport->add_child(gm_live_preview);

	gm_asset_file_dialog = memnew(FileDialog);
	gm_asset_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	gm_asset_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	gm_asset_file_dialog->set_title(CG_DB_TR("Importar imagen de fondo"));
	gm_asset_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes"));
	gm_asset_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_bg_file_selected));
	add_child(gm_asset_file_dialog);
}

void ClockGearsDatabaseDialog::_ensure_gm_test_window() {
	if (gm_test_window && gm_test_menu) {
		return;
	}
	gm_test_window = memnew(Window);
	gm_test_window->set_title(CG_DB_TR("Probar menú del juego (960x540)"));
	gm_test_window->set_visible(false);
	gm_test_window->set_transient(true);
	gm_test_window->set_content_scale_size(Size2i(960, 540));
	gm_test_window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
	gm_test_window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
	gm_test_window->connect("close_requested", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_test_popup_close_requested));
	add_child(gm_test_window);

	gm_test_menu = memnew(ClockGearsGameMenu);
	gm_test_menu->set_editor_test_popup_mode(true);
	gm_test_menu->connect("menu_closed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_test_popup_close_requested));
	gm_test_window->add_child(gm_test_menu);
}

void ClockGearsDatabaseDialog::_rebuild_game_menu_entries_rows() {
	if (!gm_entries_list_vbox) {
		return;
	}
	for (int i = gm_entries_list_vbox->get_child_count() - 1; i >= 0; i--) {
		Node *ch = gm_entries_list_vbox->get_child(i);
		gm_entries_list_vbox->remove_child(ch);
		memdelete(ch);
	}

	game_menu_config_draft = ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft);
	Array entries = game_menu_config_draft.get("entries", Array());
	Vector<Dictionary> sorted;
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary e = entries[i];
		String eid = String(e.get("id", ""));
		if (eid == "magic" || bool(e.get("reserved", false))) {
			continue; // Entrada reservada oculta por diseño
		}
		sorted.push_back(e);
	}
	struct Cmp {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("order", 0)) < int(b.get("order", 0));
		}
	};
	sorted.sort_custom<Cmp>();

	const float row_h = 30.0f * EDSCALE;
	for (int i = 0; i < sorted.size(); i++) {
		const Dictionary &e = sorted[i];
		String eid = String(e.get("id", ""));
		String etxt = String(e.get("text", _gm_entry_default_label(eid)));
		bool enabled = bool(e.get("enabled", true));

		HFlowContainer *row = memnew(HFlowContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->set_meta("cg_entry_id", eid);
		row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
		row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
		gm_entries_list_vbox->add_child(row);

		CheckBox *chk = memnew(CheckBox(CG_DB_TR("Activa")));
		chk->set_pressed_no_signal(enabled);
		chk->set_meta("cg_entry_id", eid);
		chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_entry_toggled).bind(eid));
		row->add_child(chk);

		LineEdit *edit = memnew(LineEdit);
		edit->set_text(etxt);
		edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		edit->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
		edit->set_meta("cg_entry_id", eid);
		edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_entry_text_changed).bind(eid));
		row->add_child(edit);

		Label *tag = memnew(Label(vformat("[%s]", _gm_entry_default_label(eid))));
		tag->set_custom_minimum_size(Size2(72 * EDSCALE, 0));
		tag->set_clip_text(true);
		tag->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
		row->add_child(tag);

		Button *btn_up = memnew(Button(String::utf8("↑")));
		btn_up->set_tooltip_text(CG_DB_TR("Subir"));
		btn_up->set_custom_minimum_size(Size2(32 * EDSCALE, row_h));
		btn_up->set_disabled(i == 0);
		btn_up->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_entry_move_pressed).bind(eid, -1));
		row->add_child(btn_up);

		Button *btn_down = memnew(Button(String::utf8("↓")));
		btn_down->set_tooltip_text(CG_DB_TR("Bajar"));
		btn_down->set_custom_minimum_size(Size2(32 * EDSCALE, row_h));
		btn_down->set_disabled(i == sorted.size() - 1);
		btn_down->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_gm_entry_move_pressed).bind(eid, 1));
		row->add_child(btn_down);
	}
}

void ClockGearsDatabaseDialog::_update_game_menu_live_preview() {
	game_menu_config_draft = ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft);
	if (gm_live_preview) {
		gm_live_preview->apply_game_menu_config_preview(game_menu_config_draft, gm_active_bg_screen);
	}
	if (gm_test_window && gm_test_window->is_visible() && gm_test_menu) {
		gm_test_menu->apply_game_menu_config_preview(game_menu_config_draft);
	}
}

void ClockGearsDatabaseDialog::_refresh_game_menu_editor_ui() {
	if (!game_menu_editor_panel) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;
	game_menu_config_draft = ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft);

	_rebuild_game_menu_entries_rows();

	Dictionary bgs = game_menu_config_draft.get("backgrounds", Dictionary());
	Dictionary bg = bgs.has(gm_active_bg_screen) ? Dictionary(bgs[gm_active_bg_screen]) : Dictionary();
	if (gm_bg_type_opt) {
		gm_bg_type_opt->select(String(bg.get("type", "color")) == "image" ? 1 : 0);
	}
	if (gm_bg_color_btn) {
		String col_str = String(bg.get("color", "#0e111a"));
		gm_bg_color_btn->set_pick_color(Color::html_is_valid(col_str) ? Color::html(col_str) : Color::html("#0e111a"));
	}
	if (gm_bg_image_edit) {
		gm_bg_image_edit->set_text(String(bg.get("image_path", "")));
	}
	if (gm_bg_scale_opt) {
		String sc = String(bg.get("scale_mode", "fit"));
		gm_bg_scale_opt->select(sc == "fill" ? 1 : (sc == "stretch" ? 2 : 0));
	}
	if (gm_bg_filter_opt) {
		gm_bg_filter_opt->select(String(bg.get("filter", "nearest")) == "linear" ? 1 : 0);
	}

	Dictionary prev_cfg = game_menu_config_draft.get("item_preview", Dictionary());
	if (gm_item_prev_enabled_chk) {
		gm_item_prev_enabled_chk->set_pressed(bool(prev_cfg.get("enabled", true)));
	}
	if (gm_item_prev_pos_opt) {
		gm_item_prev_pos_opt->select(String(prev_cfg.get("position", "right")) == "left" ? 1 : 0);
	}
	if (gm_item_prev_show_icon_chk) {
		gm_item_prev_show_icon_chk->set_pressed(bool(prev_cfg.get("show_icon", true)));
	}
	if (gm_item_prev_show_name_chk) {
		gm_item_prev_show_name_chk->set_pressed(bool(prev_cfg.get("show_name", true)));
	}
	if (gm_item_prev_show_desc_chk) {
		gm_item_prev_show_desc_chk->set_pressed(bool(prev_cfg.get("show_desc", true)));
	}
	if (gm_item_prev_scale_opt) {
		int sc = CLAMP(int(prev_cfg.get("icon_scale", 2)), 2, 4);
		int idx = 0;
		if (sc == 3) idx = 1;
		else if (sc == 4) idx = 2;
		gm_item_prev_scale_opt->select(idx);
	}
	if (gm_item_prev_rot_chk) {
		gm_item_prev_rot_chk->set_pressed(bool(prev_cfg.get("rotation_enabled", true)));
	}
	if (gm_item_prev_step_spin) {
		gm_item_prev_step_spin->set_value(CLAMP(int(prev_cfg.get("rotation_step", 15)), 5, 45));
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array styles = dm ? dm->get_window_styles() : Array();
	auto populate_styles_opt = [&](OptionButton *opt, const String &cur_style) {
		if (!opt) return;
		opt->clear();
		opt->add_item(CG_DB_TR("(Por defecto del tipo)"), 0);
		int sel_idx = 0;
		for (int s = 0; s < styles.size(); s++) {
			if (styles[s].get_type() != Variant::DICTIONARY) continue;
			Dictionary sd = styles[s];
			String sname = String(sd.get("name", ""));
			opt->add_item(sname, s + 1);
			if (sname == cur_style) {
				sel_idx = s + 1;
			}
		}
		opt->select(sel_idx);
	};
	if (gm_item_prev_style_opt) {
		populate_styles_opt(gm_item_prev_style_opt, String(prev_cfg.get("preview_window_style", "")));
	}
	if (gm_item_desc_style_opt) {
		populate_styles_opt(gm_item_desc_style_opt, String(prev_cfg.get("desc_window_style", "")));
	}

	_update_game_menu_live_preview();
	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_on_gm_entry_toggled(bool p_enabled, String p_entry_id) {
	if (updating_ui || p_entry_id == "magic") {
		return;
	}
	Array entries = game_menu_config_draft.get("entries", Array());
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_entry_id) {
			e["enabled"] = p_enabled;
			entries[i] = e;
			break;
		}
	}
	game_menu_config_draft["entries"] = entries;
	_update_game_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_gm_entry_text_changed(const String &p_text, String p_entry_id) {
	if (updating_ui || p_entry_id == "magic") {
		return;
	}
	Array entries = game_menu_config_draft.get("entries", Array());
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_entry_id) {
			e["text"] = p_text;
			entries[i] = e;
			break;
		}
	}
	game_menu_config_draft["entries"] = entries;
	_update_game_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_gm_entry_move_pressed(String p_entry_id, int p_delta) {
	if (move_game_menu_entry(p_entry_id, p_delta)) {
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_gm_bg_screen_tab_changed(int p_tab_idx) {
	gm_active_bg_screen = String(_gm_bg_screen_key(p_tab_idx));
	_refresh_game_menu_editor_ui();
}

void ClockGearsDatabaseDialog::_on_gm_bg_field_changed(int p_unused) {
	if (updating_ui) {
		return;
	}
	Dictionary bgs = game_menu_config_draft.get("backgrounds", Dictionary());
	Dictionary bg = bgs.has(gm_active_bg_screen) ? Dictionary(bgs[gm_active_bg_screen]) : Dictionary();
	if (gm_bg_type_opt) {
		bg["type"] = (gm_bg_type_opt->get_selected() == 1) ? "image" : "color";
	}
	if (gm_bg_color_btn) {
		bg["color"] = "#" + gm_bg_color_btn->get_pick_color().to_html(false);
	}
	if (gm_bg_image_edit) {
		bg["image_path"] = gm_bg_image_edit->get_text().strip_edges();
	}
	if (gm_bg_scale_opt) {
		int sc = gm_bg_scale_opt->get_selected();
		bg["scale_mode"] = (sc == 1) ? "fill" : ((sc == 2) ? "stretch" : "fit");
	}
	if (gm_bg_filter_opt) {
		bg["filter"] = (gm_bg_filter_opt->get_selected() == 1) ? "linear" : "nearest";
	}
	bgs[gm_active_bg_screen] = bg;
	game_menu_config_draft["backgrounds"] = bgs;
	_update_game_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_gm_bg_color_changed(const Color &p_col) {
	_on_gm_bg_field_changed();
}

void ClockGearsDatabaseDialog::_on_gm_bg_image_text_changed(const String &p_text) {
	_on_gm_bg_field_changed();
}

void ClockGearsDatabaseDialog::_on_gm_import_bg_pressed() {
	if (gm_asset_file_dialog) {
		gm_asset_file_dialog->popup_centered_ratio(0.60f);
	}
}

void ClockGearsDatabaseDialog::_on_gm_clear_bg_pressed() {
	Dictionary bgs = game_menu_config_draft.get("backgrounds", Dictionary());
	Dictionary bg = bgs.has(gm_active_bg_screen) ? Dictionary(bgs[gm_active_bg_screen]) : Dictionary();
	bg["image_path"] = "";
	if (bg.get("type", "color") == "image") {
		bg["type"] = "color";
	}
	bgs[gm_active_bg_screen] = bg;
	game_menu_config_draft["backgrounds"] = bgs;
	if (gm_bg_image_edit) gm_bg_image_edit->set_text("");
	if (gm_bg_type_opt) gm_bg_type_opt->select(0);
	_update_game_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_gm_bg_file_selected(const String &p_path) {
	bool pixel = gm_bg_filter_opt ? (gm_bg_filter_opt->get_selected() == 0) : true;
	String final_path = _import_menu_asset_file(p_path, "img/menus", pixel);
	Dictionary bgs = game_menu_config_draft.get("backgrounds", Dictionary());
	Dictionary bg = bgs.has(gm_active_bg_screen) ? Dictionary(bgs[gm_active_bg_screen]) : Dictionary();
	bg["type"] = "image";
	bg["image_path"] = final_path;
	bgs[gm_active_bg_screen] = bg;
	game_menu_config_draft["backgrounds"] = bgs;
	if (gm_bg_type_opt) gm_bg_type_opt->select(1);
	if (gm_bg_image_edit) gm_bg_image_edit->set_text(final_path);
	_update_game_menu_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_gm_test_popup_close_requested() {
	if (gm_test_window) {
		gm_test_window->hide();
	}
}

void ClockGearsDatabaseDialog::_on_gm_item_prev_field_changed(int p_unused) {
	if (updating_ui) {
		return;
	}
	Dictionary prev_cfg = game_menu_config_draft.get("item_preview", Dictionary());
	if (gm_item_prev_enabled_chk) {
		prev_cfg["enabled"] = gm_item_prev_enabled_chk->is_pressed();
	}
	if (gm_item_prev_pos_opt) {
		prev_cfg["position"] = (gm_item_prev_pos_opt->get_selected() == 1) ? "left" : "right";
	}
	if (gm_item_prev_show_icon_chk) {
		prev_cfg["show_icon"] = gm_item_prev_show_icon_chk->is_pressed();
	}
	if (gm_item_prev_show_name_chk) {
		prev_cfg["show_name"] = gm_item_prev_show_name_chk->is_pressed();
	}
	if (gm_item_prev_show_desc_chk) {
		prev_cfg["show_desc"] = gm_item_prev_show_desc_chk->is_pressed();
	}
	if (gm_item_prev_scale_opt) {
		int sel = gm_item_prev_scale_opt->get_selected();
		int sc = (sel == 1) ? 3 : ((sel == 2) ? 4 : 2);
		prev_cfg["icon_scale"] = sc;
	}
	if (gm_item_prev_rot_chk) {
		prev_cfg["rotation_enabled"] = gm_item_prev_rot_chk->is_pressed();
	}
	if (gm_item_prev_step_spin) {
		prev_cfg["rotation_step"] = int(gm_item_prev_step_spin->get_value());
	}
	if (gm_item_prev_style_opt) {
		int idx = gm_item_prev_style_opt->get_selected();
		prev_cfg["preview_window_style"] = (idx > 0) ? gm_item_prev_style_opt->get_item_text(idx) : String("");
	}
	if (gm_item_desc_style_opt) {
		int idx = gm_item_desc_style_opt->get_selected();
		prev_cfg["desc_window_style"] = (idx > 0) ? gm_item_desc_style_opt->get_item_text(idx) : String("");
	}
	game_menu_config_draft["item_preview"] = prev_cfg;
	_update_game_menu_live_preview();
	_mark_database_dirty();
}

Dictionary ClockGearsDatabaseDialog::get_game_menu_draft_copy() const {
	return ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft);
}

void ClockGearsDatabaseDialog::set_game_menu_entry_state(const String &p_id, bool p_enabled, const String &p_text) {
	if (p_id == "magic") {
		return;
	}
	Array entries = game_menu_config_draft.get("entries", Array());
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = entries[i];
		if (String(e.get("id", "")) == p_id) {
			e["enabled"] = p_enabled;
			if (!p_text.is_empty()) {
				e["text"] = p_text;
			}
			entries[i] = e;
			break;
		}
	}
	game_menu_config_draft["entries"] = entries;
	_refresh_game_menu_editor_ui();
	_mark_database_dirty();
}

bool ClockGearsDatabaseDialog::move_game_menu_entry(const String &p_id, int p_delta) {
	if (p_delta == 0 || p_id == "magic") {
		return false;
	}
	game_menu_config_draft = ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft);
	Array entries = game_menu_config_draft.get("entries", Array());
	Vector<Dictionary> visible_rows;
	Vector<Dictionary> hidden_rows;
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() == Variant::DICTIONARY) {
			Dictionary e = entries[i];
			if (String(e.get("id", "")) == "magic" || bool(e.get("reserved", false))) {
				hidden_rows.push_back(e);
			} else {
				visible_rows.push_back(e);
			}
		}
	}
	struct Cmp {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("order", 0)) < int(b.get("order", 0));
		}
	};
	visible_rows.sort_custom<Cmp>();

	int idx = -1;
	for (int i = 0; i < visible_rows.size(); i++) {
		if (String(visible_rows[i].get("id", "")) == p_id) {
			idx = i;
			break;
		}
	}
	int target = idx + p_delta;
	if (idx < 0 || target < 0 || target >= visible_rows.size()) {
		return false;
	}
	Dictionary tmp = visible_rows[idx];
	visible_rows.set(idx, visible_rows[target]);
	visible_rows.set(target, tmp);

	Array updated;
	for (int i = 0; i < visible_rows.size(); i++) {
		Dictionary d = visible_rows[i];
		d["order"] = i;
		updated.push_back(d);
	}
	for (int i = 0; i < hidden_rows.size(); i++) {
		Dictionary d = hidden_rows[i];
		d["order"] = visible_rows.size() + i;
		updated.push_back(d);
	}
	game_menu_config_draft["entries"] = updated;
	_refresh_game_menu_editor_ui();
	_mark_database_dirty();
	return true;
}

void ClockGearsDatabaseDialog::select_game_menu_preview_screen(const String &p_screen_id) {
	String sid = p_screen_id.strip_edges().to_lower();
	int tab_idx = 0;
	if (sid == "items") {
		tab_idx = 1;
	} else if (sid == "equip") {
		tab_idx = 2;
	} else if (sid == "status") {
		tab_idx = 3;
	} else {
		sid = "menu";
		tab_idx = 0;
	}
	gm_active_bg_screen = sid;
	if (gm_bg_screen_tabs && gm_bg_screen_tabs->get_current_tab() != tab_idx) {
		gm_bg_screen_tabs->set_current_tab(tab_idx);
	}
	_refresh_game_menu_editor_ui();
}

void ClockGearsDatabaseDialog::set_game_menu_screen_background(const String &p_screen_id, const String &p_type, const String &p_color_or_path, const String &p_scale_mode, const String &p_filter) {
	String sid = p_screen_id.strip_edges().to_lower();
	if (sid != "items" && sid != "equip" && sid != "status") {
		sid = "menu";
	}
	gm_active_bg_screen = sid;
	Dictionary bgs = game_menu_config_draft.get("backgrounds", Dictionary());
	Dictionary bg = bgs.has(sid) ? Dictionary(bgs[sid]) : Dictionary();
	String tp = (p_type == "image") ? "image" : "color";
	bg["type"] = tp;
	if (tp == "image") {
		bg["image_path"] = p_color_or_path.strip_edges();
	} else {
		bg["color"] = Color::html_is_valid(p_color_or_path) ? ("#" + Color::html(p_color_or_path).to_html(false)) : "#0e111a";
	}
	if (!p_scale_mode.is_empty()) {
		bg["scale_mode"] = p_scale_mode;
	}
	if (!p_filter.is_empty()) {
		bg["filter"] = p_filter;
	}
	bgs[sid] = bg;
	game_menu_config_draft["backgrounds"] = bgs;
	select_game_menu_preview_screen(sid);
	_mark_database_dirty();
}

Window *ClockGearsDatabaseDialog::open_game_menu_test_popup() {
	_ensure_gm_test_window();
	if (!gm_test_window || !gm_test_menu) {
		return nullptr;
	}
	gm_test_menu->set_editor_test_popup_mode(true);
	gm_test_menu->apply_game_menu_config_preview(ClockGearsDataManager::normalize_game_menu_config(game_menu_config_draft), "menu");
	gm_test_menu->open_menu();
	Size2i target(960, 540);
	if (DisplayServer::get_singleton()) {
		int scr = DisplayServer::get_singleton()->window_get_current_screen();
		Size2i usable = DisplayServer::get_singleton()->screen_get_usable_rect(scr).size;
		if (usable.x > 0 && usable.y > 0) {
			target = target.min(usable);
		}
	}
	gm_test_window->popup_centered(target);
	return gm_test_window;
}

Dictionary ClockGearsDatabaseDialog::get_game_menu_editor_ui_handles() {
	_setup_game_menu_editor_ui();
	Dictionary out;
	out["panel"] = game_menu_editor_panel;
	out["main_split"] = gm_main_split;
	out["entries_vbox"] = gm_entries_list_vbox;
	out["bg_screen_tabs"] = gm_bg_screen_tabs;
	out["bg_type_opt"] = gm_bg_type_opt;
	out["bg_color_btn"] = gm_bg_color_btn;
	out["bg_image_edit"] = gm_bg_image_edit;
	out["bg_scale_opt"] = gm_bg_scale_opt;
	out["bg_filter_opt"] = gm_bg_filter_opt;
	out["test_btn"] = gm_test_btn;
	out["live_preview"] = gm_live_preview;
	out["test_window"] = gm_test_window;
	out["test_menu"] = gm_test_menu;
	out["active_bg_screen"] = gm_active_bg_screen;
	return out;
}

// ============================================================================
// Menús -> Créditos (9B Tanda 4 UI)
// ============================================================================

static const char *s_cred_style_keys[4] = { "section", "role", "name", "text" };
static const char *s_cred_style_labels[4] = { "Sección", "Rol", "Nombre", "Texto" };

static String s_cred_item_summary(int p_index, const Dictionary &p_item) {
	String itype = String(p_item.get("type", "text"));
	if (itype == "section") {
		String txt = String(p_item.get("text", "")).strip_edges();
		return vformat(CG_DB_TR("%d. [Sección] %s"), p_index + 1, txt.is_empty() ? CG_DB_TR("(sin título)") : txt);
	}
	if (itype == "role_name") {
		String r = String(p_item.get("role", "")).strip_edges();
		String n = String(p_item.get("name", "")).strip_edges();
		return vformat(CG_DB_TR("%d. [Rol/Nombre] %s — %s"), p_index + 1, r.is_empty() ? CG_DB_TR("(rol)") : r, n.is_empty() ? CG_DB_TR("(nombre)") : n);
	}
	if (itype == "image") {
		String p = String(p_item.get("image_path", "")).get_file();
		return vformat("%d. [Imagen] %s", p_index + 1, p.is_empty() ? CG_DB_TR("(sin imagen)") : p);
	}
	if (itype == "spacer") {
		return vformat("%d. [Espacio] %d px", p_index + 1, int(p_item.get("height", 32.0)));
	}
	String txt = String(p_item.get("text", "")).strip_edges().replace("\n", " ");
	txt = ClockGearsDataManager::truncate_with_ellipsis(txt, 42);
	return vformat("%d. [Texto] %s", p_index + 1, txt.is_empty() ? CG_DB_TR("(vacío)") : txt);
}

void ClockGearsDatabaseDialog::_setup_credits_editor_ui() {
	if (credits_editor_panel || !col2_box) {
		return;
	}
	const float row_h = 30.0f * EDSCALE;
	credits_config_draft = ClockGearsDataManager::get_default_credits_config();
	if (ClockGearsDataManager::get_singleton()) {
		credits_config_draft = ClockGearsDataManager::normalize_credits_config(ClockGearsDataManager::get_singleton()->get_credits_config());
	}

	credits_editor_panel = memnew(VBoxContainer);
	credits_editor_panel->set_name("CreditsEditorPanel");
	credits_editor_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	credits_editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	credits_editor_panel->add_theme_constant_override("separation", int(12 * EDSCALE));
	credits_editor_panel->hide();
	col2_box->add_child(credits_editor_panel);

	cred_main_split = memnew(BoxContainer);
	cred_main_split->set_vertical(false);
	cred_main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	cred_main_split->add_theme_constant_override("separation", int(14 * EDSCALE));
	credits_editor_panel->add_child(cred_main_split);

	cred_left_col = memnew(VBoxContainer);
	cred_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_left_col->set_stretch_ratio(1.15f);
	cred_left_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	cred_main_split->add_child(cred_left_col);

	auto make_card = [&](const String &p_title, VBoxContainer *&r_body) {
		_make_collapsible_menu_card(cred_left_col, p_title, r_body, true);
	};

	auto add_grid_label = [&](GridContainer *p_grid, const String &p_txt) {
		Label *lbl = memnew(Label(p_txt));
		lbl->set_custom_minimum_size(Size2(104 * EDSCALE, 0));
		p_grid->add_child(lbl);
	};

	// 1. Secciones y líneas
	VBoxContainer *items_body = nullptr;
	make_card(CG_DB_TR("Secciones y líneas"), items_body);

	HFlowContainer *toolbar = memnew(HFlowContainer);
	toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	toolbar->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	toolbar->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	items_body->add_child(toolbar);

	cred_add_type_opt = memnew(OptionButton);
	cred_add_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_add_type_opt->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	cred_add_type_opt->add_item(CG_DB_TR("Sección"), 0);
	cred_add_type_opt->set_item_metadata(0, String("section"));
	cred_add_type_opt->add_item(CG_DB_TR("Rol y nombre"), 1);
	cred_add_type_opt->set_item_metadata(1, String("role_name"));
	cred_add_type_opt->add_item(CG_DB_TR("Texto"), 2);
	cred_add_type_opt->set_item_metadata(2, String("text"));
	cred_add_type_opt->add_item(CG_DB_TR("Imagen"), 3);
	cred_add_type_opt->set_item_metadata(3, String("image"));
	cred_add_type_opt->add_item(CG_DB_TR("Espacio"), 4);
	cred_add_type_opt->set_item_metadata(4, String("spacer"));
	toolbar->add_child(cred_add_type_opt);

	cred_add_btn = memnew(Button("+"));
	cred_add_btn->set_tooltip_text(CG_DB_TR("Añadir elemento"));
	cred_add_btn->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	cred_add_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_add_pressed));
	toolbar->add_child(cred_add_btn);

	cred_del_btn = memnew(Button("-"));
	cred_del_btn->set_tooltip_text(CG_DB_TR("Eliminar elemento"));
	cred_del_btn->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	cred_del_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_remove_pressed));
	toolbar->add_child(cred_del_btn);

	cred_up_btn = memnew(Button(String::utf8("↑")));
	cred_up_btn->set_tooltip_text(CG_DB_TR("Subir elemento"));
	cred_up_btn->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	cred_up_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_move_pressed).bind(-1));
	toolbar->add_child(cred_up_btn);

	cred_down_btn = memnew(Button(String::utf8("↓")));
	cred_down_btn->set_tooltip_text(CG_DB_TR("Bajar elemento"));
	cred_down_btn->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	cred_down_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_move_pressed).bind(1));
	toolbar->add_child(cred_down_btn);

	cred_dup_btn = memnew(Button(CG_DB_TR("Duplicar")));
	cred_dup_btn->set_custom_minimum_size(Size2(74 * EDSCALE, row_h));
	cred_dup_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_duplicate_pressed));
	toolbar->add_child(cred_dup_btn);

	cred_items_list = memnew(ItemList);
	cred_items_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_items_list->set_custom_minimum_size(Size2(0, 132 * EDSCALE));
	cred_items_list->set_allow_reselect(true);
	cred_items_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_selected));
	items_body->add_child(cred_items_list);

	// Inspector del elemento seleccionado
	cred_item_props_box = memnew(VBoxContainer);
	cred_item_props_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_props_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	items_body->add_child(cred_item_props_box);

	cred_item_empty_lbl = memnew(Label(CG_DB_TR("Selecciona o añade un elemento para editar su contenido.")));
	cred_item_empty_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	cred_item_empty_lbl->add_theme_color_override("font_color", Color(0.70f, 0.76f, 0.86f, 0.85f));
	cred_item_props_box->add_child(cred_item_empty_lbl);

	// Sección (una línea)
	cred_item_text_row = memnew(HBoxContainer);
	cred_item_text_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	cred_item_text_row->hide();
	cred_item_props_box->add_child(cred_item_text_row);
	Label *sec_lbl = memnew(Label(CG_DB_TR("Título:")));
	sec_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	cred_item_text_row->add_child(sec_lbl);
	cred_item_text_edit = memnew(LineEdit);
	cred_item_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_text_edit->set_custom_minimum_size(Size2(0, row_h));
	cred_item_text_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_text_changed));
	cred_item_text_row->add_child(cred_item_text_edit);

	// Texto libre (multilínea)
	cred_item_multiline_row = memnew(HBoxContainer);
	cred_item_multiline_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	cred_item_multiline_row->hide();
	cred_item_props_box->add_child(cred_item_multiline_row);
	Label *txt_lbl = memnew(Label(CG_DB_TR("Texto:")));
	txt_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	cred_item_multiline_row->add_child(txt_lbl);
	cred_item_multiline_edit = memnew(TextEdit);
	cred_item_multiline_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_multiline_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
	cred_item_multiline_edit->set_custom_minimum_size(Size2(0, 58 * EDSCALE));
	cred_item_multiline_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_multiline_changed));
	cred_item_multiline_row->add_child(cred_item_multiline_edit);

	// Rol y Nombre
	cred_item_role_row = memnew(HBoxContainer);
	cred_item_role_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	cred_item_role_row->hide();
	cred_item_props_box->add_child(cred_item_role_row);
	Label *role_lbl = memnew(Label(CG_DB_TR("Rol:")));
	role_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	cred_item_role_row->add_child(role_lbl);
	cred_item_role_edit = memnew(LineEdit);
	cred_item_role_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_role_edit->set_custom_minimum_size(Size2(0, row_h));
	cred_item_role_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_role_or_name_changed));
	cred_item_role_row->add_child(cred_item_role_edit);

	cred_item_name_row = memnew(HBoxContainer);
	cred_item_name_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	cred_item_name_row->hide();
	cred_item_props_box->add_child(cred_item_name_row);
	Label *name_lbl = memnew(Label(CG_DB_TR("Nombre:")));
	name_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	cred_item_name_row->add_child(name_lbl);
	cred_item_name_edit = memnew(LineEdit);
	cred_item_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_name_edit->set_custom_minimum_size(Size2(0, row_h));
	cred_item_name_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_role_or_name_changed));
	cred_item_name_row->add_child(cred_item_name_edit);

	// Imagen
	cred_item_img_box = memnew(VBoxContainer);
	cred_item_img_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	cred_item_img_box->hide();
	cred_item_props_box->add_child(cred_item_img_box);

	HBoxContainer *img_path_row = memnew(HBoxContainer);
	img_path_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	cred_item_img_box->add_child(img_path_row);
	Label *img_lbl = memnew(Label(CG_DB_TR("Imagen:")));
	img_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	img_path_row->add_child(img_lbl);
	cred_item_img_edit = memnew(LineEdit);
	cred_item_img_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_img_edit->set_custom_minimum_size(Size2(0, row_h));
	cred_item_img_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_img_changed));
	img_path_row->add_child(cred_item_img_edit);
	cred_item_img_import_btn = memnew(Button(CG_DB_TR("Importar")));
	cred_item_img_import_btn->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
	cred_item_img_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_pick_asset_pressed).bind(1));
	img_path_row->add_child(cred_item_img_import_btn);
	cred_item_img_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	cred_item_img_clear_btn->set_custom_minimum_size(Size2(66 * EDSCALE, row_h));
	cred_item_img_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_clear_asset_pressed).bind(1));
	img_path_row->add_child(cred_item_img_clear_btn);

	HBoxContainer *img_opts_row = memnew(HBoxContainer);
	img_opts_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	cred_item_img_box->add_child(img_opts_row);
	Label *iflt_lbl = memnew(Label(CG_DB_TR("Filtro:")));
	iflt_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	img_opts_row->add_child(iflt_lbl);
	cred_item_img_filter_opt = memnew(OptionButton);
	cred_item_img_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_img_filter_opt->add_item("Nearest", 0);
	cred_item_img_filter_opt->add_item("Linear", 1);
	cred_item_img_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_img_opt_or_spin_changed));
	img_opts_row->add_child(cred_item_img_filter_opt);
	img_opts_row->add_child(memnew(Label(CG_DB_TR("Máx:"))));
	cred_item_img_w_spin = memnew(SpinBox);
	cred_item_img_w_spin->set_min(16);
	cred_item_img_w_spin->set_max(960);
	cred_item_img_w_spin->set_step(8);
	cred_item_img_w_spin->set_custom_minimum_size(Size2(74 * EDSCALE, row_h));
	cred_item_img_w_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_img_opt_or_spin_changed));
	img_opts_row->add_child(cred_item_img_w_spin);
	cred_item_img_h_spin = memnew(SpinBox);
	cred_item_img_h_spin->set_min(16);
	cred_item_img_h_spin->set_max(540);
	cred_item_img_h_spin->set_step(8);
	cred_item_img_h_spin->set_custom_minimum_size(Size2(74 * EDSCALE, row_h));
	cred_item_img_h_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_img_opt_or_spin_changed));
	img_opts_row->add_child(cred_item_img_h_spin);

	// Espacio
	cred_item_spacer_row = memnew(HBoxContainer);
	cred_item_spacer_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	cred_item_spacer_row->hide();
	cred_item_props_box->add_child(cred_item_spacer_row);
	Label *sp_lbl = memnew(Label(CG_DB_TR("Altura:")));
	sp_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	cred_item_spacer_row->add_child(sp_lbl);
	cred_item_spacer_spin = memnew(SpinBox);
	cred_item_spacer_spin->set_min(4);
	cred_item_spacer_spin->set_max(480);
	cred_item_spacer_spin->set_step(2);
	cred_item_spacer_spin->set_suffix(" px");
	cred_item_spacer_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_spacer_spin->set_custom_minimum_size(Size2(0, row_h));
	cred_item_spacer_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_spacer_changed));
	cred_item_spacer_row->add_child(cred_item_spacer_spin);

	// Alineación propia del elemento (opcional)
	cred_item_align_row = memnew(HBoxContainer);
	cred_item_align_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	cred_item_align_row->hide();
	cred_item_props_box->add_child(cred_item_align_row);
	Label *ial_lbl = memnew(Label(CG_DB_TR("Alineación:")));
	ial_lbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
	cred_item_align_row->add_child(ial_lbl);
	cred_item_align_opt = memnew(OptionButton);
	cred_item_align_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_item_align_opt->set_custom_minimum_size(Size2(0, row_h));
	cred_item_align_opt->add_item(CG_DB_TR("(Usar global)"), 0);
	cred_item_align_opt->add_item(CG_DB_TR("Izquierda"), 1);
	cred_item_align_opt->add_item(CG_DB_TR("Centro"), 2);
	cred_item_align_opt->add_item(CG_DB_TR("Derecha"), 3);
	cred_item_align_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_item_align_selected));
	cred_item_align_row->add_child(cred_item_align_opt);

	// 2. Fondo, Música, Velocidad y Alineación Global
	VBoxContainer *gen_body = nullptr;
	make_card(CG_DB_TR("Fondo, música y desplazamiento"), gen_body);

	GridContainer *gen_grid = memnew(GridContainer);
	gen_grid->set_columns(2);
	gen_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gen_grid->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	gen_grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	gen_body->add_child(gen_grid);

	add_grid_label(gen_grid, CG_DB_TR("Tipo de fondo:"));
	cred_bg_type_opt = memnew(OptionButton);
	cred_bg_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_bg_type_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	cred_bg_type_opt->add_item(CG_DB_TR("Color sólido"), 0);
	cred_bg_type_opt->add_item(CG_DB_TR("Imagen"), 1);
	cred_bg_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bg_or_align_changed));
	gen_grid->add_child(cred_bg_type_opt);

	add_grid_label(gen_grid, CG_DB_TR("Color de fondo:"));
	cred_bg_color_btn = memnew(ColorPickerButton);
	cred_bg_color_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_bg_color_btn->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	cred_bg_color_btn->set_edit_alpha(false);
	cred_bg_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bg_color_changed));
	gen_grid->add_child(cred_bg_color_btn);

	add_grid_label(gen_grid, CG_DB_TR("Imagen de fondo:"));
	HFlowContainer *bg_img_row = memnew(HFlowContainer);
	bg_img_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_img_row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	bg_img_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	gen_grid->add_child(bg_img_row);

	cred_bg_image_edit = memnew(LineEdit);
	cred_bg_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_bg_image_edit->set_custom_minimum_size(Size2(70 * EDSCALE, row_h));
	cred_bg_image_edit->set_placeholder("res://img/...");
	cred_bg_image_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bg_image_text_changed));
	bg_img_row->add_child(cred_bg_image_edit);

	cred_bg_import_btn = memnew(Button(CG_DB_TR("Importar")));
	cred_bg_import_btn->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	cred_bg_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_pick_asset_pressed).bind(0));
	bg_img_row->add_child(cred_bg_import_btn);

	cred_bg_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	cred_bg_clear_btn->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
	cred_bg_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_clear_asset_pressed).bind(0));
	bg_img_row->add_child(cred_bg_clear_btn);

	add_grid_label(gen_grid, CG_DB_TR("Modo de imagen:"));
	cred_bg_scale_opt = memnew(OptionButton);
	cred_bg_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_bg_scale_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	cred_bg_scale_opt->add_item(CG_DB_TR("Ajustar (Fit)"), 0);
	cred_bg_scale_opt->add_item(CG_DB_TR("Rellenar (Fill)"), 1);
	cred_bg_scale_opt->add_item(CG_DB_TR("Estirar (Stretch)"), 2);
	cred_bg_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bg_or_align_changed));
	gen_grid->add_child(cred_bg_scale_opt);

	add_grid_label(gen_grid, CG_DB_TR("Filtro de fondo:"));
	cred_bg_filter_opt = memnew(OptionButton);
	cred_bg_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_bg_filter_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	cred_bg_filter_opt->add_item(CG_DB_TR("Nearest (Pixel Art)"), 0);
	cred_bg_filter_opt->add_item(CG_DB_TR("Linear (Suavizado)"), 1);
	cred_bg_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bg_or_align_changed));
	gen_grid->add_child(cred_bg_filter_opt);

	add_grid_label(gen_grid, CG_DB_TR("Música opcional:"));
	HFlowContainer *bgm_row = memnew(HFlowContainer);
	bgm_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bgm_row->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	bgm_row->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	gen_grid->add_child(bgm_row);

	cred_bgm_edit = memnew(LineEdit);
	cred_bgm_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_bgm_edit->set_custom_minimum_size(Size2(70 * EDSCALE, row_h));
	cred_bgm_edit->set_placeholder("res://audio/...");
	cred_bgm_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bgm_text_changed));
	bgm_row->add_child(cred_bgm_edit);

	cred_bgm_import_btn = memnew(Button(CG_DB_TR("Importar")));
	cred_bgm_import_btn->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	cred_bgm_import_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_pick_asset_pressed).bind(2));
	bgm_row->add_child(cred_bgm_import_btn);

	cred_bgm_clear_btn = memnew(Button(CG_DB_TR("Quitar")));
	cred_bgm_clear_btn->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
	cred_bgm_clear_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_clear_asset_pressed).bind(2));
	bgm_row->add_child(cred_bgm_clear_btn);

	add_grid_label(gen_grid, CG_DB_TR("Velocidad:"));
	HBoxContainer *spd_row = memnew(HBoxContainer);
	spd_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	spd_row->add_theme_constant_override("separation", int(8 * EDSCALE));
	gen_grid->add_child(spd_row);

	cred_speed_slider = memnew(HSlider);
	cred_speed_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_speed_slider->set_min(5.0);
	cred_speed_slider->set_max(600.0);
	cred_speed_slider->set_step(1.0);
	cred_speed_slider->set_value(60.0);
	cred_speed_slider->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_speed_slider_changed));
	spd_row->add_child(cred_speed_slider);

	cred_speed_spin = memnew(SpinBox);
	cred_speed_spin->set_min(5.0);
	cred_speed_spin->set_max(600.0);
	cred_speed_spin->set_step(1.0);
	cred_speed_spin->set_value(60.0);
	cred_speed_spin->set_suffix(" px/s");
	cred_speed_spin->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	cred_speed_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_speed_spin_changed));
	spd_row->add_child(cred_speed_spin);

	add_grid_label(gen_grid, CG_DB_TR("Alineación global:"));
	cred_align_opt = memnew(OptionButton);
	cred_align_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_align_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	cred_align_opt->add_item(CG_DB_TR("Izquierda"), 0);
	cred_align_opt->add_item(CG_DB_TR("Centro"), 1);
	cred_align_opt->add_item(CG_DB_TR("Derecha"), 2);
	cred_align_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_bg_or_align_changed));
	gen_grid->add_child(cred_align_opt);

	// 3. Estilos de texto por tipo de línea
	VBoxContainer *st_body = nullptr;
	make_card(CG_DB_TR("Estilo de texto por tipo de línea"), st_body);

	GridContainer *st_grid = memnew(GridContainer);
	st_grid->set_columns(3);
	st_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	st_grid->add_theme_constant_override("h_separation", int(10 * EDSCALE));
	st_grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	st_body->add_child(st_grid);

	for (int i = 0; i < 4; i++) {
		Label *slbl = memnew(Label(CG_DB_TR(s_cred_style_labels[i])));
		slbl->set_custom_minimum_size(Size2(96 * EDSCALE, 0));
		st_grid->add_child(slbl);

		cred_style_size_spin[i] = memnew(SpinBox);
		cred_style_size_spin[i]->set_min(8);
		cred_style_size_spin[i]->set_max(96);
		cred_style_size_spin[i]->set_step(1);
		cred_style_size_spin[i]->set_suffix(" px");
		cred_style_size_spin[i]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		cred_style_size_spin[i]->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
		cred_style_size_spin[i]->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_style_size_changed).bind(i));
		st_grid->add_child(cred_style_size_spin[i]);

		cred_style_color_btn[i] = memnew(ColorPickerButton);
		cred_style_color_btn[i]->set_custom_minimum_size(Size2(72 * EDSCALE, row_h));
		cred_style_color_btn[i]->set_edit_alpha(false);
		cred_style_color_btn[i]->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_style_color_changed).bind(i));
		st_grid->add_child(cred_style_color_btn[i]);
	}

	// Columna derecha: Vista previa en vivo + Probar
	cred_prev_card = memnew(PanelContainer);
	cred_prev_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cred_prev_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	cred_prev_card->set_stretch_ratio(1.0f);
	cred_prev_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	cred_main_split->add_child(cred_prev_card);

	VBoxContainer *prev_vb = memnew(VBoxContainer);
	prev_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	prev_vb->add_theme_constant_override("separation", int(10 * EDSCALE));
	cred_prev_card->add_child(prev_vb);

	cred_test_btn = memnew(Button(CG_DB_TR("Probar")));
	cred_test_btn->set_custom_minimum_size(Size2(92 * EDSCALE, row_h));
	cred_test_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::open_credits_test_popup));

	_build_16_9_viewport_preview(prev_vb, cred_preview_viewport, cred_prev_title_lbl, cred_preview_res_opt, cred_test_btn);
	_sync_common_preview_res_options();

	cred_live_preview = memnew(ClockGearsCreditsPlayer);
	cred_live_preview->set_preview_mode(true);
	cred_preview_viewport->add_child(cred_live_preview);

	Label *prev_tip = memnew(Label(CG_DB_TR("Pulsa 'Probar' para ver los créditos desplazarse a velocidad real (Esc cierra, Enter o clic acelera).")));
	prev_tip->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	prev_tip->add_theme_color_override("font_color", Color(0.70f, 0.76f, 0.86f, 0.85f));
	prev_vb->add_child(prev_tip);

	cred_asset_file_dialog = memnew(FileDialog);
	cred_asset_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	cred_asset_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	cred_asset_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_asset_file_selected));
	add_child(cred_asset_file_dialog);
}

void ClockGearsDatabaseDialog::_ensure_cred_test_window() {
	if (cred_test_window && cred_test_player) {
		return;
	}
	cred_test_window = memnew(Window);
	cred_test_window->set_title(CG_DB_TR("Probar créditos (960x540)"));
	cred_test_window->set_visible(false);
	cred_test_window->set_transient(true);
	cred_test_window->set_content_scale_size(Size2i(960, 540));
	cred_test_window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
	cred_test_window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
	cred_test_window->connect("close_requested", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_test_popup_close_requested));
	add_child(cred_test_window);

	cred_test_player = memnew(ClockGearsCreditsPlayer);
	cred_test_player->connect("credits_finished", callable_mp(this, &ClockGearsDatabaseDialog::_on_cred_test_player_finished));
	cred_test_window->add_child(cred_test_player);
}

void ClockGearsDatabaseDialog::_update_credits_live_preview() {
	if (!cred_live_preview) {
		return;
	}
	cred_live_preview->apply_credits_config_preview(ClockGearsDataManager::normalize_credits_config(credits_config_draft), 42.0f);
}

void ClockGearsDatabaseDialog::_rebuild_credits_items_list() {
	if (!cred_items_list) {
		return;
	}
	cred_items_list->clear();
	Array items = credits_config_draft.get("items", Array());
	for (int i = 0; i < items.size(); i++) {
		if (items[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		cred_items_list->add_item(s_cred_item_summary(i, Dictionary(items[i])));
	}
	if (items.is_empty()) {
		cred_selected_item_idx = -1;
	} else {
		if (cred_selected_item_idx < 0) {
			cred_selected_item_idx = 0;
		} else if (cred_selected_item_idx >= items.size()) {
			cred_selected_item_idx = items.size() - 1;
		}
		if (cred_selected_item_idx >= 0 && cred_selected_item_idx < cred_items_list->get_item_count()) {
			cred_items_list->select(cred_selected_item_idx);
		}
	}
	if (cred_del_btn) cred_del_btn->set_disabled(cred_selected_item_idx < 0);
	if (cred_dup_btn) cred_dup_btn->set_disabled(cred_selected_item_idx < 0);
	if (cred_up_btn) cred_up_btn->set_disabled(cred_selected_item_idx <= 0);
	if (cred_down_btn) cred_down_btn->set_disabled(cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size() - 1);
	_refresh_credits_selected_item_props();
}

void ClockGearsDatabaseDialog::_refresh_credits_selected_item_props() {
	bool prev_updating = updating_ui;
	updating_ui = true;

	Array items = credits_config_draft.get("items", Array());
	bool has_sel = (cred_selected_item_idx >= 0 && cred_selected_item_idx < items.size() && items[cred_selected_item_idx].get_type() == Variant::DICTIONARY);

	if (cred_item_empty_lbl) cred_item_empty_lbl->set_visible(!has_sel);
	if (cred_item_text_row) cred_item_text_row->set_visible(false);
	if (cred_item_multiline_row) cred_item_multiline_row->set_visible(false);
	if (cred_item_role_row) cred_item_role_row->set_visible(false);
	if (cred_item_name_row) cred_item_name_row->set_visible(false);
	if (cred_item_img_box) cred_item_img_box->set_visible(false);
	if (cred_item_spacer_row) cred_item_spacer_row->set_visible(false);
	if (cred_item_align_row) cred_item_align_row->set_visible(false);

	if (has_sel) {
		Dictionary item = items[cred_selected_item_idx];
		String itype = String(item.get("type", "text"));
		if (itype == "section") {
			if (cred_item_text_row) cred_item_text_row->set_visible(true);
			if (cred_item_text_edit) cred_item_text_edit->set_text(String(item.get("text", "")));
		} else if (itype == "text") {
			if (cred_item_multiline_row) cred_item_multiline_row->set_visible(true);
			if (cred_item_multiline_edit) cred_item_multiline_edit->set_text(String(item.get("text", "")));
		} else if (itype == "role_name") {
			if (cred_item_role_row) cred_item_role_row->set_visible(true);
			if (cred_item_name_row) cred_item_name_row->set_visible(true);
			if (cred_item_role_edit) cred_item_role_edit->set_text(String(item.get("role", "")));
			if (cred_item_name_edit) cred_item_name_edit->set_text(String(item.get("name", "")));
		} else if (itype == "image") {
			if (cred_item_img_box) cred_item_img_box->set_visible(true);
			if (cred_item_img_edit) cred_item_img_edit->set_text(String(item.get("image_path", "")));
			if (cred_item_img_filter_opt) {
				String flt = String(item.get("filter", "nearest")).strip_edges().to_lower();
				cred_item_img_filter_opt->select(flt == "linear" ? 1 : 0);
			}
			if (cred_item_img_w_spin) cred_item_img_w_spin->set_value(float(item.get("max_width", 420.0)));
			if (cred_item_img_h_spin) cred_item_img_h_spin->set_value(float(item.get("max_height", 220.0)));
		} else if (itype == "spacer") {
			if (cred_item_spacer_row) cred_item_spacer_row->set_visible(true);
			if (cred_item_spacer_spin) cred_item_spacer_spin->set_value(float(item.get("height", 32.0)));
		}

		if (itype != "spacer" && cred_item_align_row && cred_item_align_opt) {
			cred_item_align_row->set_visible(true);
			String ial = String(item.get("align", "")).strip_edges().to_lower();
			int aidx = 0;
			if (ial == "left" || ial == "izquierda") {
				aidx = 1;
			} else if (ial == "center" || ial == "centro") {
				aidx = 2;
			} else if (ial == "right" || ial == "derecha") {
				aidx = 3;
			}
			cred_item_align_opt->select(aidx);
		}
	}

	updating_ui = prev_updating;
}

void ClockGearsDatabaseDialog::_refresh_credits_editor_ui() {
	if (!credits_editor_panel) {
		return;
	}
	bool prev_updating = updating_ui;
	updating_ui = true;

	credits_config_draft = ClockGearsDataManager::normalize_credits_config(credits_config_draft);
	Dictionary cred_bg = credits_config_draft.get("background", Dictionary());

	String bg_type = String(cred_bg.get("type", "color")).to_lower();
	if (cred_bg_type_opt) cred_bg_type_opt->select(bg_type == "image" ? 1 : 0);
	String col_s = String(cred_bg.get("color", "#090b12ff"));
	if (cred_bg_color_btn) cred_bg_color_btn->set_pick_color(Color::html_is_valid(col_s) ? Color::html(col_s) : Color::html("#090b12ff"));
	if (cred_bg_image_edit) cred_bg_image_edit->set_text(String(cred_bg.get("image_path", "")));

	String sc = String(cred_bg.get("scale_mode", "fit")).to_lower();
	if (cred_bg_scale_opt) cred_bg_scale_opt->select(sc == "fill" ? 1 : (sc == "stretch" ? 2 : 0));
	String flt = String(cred_bg.get("filter", "nearest")).to_lower();
	if (cred_bg_filter_opt) cred_bg_filter_opt->select(flt == "linear" ? 1 : 0);

	if (cred_bgm_edit) cred_bgm_edit->set_text(String(credits_config_draft.get("bgm_path", "")));
	double spd = CLAMP(double(credits_config_draft.get("scroll_speed", 60.0)), 5.0, 600.0);
	if (cred_speed_slider) cred_speed_slider->set_value(spd);
	if (cred_speed_spin) cred_speed_spin->set_value(spd);

	String gal = String(credits_config_draft.get("alignment", "center")).to_lower();
	if (cred_align_opt) cred_align_opt->select(gal == "left" ? 0 : (gal == "right" ? 2 : 1));

	Dictionary styles = credits_config_draft.get("styles", Dictionary());
	for (int i = 0; i < 4; i++) {
		Dictionary st = styles.get(s_cred_style_keys[i], Dictionary());
		if (cred_style_size_spin[i]) {
			cred_style_size_spin[i]->set_value(CLAMP(int(st.get("font_size", 16)), 8, 96));
		}
		if (cred_style_color_btn[i]) {
			String scs = String(st.get("color", "#ffffffff"));
			cred_style_color_btn[i]->set_pick_color(Color::html_is_valid(scs) ? Color::html(scs) : Color(1, 1, 1, 1));
		}
	}

	updating_ui = prev_updating;
	_rebuild_credits_items_list();
	_update_credits_live_preview();
}

int ClockGearsDatabaseDialog::add_credits_item(const String &p_type, const Dictionary &p_fields) {
	credits_config_draft = ClockGearsDataManager::normalize_credits_config(credits_config_draft);
	Array items = credits_config_draft.get("items", Array());
	Dictionary ni;
	String tp = p_type.strip_edges().to_lower();
	if (tp != "section" && tp != "role_name" && tp != "text" && tp != "image" && tp != "spacer") {
		tp = "text";
	}
	ni["type"] = tp;
	if (tp == "section") {
		ni["text"] = p_fields.has("text") ? p_fields["text"] : Variant(CG_DB_TR("Nueva sección"));
	} else if (tp == "role_name") {
		ni["role"] = p_fields.has("role") ? p_fields["role"] : Variant(CG_DB_TR("Rol"));
		ni["name"] = p_fields.has("name") ? p_fields["name"] : Variant(CG_DB_TR("Nombre"));
	} else if (tp == "text") {
		ni["text"] = p_fields.has("text") ? p_fields["text"] : Variant(CG_DB_TR("Texto de créditos"));
	} else if (tp == "image") {
		ni["image_path"] = p_fields.get("image_path", "");
		ni["filter"] = p_fields.get("filter", "nearest");
		ni["max_width"] = p_fields.get("max_width", 420.0);
		ni["max_height"] = p_fields.get("max_height", 220.0);
	} else if (tp == "spacer") {
		ni["height"] = p_fields.get("height", 32.0);
	}
	if (p_fields.has("align")) {
		ni["align"] = p_fields["align"];
	}
	int insert_idx = (cred_selected_item_idx >= 0 && cred_selected_item_idx < items.size()) ? (cred_selected_item_idx + 1) : items.size();
	items.insert(insert_idx, ni);
	credits_config_draft["items"] = items;
	credits_config_draft = ClockGearsDataManager::normalize_credits_config(credits_config_draft);
	cred_selected_item_idx = insert_idx;
	_rebuild_credits_items_list();
	_update_credits_live_preview();
	_mark_database_dirty();
	return insert_idx;
}

bool ClockGearsDatabaseDialog::remove_credits_item(int p_index) {
	Array items = credits_config_draft.get("items", Array());
	if (p_index < 0 || p_index >= items.size()) {
		return false;
	}
	items.remove_at(p_index);
	credits_config_draft["items"] = items;
	if (items.is_empty()) {
		cred_selected_item_idx = -1;
	} else {
		cred_selected_item_idx = CLAMP(p_index, 0, items.size() - 1);
	}
	_rebuild_credits_items_list();
	_update_credits_live_preview();
	_mark_database_dirty();
	return true;
}

bool ClockGearsDatabaseDialog::move_credits_item(int p_index, int p_delta) {
	Array items = credits_config_draft.get("items", Array());
	int target = p_index + p_delta;
	if (p_delta == 0 || p_index < 0 || p_index >= items.size() || target < 0 || target >= items.size()) {
		return false;
	}
	Variant tmp = items[p_index];
	items[p_index] = items[target];
	items[target] = tmp;
	credits_config_draft["items"] = items;
	cred_selected_item_idx = target;
	_rebuild_credits_items_list();
	_update_credits_live_preview();
	_mark_database_dirty();
	return true;
}

int ClockGearsDatabaseDialog::duplicate_credits_item(int p_index) {
	Array items = credits_config_draft.get("items", Array());
	if (p_index < 0 || p_index >= items.size() || items[p_index].get_type() != Variant::DICTIONARY) {
		return -1;
	}
	Dictionary copy = Dictionary(items[p_index]).duplicate(true);
	int new_idx = p_index + 1;
	items.insert(new_idx, copy);
	credits_config_draft["items"] = items;
	cred_selected_item_idx = new_idx;
	_rebuild_credits_items_list();
	_update_credits_live_preview();
	_mark_database_dirty();
	return new_idx;
}

void ClockGearsDatabaseDialog::select_credits_item(int p_index) {
	Array items = credits_config_draft.get("items", Array());
	if (p_index < 0 || p_index >= items.size()) {
		return;
	}
	cred_selected_item_idx = p_index;
	_rebuild_credits_items_list();
}

bool ClockGearsDatabaseDialog::set_credits_item_field(int p_index, const String &p_key, const Variant &p_val) {
	Array items = credits_config_draft.get("items", Array());
	if (p_index < 0 || p_index >= items.size() || items[p_index].get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary item = items[p_index];
	if (p_key == "align" && String(p_val).strip_edges().is_empty()) {
		item.erase("align");
	} else {
		item[p_key] = p_val;
	}
	items[p_index] = item;
	credits_config_draft["items"] = items;
	credits_config_draft = ClockGearsDataManager::normalize_credits_config(credits_config_draft);
	cred_selected_item_idx = p_index;
	_rebuild_credits_items_list();
	_update_credits_live_preview();
	_mark_database_dirty();
	return true;
}

void ClockGearsDatabaseDialog::_on_cred_add_pressed() {
	String tp = "section";
	if (cred_add_type_opt && cred_add_type_opt->get_selected() >= 0) {
		tp = String(cred_add_type_opt->get_selected_metadata());
	}
	add_credits_item(tp);
}

void ClockGearsDatabaseDialog::_on_cred_remove_pressed() {
	remove_credits_item(cred_selected_item_idx);
}

void ClockGearsDatabaseDialog::_on_cred_move_pressed(int p_delta) {
	move_credits_item(cred_selected_item_idx, p_delta);
}

void ClockGearsDatabaseDialog::_on_cred_duplicate_pressed() {
	duplicate_credits_item(cred_selected_item_idx);
}

void ClockGearsDatabaseDialog::_on_cred_item_selected(int p_idx) {
	if (updating_ui) return;
	cred_selected_item_idx = p_idx;
	Array items = credits_config_draft.get("items", Array());
	if (cred_del_btn) cred_del_btn->set_disabled(cred_selected_item_idx < 0);
	if (cred_dup_btn) cred_dup_btn->set_disabled(cred_selected_item_idx < 0);
	if (cred_up_btn) cred_up_btn->set_disabled(cred_selected_item_idx <= 0);
	if (cred_down_btn) cred_down_btn->set_disabled(cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size() - 1);
	_refresh_credits_selected_item_props();
}

void ClockGearsDatabaseDialog::_on_cred_item_text_changed(const String &p_text) {
	if (updating_ui) return;
	Array items = credits_config_draft.get("items", Array());
	if (cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size()) return;
	Dictionary item = items[cred_selected_item_idx];
	item["text"] = p_text;
	items[cred_selected_item_idx] = item;
	credits_config_draft["items"] = items;
	if (cred_items_list && cred_selected_item_idx < cred_items_list->get_item_count()) {
		cred_items_list->set_item_text(cred_selected_item_idx, s_cred_item_summary(cred_selected_item_idx, item));
	}
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_item_multiline_changed() {
	if (updating_ui || !cred_item_multiline_edit) return;
	_on_cred_item_text_changed(cred_item_multiline_edit->get_text());
}

void ClockGearsDatabaseDialog::_on_cred_item_role_or_name_changed(const String &p_text) {
	if (updating_ui) return;
	Array items = credits_config_draft.get("items", Array());
	if (cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size()) return;
	Dictionary item = items[cred_selected_item_idx];
	if (cred_item_role_edit) item["role"] = cred_item_role_edit->get_text();
	if (cred_item_name_edit) item["name"] = cred_item_name_edit->get_text();
	items[cred_selected_item_idx] = item;
	credits_config_draft["items"] = items;
	if (cred_items_list && cred_selected_item_idx < cred_items_list->get_item_count()) {
		cred_items_list->set_item_text(cred_selected_item_idx, s_cred_item_summary(cred_selected_item_idx, item));
	}
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_item_img_changed(const String &p_text) {
	if (updating_ui) return;
	Array items = credits_config_draft.get("items", Array());
	if (cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size()) return;
	Dictionary item = items[cred_selected_item_idx];
	item["image_path"] = p_text.strip_edges();
	items[cred_selected_item_idx] = item;
	credits_config_draft["items"] = items;
	if (cred_items_list && cred_selected_item_idx < cred_items_list->get_item_count()) {
		cred_items_list->set_item_text(cred_selected_item_idx, s_cred_item_summary(cred_selected_item_idx, item));
	}
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_item_img_opt_or_spin_changed(double p_unused) {
	if (updating_ui) return;
	Array items = credits_config_draft.get("items", Array());
	if (cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size()) return;
	Dictionary item = items[cred_selected_item_idx];
	if (cred_item_img_filter_opt) {
		item["filter"] = (cred_item_img_filter_opt->get_selected() == 1) ? "linear" : "nearest";
	}
	if (cred_item_img_w_spin) item["max_width"] = cred_item_img_w_spin->get_value();
	if (cred_item_img_h_spin) item["max_height"] = cred_item_img_h_spin->get_value();
	items[cred_selected_item_idx] = item;
	credits_config_draft["items"] = items;
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_item_spacer_changed(double p_val) {
	if (updating_ui) return;
	Array items = credits_config_draft.get("items", Array());
	if (cred_selected_item_idx < 0 || cred_selected_item_idx >= items.size()) return;
	Dictionary item = items[cred_selected_item_idx];
	item["height"] = CLAMP(p_val, 4.0, 480.0);
	items[cred_selected_item_idx] = item;
	credits_config_draft["items"] = items;
	if (cred_items_list && cred_selected_item_idx < cred_items_list->get_item_count()) {
		cred_items_list->set_item_text(cred_selected_item_idx, s_cred_item_summary(cred_selected_item_idx, item));
	}
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_item_align_selected(int p_idx) {
	if (updating_ui) return;
	String al;
	if (p_idx == 1) al = "left";
	else if (p_idx == 2) al = "center";
	else if (p_idx == 3) al = "right";
	set_credits_item_field(cred_selected_item_idx, "align", al);
}

void ClockGearsDatabaseDialog::_on_cred_bg_or_align_changed(int p_unused) {
	if (updating_ui) return;
	Dictionary cred_bg = credits_config_draft.get("background", Dictionary());
	if (cred_bg_type_opt) cred_bg["type"] = (cred_bg_type_opt->get_selected() == 1) ? "image" : "color";
	if (cred_bg_color_btn) cred_bg["color"] = "#" + cred_bg_color_btn->get_pick_color().to_html(false);
	if (cred_bg_image_edit) cred_bg["image_path"] = cred_bg_image_edit->get_text().strip_edges();
	if (cred_bg_scale_opt) {
		int sc = cred_bg_scale_opt->get_selected();
		cred_bg["scale_mode"] = (sc == 1) ? "fill" : ((sc == 2) ? "stretch" : "fit");
	}
	if (cred_bg_filter_opt) {
		cred_bg["filter"] = (cred_bg_filter_opt->get_selected() == 1) ? "linear" : "nearest";
	}
	credits_config_draft["background"] = cred_bg;
	if (cred_align_opt) {
		int ai = cred_align_opt->get_selected();
		credits_config_draft["alignment"] = (ai == 0) ? "left" : ((ai == 2) ? "right" : "center");
	}
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_bg_color_changed(const Color &p_col) {
	_on_cred_bg_or_align_changed();
}

void ClockGearsDatabaseDialog::_on_cred_bg_image_text_changed(const String &p_text) {
	_on_cred_bg_or_align_changed();
}

void ClockGearsDatabaseDialog::_on_cred_bgm_text_changed(const String &p_text) {
	if (updating_ui) return;
	credits_config_draft["bgm_path"] = p_text.strip_edges();
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_speed_slider_changed(double p_val) {
	if (updating_ui) return;
	bool prev = updating_ui;
	updating_ui = true;
	double v = CLAMP(p_val, 5.0, 600.0);
	if (cred_speed_spin) cred_speed_spin->set_value(v);
	updating_ui = prev;
	credits_config_draft["scroll_speed"] = v;
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_speed_spin_changed(double p_val) {
	if (updating_ui) return;
	bool prev = updating_ui;
	updating_ui = true;
	double v = CLAMP(p_val, 5.0, 600.0);
	if (cred_speed_slider) cred_speed_slider->set_value(v);
	updating_ui = prev;
	credits_config_draft["scroll_speed"] = v;
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_style_size_changed(double p_val, int p_style_idx) {
	if (updating_ui || p_style_idx < 0 || p_style_idx >= 4) return;
	Dictionary styles = credits_config_draft.get("styles", Dictionary());
	String key = s_cred_style_keys[p_style_idx];
	Dictionary st = styles.get(key, Dictionary());
	st["font_size"] = CLAMP(int(p_val), 8, 96);
	styles[key] = st;
	credits_config_draft["styles"] = styles;
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_style_color_changed(const Color &p_col, int p_style_idx) {
	if (updating_ui || p_style_idx < 0 || p_style_idx >= 4) return;
	Dictionary styles = credits_config_draft.get("styles", Dictionary());
	String key = s_cred_style_keys[p_style_idx];
	Dictionary st = styles.get(key, Dictionary());
	st["color"] = "#" + p_col.to_html(false);
	styles[key] = st;
	credits_config_draft["styles"] = styles;
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_pick_asset_pressed(int p_target) {
	cred_asset_pick_target = p_target;
	if (!cred_asset_file_dialog) return;
	cred_asset_file_dialog->clear_filters();
	if (p_target == 2) {
		cred_asset_file_dialog->set_title(CG_DB_TR("Importar música de créditos"));
		cred_asset_file_dialog->add_filter("*.ogg, *.mp3, *.wav ; " + CG_DB_TR("Archivos de audio"));
	} else {
		cred_asset_file_dialog->set_title(p_target == 0 ? CG_DB_TR("Importar imagen de fondo") : CG_DB_TR("Importar imagen de créditos"));
		cred_asset_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes"));
	}
	cred_asset_file_dialog->popup_centered_ratio(0.60f);
}

void ClockGearsDatabaseDialog::_on_cred_clear_asset_pressed(int p_target) {
	if (p_target == 0) {
		Dictionary cred_bg = credits_config_draft.get("background", Dictionary());
		cred_bg["image_path"] = "";
		if (String(cred_bg.get("type", "color")) == "image") {
			cred_bg["type"] = "color";
		}
		credits_config_draft["background"] = cred_bg;
		if (cred_bg_image_edit) cred_bg_image_edit->set_text("");
		if (cred_bg_type_opt) cred_bg_type_opt->select(0);
	} else if (p_target == 1) {
		set_credits_item_field(cred_selected_item_idx, "image_path", String(""));
		return;
	} else if (p_target == 2) {
		credits_config_draft["bgm_path"] = "";
		if (cred_bgm_edit) cred_bgm_edit->set_text("");
	}
	_update_credits_live_preview();
	_mark_database_dirty();
}

void ClockGearsDatabaseDialog::_on_cred_asset_file_selected(const String &p_path) {
	if (cred_asset_pick_target == 2) {
		String final_path = _import_menu_asset_file(p_path, "audio/bgm", false);
		credits_config_draft["bgm_path"] = final_path;
		if (cred_bgm_edit) cred_bgm_edit->set_text(final_path);
		_update_credits_live_preview();
		_mark_database_dirty();
	} else if (cred_asset_pick_target == 1) {
		bool pixel = cred_item_img_filter_opt ? (cred_item_img_filter_opt->get_selected() == 0) : true;
		String final_path = _import_menu_asset_file(p_path, "img/credits", pixel);
		set_credits_item_field(cred_selected_item_idx, "image_path", final_path);
	} else {
		bool pixel = cred_bg_filter_opt ? (cred_bg_filter_opt->get_selected() == 0) : true;
		String final_path = _import_menu_asset_file(p_path, "img/credits", pixel);
		Dictionary cred_bg = credits_config_draft.get("background", Dictionary());
		cred_bg["type"] = "image";
		cred_bg["image_path"] = final_path;
		credits_config_draft["background"] = cred_bg;
		if (cred_bg_type_opt) cred_bg_type_opt->select(1);
		if (cred_bg_image_edit) cred_bg_image_edit->set_text(final_path);
		_update_credits_live_preview();
		_mark_database_dirty();
	}
}

void ClockGearsDatabaseDialog::_on_cred_test_popup_close_requested() {
	if (cred_test_player && cred_test_player->is_playing()) {
		cred_test_player->stop_credits(true);
	}
	if (cred_test_window) {
		cred_test_window->hide();
	}
}

void ClockGearsDatabaseDialog::_on_cred_test_player_finished(bool p_cancelled) {
	if (cred_test_window && cred_test_window->is_visible()) {
		cred_test_window->hide();
	}
}

Dictionary ClockGearsDatabaseDialog::get_credits_draft_copy() const {
	return ClockGearsDataManager::normalize_credits_config(credits_config_draft);
}

Window *ClockGearsDatabaseDialog::open_credits_test_popup() {
	_ensure_cred_test_window();
	if (!cred_test_window || !cred_test_player) {
		return nullptr;
	}
	Size2i popup_sz(960, 540);
	if (DisplayServer::get_singleton()) {
		int scr = DisplayServer::get_singleton()->window_get_current_screen();
		Size2i usable = DisplayServer::get_singleton()->screen_get_usable_rect(scr).size;
		if (usable.x > 0 && usable.y > 0) {
			popup_sz = popup_sz.min(usable);
		}
	}
	cred_test_window->popup_centered(popup_sz);
	cred_test_player->start_credits(ClockGearsDataManager::normalize_credits_config(credits_config_draft));
	return cred_test_window;
}

Dictionary ClockGearsDatabaseDialog::get_credits_editor_ui_handles() {
	_setup_credits_editor_ui();
	Dictionary out;
	out["panel"] = credits_editor_panel;
	out["main_split"] = cred_main_split;
	out["add_type_opt"] = cred_add_type_opt;
	out["add_btn"] = cred_add_btn;
	out["del_btn"] = cred_del_btn;
	out["up_btn"] = cred_up_btn;
	out["down_btn"] = cred_down_btn;
	out["dup_btn"] = cred_dup_btn;
	out["items_list"] = cred_items_list;
	out["item_text_edit"] = cred_item_text_edit;
	out["item_multiline_edit"] = cred_item_multiline_edit;
	out["item_role_edit"] = cred_item_role_edit;
	out["item_name_edit"] = cred_item_name_edit;
	out["item_img_edit"] = cred_item_img_edit;
	out["item_spacer_spin"] = cred_item_spacer_spin;
	out["item_align_opt"] = cred_item_align_opt;
	out["bg_type_opt"] = cred_bg_type_opt;
	out["bg_color_btn"] = cred_bg_color_btn;
	out["bg_image_edit"] = cred_bg_image_edit;
	out["bg_scale_opt"] = cred_bg_scale_opt;
	out["bg_filter_opt"] = cred_bg_filter_opt;
	out["bgm_edit"] = cred_bgm_edit;
	out["speed_slider"] = cred_speed_slider;
	out["speed_spin"] = cred_speed_spin;
	out["align_opt"] = cred_align_opt;
	out["test_btn"] = cred_test_btn;
	out["live_preview"] = cred_live_preview;
	out["test_window"] = cred_test_window;
	out["test_player"] = cred_test_player;
	out["selected_item_idx"] = cred_selected_item_idx;
	return out;
}

// ============================================================================
// Menús -> HUD Editor (9B Tanda 5 UI)
// ============================================================================

static Ref<ImageTexture> s_make_hud_sample_map_texture() {
	const int W = 240;
	const int H = 135;
	Ref<Image> img = Image::create_empty(W, H, false, Image::FORMAT_RGBA8);
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			bool grid_dark = (((x / 16) + (y / 16)) % 2) == 0;
			Color col = grid_dark ? Color(0.18f, 0.34f, 0.22f, 1.0f) : Color(0.20f, 0.37f, 0.24f, 1.0f);

			// Camino horizontal y vertical en cruz
			if (y >= 62 && y <= 76) {
				bool stone = ((x + y * 3) % 11) == 0;
				col = stone ? Color(0.55f, 0.46f, 0.34f, 1.0f) : Color(0.49f, 0.40f, 0.29f, 1.0f);
			} else if (x >= 112 && x <= 128 && y >= 28) {
				bool stone = ((x * 2 + y) % 9) == 0;
				col = stone ? Color(0.54f, 0.45f, 0.33f, 1.0f) : Color(0.47f, 0.38f, 0.28f, 1.0f);
			}

			// Arroyo en la esquina inferior derecha
			if (x >= 166 && y >= 92 && (x - 166) * (x - 166) / 6 + (y - 134) * (y - 134) / 3 < 720) {
				col = ((x + y) % 13 == 0) ? Color(0.32f, 0.62f, 0.84f, 1.0f) : Color(0.16f, 0.42f, 0.68f, 1.0f);
			}

			// Copas de árboles decorativos en los bordes
			auto draw_tree = [&](int cx, int cy, int r) {
				int dx = x - cx;
				int dy = y - cy;
				if (dx * dx + dy * dy <= r * r) {
					col = (dy < -r / 3) ? Color(0.16f, 0.44f, 0.21f, 1.0f) : Color(0.10f, 0.28f, 0.14f, 1.0f);
				}
			};
			draw_tree(32, 32, 13);
			draw_tree(62, 26, 11);
			draw_tree(206, 36, 14);
			draw_tree(28, 106, 12);

			img->set_pixel(x, y, col);
		}
	}
	return ImageTexture::create_from_image(img);
}

static String s_hud_type_label(const String &p_type) {
	if (p_type == "bar") return CG_DB_TR("Barra");
	if (p_type == "hearts") return CG_DB_TR("Corazones");
	if (p_type == "currency") return CG_DB_TR("Monedas");
	if (p_type == "item") return CG_DB_TR("Objeto");
	if (p_type == "text") return CG_DB_TR("Texto");
	return CG_DB_TR("Imagen");
}

static Color s_hud_parse_color(const String &p_hex, const Color &p_fallback) {
	return Color::html_is_valid(p_hex) ? Color::html(p_hex) : p_fallback;
}

String ClockGearsDatabaseDialog::_hud_format_widget_list_label(const Dictionary &p_widget, int p_index) const {
	String wname = String(p_widget.get("name", "Widget")).strip_edges();
	String wtype = s_hud_type_label(String(p_widget.get("type", "bar")));
	int layer = int(p_widget.get("layer_order", 0));
	return vformat("%d. %s [%s | Capa %d]", p_index + 1, wname.is_empty() ? CG_DB_TR("(sin nombre)") : wname, wtype, layer);
}

void ClockGearsDatabaseDialog::_restore_hud_preview_runtime_state() {
	if (!hud_preview_has_saved_rt) {
		return;
	}
	hud_preview_has_saved_rt = false;
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		if (!hud_preview_saved_rt.is_empty()) {
			dm->import_runtime_state(hud_preview_saved_rt);
		}
	}
	hud_preview_saved_rt.clear();
}

void ClockGearsDatabaseDialog::_setup_hud_editor_ui() {
	if (hud_editor_panel || !col2_box) {
		return;
	}
	const float row_h = 30.0f * EDSCALE;
	hud_config_draft = ClockGearsDataManager::get_default_hud_config();
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		hud_config_draft = ClockGearsDataManager::normalize_hud_config(dm->get_hud_config());
	}

	hud_editor_panel = memnew(VBoxContainer);
	hud_editor_panel->set_name("HudEditorPanel");
	hud_editor_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hud_editor_panel->add_theme_constant_override("separation", int(12 * EDSCALE));
	hud_editor_panel->hide();
	col2_box->add_child(hud_editor_panel);

	hud_main_split = memnew(BoxContainer);
	hud_main_split->set_vertical(false);
	hud_main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hud_main_split->add_theme_constant_override("separation", int(14 * EDSCALE));
	hud_editor_panel->add_child(hud_main_split);

	hud_left_col = memnew(VBoxContainer);
	hud_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_left_col->set_stretch_ratio(1.1f);
	hud_left_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	hud_main_split->add_child(hud_left_col);

	VBoxContainer *left_col = hud_left_col;

	auto make_card = [&](VBoxContainer *p_parent, const String &p_hdr_txt, VBoxContainer *&r_body) {
		return _make_collapsible_menu_card(p_parent, p_hdr_txt, r_body, true);
	};

	auto add_grid_label = [&](GridContainer *p_grid, const String &p_txt) {
		Label *lbl = memnew(Label(p_txt));
		lbl->set_custom_minimum_size(Size2(104 * EDSCALE, 0));
		p_grid->add_child(lbl);
	};

	auto add_slider_spin = [&](GridContainer *p_grid, const String &p_lbl, double p_min, double p_max, double p_step, const String &p_suffix, const String &p_key, HSlider *&r_slider, SpinBox *&r_spin, bool p_is_pct_opacity = false) {
		add_grid_label(p_grid, p_lbl);
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		p_grid->add_child(hb);

		r_slider = memnew(HSlider);
		r_slider->set_min(p_min);
		r_slider->set_max(p_max);
		r_slider->set_step(p_step);
		r_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_slider->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
		hb->add_child(r_slider);

		r_spin = memnew(SpinBox);
		r_spin->set_min(p_min);
		r_spin->set_max(p_max);
		r_spin->set_step(p_step);
		if (!p_suffix.is_empty()) {
			r_spin->set_suffix(p_suffix);
		}
		r_spin->set_custom_minimum_size(Size2(84 * EDSCALE, row_h));
		hb->add_child(r_spin);

		r_slider->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_slider_spin_sync).bind(p_key, r_slider, r_spin, p_is_pct_opacity));
		r_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_slider_spin_sync).bind(p_key, r_slider, r_spin, p_is_pct_opacity));
	};

	auto add_image_row = [&](VBoxContainer *p_box, const String &p_lbl, const String &p_key, LineEdit *&r_edit, Button *&r_import, Button *&r_clear) {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		p_box->add_child(hb);

		Label *lbl = memnew(Label(p_lbl));
		lbl->set_custom_minimum_size(Size2(114 * EDSCALE, 0));
		hb->add_child(lbl);

		r_edit = memnew(LineEdit);
		r_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
		r_edit->set_placeholder("res://img/hud/...");
		r_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
		hb->add_child(r_edit);

		r_import = memnew(Button(CG_DB_TR("Importar")));
		r_import->set_custom_minimum_size(Size2(0, row_h));
		r_import->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_pick_asset_pressed).bind(p_key));
		hb->add_child(r_import);

		r_clear = memnew(Button(CG_DB_TR("Limpiar")));
		r_clear->set_custom_minimum_size(Size2(0, row_h));
		r_clear->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_clear_asset_pressed).bind(p_key));
		hb->add_child(r_clear);
	};

	auto add_color_row = [&](GridContainer *p_grid, const String &p_lbl, const String &p_key, bool p_alpha, ColorPickerButton *&r_btn) {
		add_grid_label(p_grid, p_lbl);
		r_btn = memnew(ColorPickerButton);
		r_btn->set_edit_alpha(p_alpha);
		r_btn->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
		r_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_color_changed).bind(p_key));
		p_grid->add_child(r_btn);
	};

	// 1. Lista de widgets
	VBoxContainer *list_body = nullptr;
	make_card(left_col, CG_DB_TR("Widgets del HUD"), list_body);

	HFlowContainer *toolbar = memnew(HFlowContainer);
	toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	toolbar->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	toolbar->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	list_body->add_child(toolbar);

	hud_add_type_opt = memnew(OptionButton);
	hud_add_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_add_type_opt->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	struct HudTypeItem {
		const char *label;
		const char *id;
	};
	static const HudTypeItem k_hud_types[6] = {
		{ "Barra de valor", "bar" },
		{ "Corazones", "hearts" },
		{ "Contador de monedas", "currency" },
		{ "Contador de objetos", "item" },
		{ "Texto", "text" },
		{ "Imagen", "image" }
	};
	for (int i = 0; i < 6; i++) {
		hud_add_type_opt->add_item(CG_DB_TR(k_hud_types[i].label), i);
		hud_add_type_opt->set_item_metadata(i, String(k_hud_types[i].id));
	}
	toolbar->add_child(hud_add_type_opt);

	hud_add_btn = memnew(Button("+"));
	hud_add_btn->set_tooltip_text(CG_DB_TR("Añadir widget"));
	hud_add_btn->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	hud_add_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_add_pressed));
	toolbar->add_child(hud_add_btn);

	hud_del_btn = memnew(Button("-"));
	hud_del_btn->set_tooltip_text(CG_DB_TR("Eliminar widget"));
	hud_del_btn->set_custom_minimum_size(Size2(34 * EDSCALE, row_h));
	hud_del_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_remove_pressed));
	toolbar->add_child(hud_del_btn);

	hud_dup_btn = memnew(Button(CG_DB_TR("Duplicar")));
	hud_dup_btn->set_custom_minimum_size(Size2(0, row_h));
	hud_dup_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_duplicate_pressed));
	toolbar->add_child(hud_dup_btn);

	hud_layer_down_btn = memnew(Button(CG_DB_TR("Bajar capa")));
	hud_layer_down_btn->set_custom_minimum_size(Size2(0, row_h));
	hud_layer_down_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_layer_move_pressed).bind(-1));
	toolbar->add_child(hud_layer_down_btn);

	hud_layer_up_btn = memnew(Button(CG_DB_TR("Subir capa")));
	hud_layer_up_btn->set_custom_minimum_size(Size2(0, row_h));
	hud_layer_up_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_layer_move_pressed).bind(1));
	toolbar->add_child(hud_layer_up_btn);

	hud_widgets_list = memnew(ItemList);
	hud_widgets_list->set_custom_minimum_size(Size2(0, 128 * EDSCALE));
	hud_widgets_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_widgets_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_widget_selected));
	list_body->add_child(hud_widgets_list);

	// 2. Propiedades comunes + anclaje 3x3
	VBoxContainer *common_body = nullptr;
	make_card(left_col, CG_DB_TR("Posición y propiedades comunes"), common_body);

	hud_empty_props_lbl = memnew(Label(CG_DB_TR("Añade o selecciona un widget del HUD para editar sus propiedades.")));
	hud_empty_props_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	common_body->add_child(hud_empty_props_lbl);

	hud_common_box = memnew(VBoxContainer);
	hud_common_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_common_box->add_theme_constant_override("separation", int(7 * EDSCALE));
	common_body->add_child(hud_common_box);

	GridContainer *cgrid = memnew(GridContainer);
	cgrid->set_columns(2);
	cgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	cgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_common_box->add_child(cgrid);

	add_grid_label(cgrid, CG_DB_TR("Nombre:"));
	hud_name_edit = memnew(LineEdit);
	hud_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_name_edit->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_name_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	cgrid->add_child(hud_name_edit);

	add_grid_label(cgrid, CG_DB_TR("Anclaje (3x3):"));
	HBoxContainer *anchor_row = memnew(HBoxContainer);
	anchor_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anchor_row->add_theme_constant_override("separation", int(12 * EDSCALE));
	cgrid->add_child(anchor_row);

	GridContainer *anchor_grid = memnew(GridContainer);
	anchor_grid->set_columns(3);
	anchor_grid->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	anchor_grid->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	VBoxContainer *anchor_col = memnew(VBoxContainer);
	anchor_col->add_theme_constant_override("separation", int(4 * EDSCALE));
	anchor_row->add_child(anchor_col);
	anchor_col->add_child(anchor_grid);
	hud_free_anchor_btn = memnew(Button(CG_DB_TR("Libre (desanclado)")));
	hud_free_anchor_btn->set_theme_type_variation("ClockGearsTabButton");
	hud_free_anchor_btn->set_toggle_mode(true);
	hud_free_anchor_btn->set_tooltip_text(CG_DB_TR("El widget se coloca en píxeles exactos sobre 960x540. Arrástralo en la vista previa o usa X e Y."));
	hud_free_anchor_btn->set_custom_minimum_size(Size2(0, 28 * EDSCALE));
	hud_free_anchor_btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_anchor_pressed).bind(String("free")));
	anchor_col->add_child(hud_free_anchor_btn);

	Array anchor_ids = ClockGearsDataManager::get_hud_anchor_ids();
	static const char *k_anchor_glyphs[9] = { "↖", "↑", "↗", "←", "•", "→", "↙", "↓", "↘" };
	static const char *k_anchor_tips[9] = {
		"Arriba izquierda", "Arriba centro", "Arriba derecha",
		"Centro izquierda", "Centro", "Centro derecha",
		"Abajo izquierda", "Abajo centro", "Abajo derecha"
	};
	for (int i = 0; i < 9; i++) {
		String aid = (i < anchor_ids.size()) ? String(anchor_ids[i]) : String("top_left");
		Button *ab = memnew(Button(String::utf8(k_anchor_glyphs[i])));
		ab->set_theme_type_variation("ClockGearsTabButton");
		ab->set_toggle_mode(true);
		ab->set_tooltip_text(CG_DB_TR(k_anchor_tips[i]));
		ab->set_custom_minimum_size(Size2(34 * EDSCALE, 28 * EDSCALE));
		ab->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_anchor_pressed).bind(aid));
		anchor_grid->add_child(ab);
		hud_anchor_btns[i] = ab;
	}

	VBoxContainer *xy_box = memnew(VBoxContainer);
	xy_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	xy_box->add_theme_constant_override("separation", int(5 * EDSCALE));
	anchor_row->add_child(xy_box);

	HBoxContainer *x_hb = memnew(HBoxContainer);
	x_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	xy_box->add_child(x_hb);
	x_hb->add_child(memnew(Label("X:")));
	hud_offset_x_spin = memnew(SpinBox);
	hud_offset_x_spin->set_min(-960);
	hud_offset_x_spin->set_max(960);
	hud_offset_x_spin->set_suffix(" px");
	hud_offset_x_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_offset_x_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	x_hb->add_child(hud_offset_x_spin);

	HBoxContainer *y_hb = memnew(HBoxContainer);
	y_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	xy_box->add_child(y_hb);
	y_hb->add_child(memnew(Label("Y:")));
	hud_offset_y_spin = memnew(SpinBox);
	hud_offset_y_spin->set_min(-540);
	hud_offset_y_spin->set_max(540);
	hud_offset_y_spin->set_suffix(" px");
	hud_offset_y_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_offset_y_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	y_hb->add_child(hud_offset_y_spin);

	add_slider_spin(cgrid, CG_DB_TR("Ancho:"), 16, 960, 1, " px", "width", hud_width_slider, hud_width_spin);
	add_slider_spin(cgrid, CG_DB_TR("Alto:"), 16, 540, 1, " px", "height", hud_height_slider, hud_height_spin);

	add_grid_label(cgrid, CG_DB_TR("Orden de capa:"));
	hud_layer_spin = memnew(SpinBox);
	hud_layer_spin->set_min(-100);
	hud_layer_spin->set_max(1000);
	hud_layer_spin->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	hud_layer_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	cgrid->add_child(hud_layer_spin);

	add_grid_label(cgrid, CG_DB_TR("Estilo ventana:"));
	hud_window_style_opt = memnew(OptionButton);
	hud_window_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_window_style_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_window_style_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	cgrid->add_child(hud_window_style_opt);

	add_grid_label(cgrid, CG_DB_TR("Visibilidad:"));
	hud_vis_mode_opt = memnew(OptionButton);
	hud_vis_mode_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_vis_mode_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_vis_mode_opt->add_item(CG_DB_TR("Siempre"), 0);
	hud_vis_mode_opt->set_item_metadata(0, String("always"));
	hud_vis_mode_opt->add_item(CG_DB_TR("Con interruptor"), 1);
	hud_vis_mode_opt->set_item_metadata(1, String("switch"));
	hud_vis_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	cgrid->add_child(hud_vis_mode_opt);

	hud_vis_switch_row = memnew(HBoxContainer);
	hud_vis_switch_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_vis_switch_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	hud_common_box->add_child(hud_vis_switch_row);

	Label *sw_lbl = memnew(Label(CG_DB_TR("Interruptor:")));
	sw_lbl->set_custom_minimum_size(Size2(114 * EDSCALE, 0));
	hud_vis_switch_row->add_child(sw_lbl);

	hud_vis_switch_opt = memnew(OptionButton);
	hud_vis_switch_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_vis_switch_opt->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	hud_vis_switch_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_switch_opt_selected));
	hud_vis_switch_row->add_child(hud_vis_switch_opt);

	hud_vis_switch_edit = memnew(LineEdit);
	hud_vis_switch_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_vis_switch_edit->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	hud_vis_switch_edit->set_placeholder(CG_DB_TR("Nombre o ID"));
	hud_vis_switch_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	hud_vis_switch_row->add_child(hud_vis_switch_edit);

	HBoxContainer *chk_row = memnew(HBoxContainer);
	chk_row->add_theme_constant_override("separation", int(14 * EDSCALE));
	hud_common_box->add_child(chk_row);

	hud_hide_ctx_chk = memnew(CheckBox(CG_DB_TR("Ocultar en mensajes y menús")));
	hud_hide_ctx_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	chk_row->add_child(hud_hide_ctx_chk);

	hud_pixel_art_chk = memnew(CheckBox(CG_DB_TR("Pixel Art (Nearest)")));
	hud_pixel_art_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_common_changed).unbind(1));
	chk_row->add_child(hud_pixel_art_chk);

	// 3. Propiedades específicas según tipo
	VBoxContainer *type_body = nullptr;
	hud_type_card = make_card(left_col, CG_DB_TR("Propiedades del widget"), type_body);

	// --- 3A. Barra ---
	hud_bar_box = memnew(VBoxContainer);
	hud_bar_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_bar_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	type_body->add_child(hud_bar_box);

	GridContainer *bgrid = memnew(GridContainer);
	bgrid->set_columns(2);
	bgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	bgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_bar_box->add_child(bgrid);

	add_grid_label(bgrid, CG_DB_TR("Fuente:"));
	hud_bar_source_opt = memnew(OptionButton);
	hud_bar_source_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_bar_source_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_bar_source_opt->add_item(CG_DB_TR("HP del jugador"), 0);
	hud_bar_source_opt->set_item_metadata(0, String("hp"));
	hud_bar_source_opt->add_item(CG_DB_TR("MP del jugador"), 1);
	hud_bar_source_opt->set_item_metadata(1, String("mp"));
	hud_bar_source_opt->add_item(CG_DB_TR("EXP del jugador"), 2);
	hud_bar_source_opt->set_item_metadata(2, String("exp"));
	hud_bar_source_opt->add_item(CG_DB_TR("Variable"), 3);
	hud_bar_source_opt->set_item_metadata(3, String("variable"));
	hud_bar_source_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	bgrid->add_child(hud_bar_source_opt);

	hud_bar_var_box = memnew(VBoxContainer);
	hud_bar_var_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_bar_var_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	hud_bar_box->add_child(hud_bar_var_box);

	HBoxContainer *bvar_row = memnew(HBoxContainer);
	bvar_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	hud_bar_var_box->add_child(bvar_row);
	Label *bvl = memnew(Label(CG_DB_TR("Variable:")));
	bvl->set_custom_minimum_size(Size2(114 * EDSCALE, 0));
	bvar_row->add_child(bvl);

	hud_bar_var_opt = memnew(OptionButton);
	hud_bar_var_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_bar_var_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	bvar_row->add_child(hud_bar_var_opt);

	hud_bar_var_edit = memnew(LineEdit);
	hud_bar_var_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_bar_var_edit->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	hud_bar_var_edit->set_placeholder(CG_DB_TR("Nombre variable"));
	hud_bar_var_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	bvar_row->add_child(hud_bar_var_edit);
	hud_bar_var_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_var_opt_selected).bind(hud_bar_var_opt, hud_bar_var_edit));

	HBoxContainer *bmax_row = memnew(HBoxContainer);
	bmax_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	hud_bar_var_box->add_child(bmax_row);
	Label *bvml = memnew(Label(CG_DB_TR("Valor máximo:")));
	bvml->set_custom_minimum_size(Size2(114 * EDSCALE, 0));
	bmax_row->add_child(bvml);
	hud_bar_var_max_spin = memnew(SpinBox);
	hud_bar_var_max_spin->set_min(1);
	hud_bar_var_max_spin->set_max(99999999);
	hud_bar_var_max_spin->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	hud_bar_var_max_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	bmax_row->add_child(hud_bar_var_max_spin);

	GridContainer *bcgrid = memnew(GridContainer);
	bcgrid->set_columns(2);
	bcgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bcgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	bcgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_bar_box->add_child(bcgrid);

	add_color_row(bcgrid, CG_DB_TR("Color fondo:"), "bg_color", true, hud_bar_bg_col_btn);
	add_color_row(bcgrid, CG_DB_TR("Color relleno:"), "fill_color", true, hud_bar_fill_col_btn);
	add_color_row(bcgrid, CG_DB_TR("Color borde:"), "border_color", true, hud_bar_border_col_btn);
	add_color_row(bcgrid, CG_DB_TR("Color texto:"), "font_color", false, hud_bar_font_col_btn);

	add_grid_label(bcgrid, CG_DB_TR("Etiqueta:"));
	hud_bar_label_edit = memnew(LineEdit);
	hud_bar_label_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_bar_label_edit->set_custom_minimum_size(Size2(100 * EDSCALE, row_h));
	hud_bar_label_edit->set_placeholder("HP / MP...");
	hud_bar_label_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	bcgrid->add_child(hud_bar_label_edit);

	add_slider_spin(bcgrid, CG_DB_TR("Tamaño fuente:"), 8, 48, 1, " px", "font_size", hud_bar_font_size_slider, hud_bar_font_size_spin);

	hud_bar_show_nums_chk = memnew(CheckBox(CG_DB_TR("Mostrar números")));
	hud_bar_show_nums_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hud_bar_box->add_child(hud_bar_show_nums_chk);

	add_image_row(hud_bar_box, CG_DB_TR("Img. fondo:"), "bg_image_path", hud_bar_bg_img_edit, hud_bar_bg_img_import_btn, hud_bar_bg_img_clear_btn);
	add_image_row(hud_bar_box, CG_DB_TR("Img. relleno:"), "fill_image_path", hud_bar_fill_img_edit, hud_bar_fill_img_import_btn, hud_bar_fill_img_clear_btn);

	// --- 3B. Corazones ---
	hud_hearts_box = memnew(VBoxContainer);
	hud_hearts_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_hearts_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	type_body->add_child(hud_hearts_box);

	hud_hearts_show_bg_chk = memnew(CheckBox(CG_DB_TR("Mostrar fondo")));
	hud_hearts_show_bg_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hud_hearts_box->add_child(hud_hearts_show_bg_chk);
	hud_hearts_auto_size_chk = memnew(CheckBox(CG_DB_TR("Tamaño automático")));
	hud_hearts_auto_size_chk->set_tooltip_text(CG_DB_TR("El widget se ajusta a corazones × tamaño × separación. Desactívalo para usar Ancho y Alto manuales."));
	hud_hearts_auto_size_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hud_hearts_box->add_child(hud_hearts_auto_size_chk);
	{
		HBoxContainer *bg_row = memnew(HBoxContainer);
		bg_row->add_theme_constant_override("separation", int(8 * EDSCALE));
		hud_hearts_box->add_child(bg_row);
		Label *bc_lbl = memnew(Label(CG_DB_TR("Color fondo:")));
		bg_row->add_child(bc_lbl);
		hud_hearts_bg_color_btn = memnew(ColorPickerButton);
		hud_hearts_bg_color_btn->set_edit_alpha(false);
		hud_hearts_bg_color_btn->set_custom_minimum_size(Size2(60 * EDSCALE, 26 * EDSCALE));
		hud_hearts_bg_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
		bg_row->add_child(hud_hearts_bg_color_btn);
		Label *bo_lbl = memnew(Label(CG_DB_TR("Opacidad:")));
		bg_row->add_child(bo_lbl);
		hud_hearts_bg_opacity_spin = memnew(SpinBox);
		hud_hearts_bg_opacity_spin->set_min(0);
		hud_hearts_bg_opacity_spin->set_max(100);
		hud_hearts_bg_opacity_spin->set_suffix("%");
		hud_hearts_bg_opacity_spin->set_value(60);
		hud_hearts_bg_opacity_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
		bg_row->add_child(hud_hearts_bg_opacity_spin);
	}

	GridContainer *hgrid = memnew(GridContainer);
	hgrid->set_columns(2);
	hgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	hgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_hearts_box->add_child(hgrid);

	add_grid_label(hgrid, CG_DB_TR("HP / corazón:"));
	hud_hearts_hp_spin = memnew(SpinBox);
	hud_hearts_hp_spin->set_min(1);
	hud_hearts_hp_spin->set_max(99999);
	hud_hearts_hp_spin->set_custom_minimum_size(Size2(110 * EDSCALE, row_h));
	hud_hearts_hp_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hgrid->add_child(hud_hearts_hp_spin);

	add_slider_spin(hgrid, CG_DB_TR("Tam. corazón:"), 8, 128, 1, " px", "heart_size", hud_hearts_size_slider, hud_hearts_size_spin);
	add_slider_spin(hgrid, CG_DB_TR("Separación:"), 0, 64, 1, " px", "spacing", hud_hearts_spacing_slider, hud_hearts_spacing_spin);

	add_grid_label(hgrid, CG_DB_TR("Por fila:"));
	hud_hearts_per_row_spin = memnew(SpinBox);
	hud_hearts_per_row_spin->set_min(1);
	hud_hearts_per_row_spin->set_max(50);
	hud_hearts_per_row_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hgrid->add_child(hud_hearts_per_row_spin);

	add_grid_label(hgrid, CG_DB_TR("Máx. filas:"));
	hud_hearts_max_rows_spin = memnew(SpinBox);
	hud_hearts_max_rows_spin->set_min(1);
	hud_hearts_max_rows_spin->set_max(20);
	hud_hearts_max_rows_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hgrid->add_child(hud_hearts_max_rows_spin);

	add_image_row(hud_hearts_box, CG_DB_TR("Corazón lleno:"), "full_image_path", hud_heart_full_edit, hud_heart_full_import_btn, hud_heart_full_clear_btn);
	add_image_row(hud_hearts_box, CG_DB_TR("Medio corazón:"), "half_image_path", hud_heart_half_edit, hud_heart_half_import_btn, hud_heart_half_clear_btn);
	add_image_row(hud_hearts_box, CG_DB_TR("Corazón vacío:"), "empty_image_path", hud_heart_empty_edit, hud_heart_empty_import_btn, hud_heart_empty_clear_btn);

	// --- 3C. Monedas ---
	hud_currency_box = memnew(VBoxContainer);
	hud_currency_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_currency_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	type_body->add_child(hud_currency_box);

	HBoxContainer *cur_chk_hb = memnew(HBoxContainer);
	cur_chk_hb->add_theme_constant_override("separation", int(12 * EDSCALE));
	hud_currency_box->add_child(cur_chk_hb);
	hud_cur_show_icon_chk = memnew(CheckBox(CG_DB_TR("Mostrar icono")));
	hud_cur_show_icon_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	cur_chk_hb->add_child(hud_cur_show_icon_chk);
	hud_cur_show_name_chk = memnew(CheckBox(CG_DB_TR("Mostrar nombre moneda")));
	hud_cur_show_name_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	cur_chk_hb->add_child(hud_cur_show_name_chk);

	add_image_row(hud_currency_box, CG_DB_TR("Icono moneda:"), "icon_path", hud_cur_icon_edit, hud_cur_icon_import_btn, hud_cur_icon_clear_btn);

	GridContainer *curgrid = memnew(GridContainer);
	curgrid->set_columns(2);
	curgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	curgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	curgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_currency_box->add_child(curgrid);
	add_slider_spin(curgrid, CG_DB_TR("Tamaño fuente:"), 8, 48, 1, " px", "font_size", hud_cur_font_size_slider, hud_cur_font_size_spin);
	add_color_row(curgrid, CG_DB_TR("Color fuente:"), "font_color", false, hud_cur_font_col_btn);

	// --- 3D. Contador de objetos ---
	hud_item_box = memnew(VBoxContainer);
	hud_item_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_item_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	type_body->add_child(hud_item_box);

	GridContainer *igrid = memnew(GridContainer);
	igrid->set_columns(2);
	igrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	igrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	igrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_item_box->add_child(igrid);

	add_grid_label(igrid, CG_DB_TR("Objeto (BD):"));
	hud_item_db_opt = memnew(OptionButton);
	hud_item_db_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_item_db_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_item_db_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	igrid->add_child(hud_item_db_opt);

	add_slider_spin(igrid, CG_DB_TR("Tamaño fuente:"), 8, 48, 1, " px", "font_size", hud_item_font_size_slider, hud_item_font_size_spin);
	add_color_row(igrid, CG_DB_TR("Color fuente:"), "font_color", false, hud_item_font_col_btn);

	HBoxContainer *item_chk_hb = memnew(HBoxContainer);
	item_chk_hb->add_theme_constant_override("separation", int(12 * EDSCALE));
	hud_item_box->add_child(item_chk_hb);
	hud_item_hide_zero_chk = memnew(CheckBox(CG_DB_TR("Ocultar si es 0")));
	hud_item_hide_zero_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	item_chk_hb->add_child(hud_item_hide_zero_chk);
	hud_item_show_name_chk = memnew(CheckBox(CG_DB_TR("Mostrar nombre")));
	hud_item_show_name_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	item_chk_hb->add_child(hud_item_show_name_chk);

	add_image_row(hud_item_box, CG_DB_TR("Icono propio:"), "icon_path", hud_item_icon_edit, hud_item_icon_import_btn, hud_item_icon_clear_btn);

	// --- 3E. Texto ---
	hud_text_box = memnew(VBoxContainer);
	hud_text_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_text_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	type_body->add_child(hud_text_box);

	GridContainer *tgrid = memnew(GridContainer);
	tgrid->set_columns(2);
	tgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	tgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_text_box->add_child(tgrid);

	add_grid_label(tgrid, CG_DB_TR("Contenido:"));
	hud_text_mode_opt = memnew(OptionButton);
	hud_text_mode_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_text_mode_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_text_mode_opt->add_item(CG_DB_TR("Texto fijo"), 0);
	hud_text_mode_opt->set_item_metadata(0, String("fixed"));
	hud_text_mode_opt->add_item(CG_DB_TR("Variable"), 1);
	hud_text_mode_opt->set_item_metadata(1, String("variable"));
	hud_text_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	tgrid->add_child(hud_text_mode_opt);

	hud_text_fixed_row = memnew(HBoxContainer);
	hud_text_fixed_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	hud_text_box->add_child(hud_text_fixed_row);
	Label *tfl = memnew(Label(CG_DB_TR("Texto:")));
	tfl->set_custom_minimum_size(Size2(114 * EDSCALE, 0));
	hud_text_fixed_row->add_child(tfl);
	hud_text_edit = memnew(LineEdit);
	hud_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_text_edit->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_text_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hud_text_fixed_row->add_child(hud_text_edit);

	hud_text_var_row = memnew(HBoxContainer);
	hud_text_var_row->add_theme_constant_override("separation", int(6 * EDSCALE));
	hud_text_box->add_child(hud_text_var_row);
	Label *tvl = memnew(Label(CG_DB_TR("Variable:")));
	tvl->set_custom_minimum_size(Size2(114 * EDSCALE, 0));
	hud_text_var_row->add_child(tvl);
	hud_text_var_opt = memnew(OptionButton);
	hud_text_var_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_text_var_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	hud_text_var_row->add_child(hud_text_var_opt);
	hud_text_var_edit = memnew(LineEdit);
	hud_text_var_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_text_var_edit->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	hud_text_var_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	hud_text_var_row->add_child(hud_text_var_edit);
	hud_text_var_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_var_opt_selected).bind(hud_text_var_opt, hud_text_var_edit));

	GridContainer *tgrid2 = memnew(GridContainer);
	tgrid2->set_columns(2);
	tgrid2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tgrid2->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	tgrid2->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_text_box->add_child(tgrid2);

	add_grid_label(tgrid2, CG_DB_TR("Alineación:"));
	hud_text_align_opt = memnew(OptionButton);
	hud_text_align_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_text_align_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_text_align_opt->add_item(CG_DB_TR("Izquierda"), 0);
	hud_text_align_opt->set_item_metadata(0, String("left"));
	hud_text_align_opt->add_item(CG_DB_TR("Centro"), 1);
	hud_text_align_opt->set_item_metadata(1, String("center"));
	hud_text_align_opt->add_item(CG_DB_TR("Derecha"), 2);
	hud_text_align_opt->set_item_metadata(2, String("right"));
	hud_text_align_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	tgrid2->add_child(hud_text_align_opt);

	add_slider_spin(tgrid2, CG_DB_TR("Tamaño fuente:"), 8, 64, 1, " px", "font_size", hud_text_font_size_slider, hud_text_font_size_spin);
	add_color_row(tgrid2, CG_DB_TR("Color texto:"), "font_color", false, hud_text_font_col_btn);

	// --- 3F. Imagen ---
	hud_image_box = memnew(VBoxContainer);
	hud_image_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_image_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	type_body->add_child(hud_image_box);

	add_image_row(hud_image_box, CG_DB_TR("Imagen:"), "image_path", hud_img_path_edit, hud_img_import_btn, hud_img_clear_btn);

	GridContainer *imgrid = memnew(GridContainer);
	imgrid->set_columns(2);
	imgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	imgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	imgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	hud_image_box->add_child(imgrid);

	add_grid_label(imgrid, CG_DB_TR("Escala:"));
	hud_img_scale_opt = memnew(OptionButton);
	hud_img_scale_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_img_scale_opt->set_custom_minimum_size(Size2(120 * EDSCALE, row_h));
	hud_img_scale_opt->add_item(CG_DB_TR("Ajustar"), 0);
	hud_img_scale_opt->set_item_metadata(0, String("fit"));
	hud_img_scale_opt->add_item(CG_DB_TR("Estirar"), 1);
	hud_img_scale_opt->set_item_metadata(1, String("stretch"));
	hud_img_scale_opt->add_item(CG_DB_TR("Repetir"), 2);
	hud_img_scale_opt->set_item_metadata(2, String("tile"));
	hud_img_scale_opt->add_item(CG_DB_TR("Tamaño real"), 3);
	hud_img_scale_opt->set_item_metadata(3, String("native"));
	hud_img_scale_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_type_field_changed).unbind(1));
	imgrid->add_child(hud_img_scale_opt);

	add_slider_spin(imgrid, CG_DB_TR("Opacidad:"), 0, 100, 1, " %", "opacity", hud_img_opacity_slider, hud_img_opacity_spin, true);

	// Columna derecha: Vista previa 16:9 sobre mapa de ejemplo + Datos de ejemplo
	hud_right_col = memnew(VBoxContainer);
	hud_right_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	hud_right_col->set_stretch_ratio(1.05f);
	hud_right_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	hud_main_split->add_child(hud_right_col);

	VBoxContainer *right_col = hud_right_col;

	VBoxContainer *prev_vb = nullptr;
	make_card(right_col, CG_DB_TR("Vista previa en vivo (16:9)"), prev_vb);

	AspectRatioContainer *aspect = Object::cast_to<AspectRatioContainer>(_build_16_9_viewport_preview(prev_vb, hud_preview_viewport, hud_prev_title_lbl, hud_preview_res_opt));
	_sync_common_preview_res_options();

	TextureRect *preview_disp = nullptr;
	for (int ci = 0; aspect && ci < aspect->get_child_count(); ci++) {
		if (TextureRect *tr = Object::cast_to<TextureRect>(aspect->get_child(ci))) {
			preview_disp = tr;
			break;
		}
	}
	if (preview_disp) {
		preview_disp->set_name("HudPreviewDisplay");
	}

	// Capa transparente encima de la vista previa: selección y arrastre de widgets en modo libre.
	hud_preview_drag_overlay = memnew(Control);
	hud_preview_drag_overlay->set_name("HudPreviewDragOverlay");
	hud_preview_drag_overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	hud_preview_drag_overlay->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	hud_preview_drag_overlay->set_tooltip_text(CG_DB_TR("Clic para seleccionar un widget. Los widgets en modo Libre se arrastran."));
	hud_preview_drag_overlay->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_preview_gui_input));
	aspect->add_child(hud_preview_drag_overlay);

	hud_preview_map_bg = memnew(TextureRect);
	hud_preview_map_bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	hud_preview_map_bg->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	hud_preview_map_bg->set_stretch_mode(TextureRect::STRETCH_SCALE);
	hud_preview_map_bg->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	hud_preview_map_bg->set_texture(s_make_hud_sample_map_texture());
	hud_preview_viewport->add_child(hud_preview_map_bg);

	hud_live_preview = memnew(ClockGearsHudOverlay);
	hud_live_preview->set_preview_mode(true);
	hud_live_preview->set_viewport_size_override(Size2(960, 540));
	hud_preview_viewport->add_child(hud_live_preview);

	// Card: Datos de ejemplo
	VBoxContainer *samp_vb = nullptr;
	make_card(right_col, CG_DB_TR("Datos de ejemplo (vista previa)"), samp_vb);

	GridContainer *sgrid = memnew(GridContainer);
	sgrid->set_columns(2);
	sgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	sgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	samp_vb->add_child(sgrid);

	add_grid_label(sgrid, CG_DB_TR("HP actual:"));
	hud_sample_hp_spin = memnew(SpinBox);
	hud_sample_hp_spin->set_min(0);
	hud_sample_hp_spin->set_max(99999);
	hud_sample_hp_spin->set_value(75);
	hud_sample_hp_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_sample_hp_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_sample_value_changed));
	sgrid->add_child(hud_sample_hp_spin);

	add_grid_label(sgrid, CG_DB_TR("Monedas:"));
	hud_sample_gold_spin = memnew(SpinBox);
	hud_sample_gold_spin->set_min(0);
	hud_sample_gold_spin->set_max(99999999);
	hud_sample_gold_spin->set_value(250);
	hud_sample_gold_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_sample_gold_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_sample_value_changed));
	sgrid->add_child(hud_sample_gold_spin);

	add_grid_label(sgrid, CG_DB_TR("Cant. objeto:"));
	hud_sample_item_qty_spin = memnew(SpinBox);
	hud_sample_item_qty_spin->set_min(0);
	hud_sample_item_qty_spin->set_max(9999);
	hud_sample_item_qty_spin->set_value(3);
	hud_sample_item_qty_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hud_sample_item_qty_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_sample_value_changed));
	sgrid->add_child(hud_sample_item_qty_spin);

	hud_asset_file_dialog = memnew(FileDialog);
	hud_asset_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	hud_asset_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	hud_asset_file_dialog->set_title(CG_DB_TR("Importar imagen para el HUD"));
	hud_asset_file_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; " + CG_DB_TR("Imágenes"));
	hud_asset_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_hud_asset_file_selected));
	add_child(hud_asset_file_dialog);
}

void ClockGearsDatabaseDialog::_refresh_hud_dynamic_options() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (hud_window_style_opt) {
		hud_window_style_opt->clear();
		hud_window_style_opt->add_item(CG_DB_TR("Sin estilo (transparente)"), 0);
		hud_window_style_opt->set_item_metadata(0, String(""));
		hud_window_style_opt->add_item(CG_DB_TR("(Por defecto del proyecto)"), 1);
		hud_window_style_opt->set_item_metadata(1, String("default"));
		if (dm) {
			Array styles = dm->get_window_styles();
			for (int i = 0; i < styles.size(); i++) {
				if (styles[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary st = styles[i];
				String sid = String(st.get("id", ""));
				String sname = String(st.get("name", sid));
				int idx = hud_window_style_opt->get_item_count();
				hud_window_style_opt->add_item(sname, idx);
				hud_window_style_opt->set_item_metadata(idx, sid);
			}
		}
	}

	if (hud_vis_switch_opt) {
		hud_vis_switch_opt->clear();
		hud_vis_switch_opt->add_item(CG_DB_TR("(Escribir interruptor...)"), 0);
		hud_vis_switch_opt->set_item_metadata(0, String(""));
		if (dm) {
			Dictionary sw = dm->get_switches_names();
			Array keys = sw.keys();
			keys.sort();
			for (int i = 0; i < keys.size(); i++) {
				String k = String(keys[i]);
				String label = String(sw[k]).strip_edges();
				if (label.is_empty()) continue;
				int idx = hud_vis_switch_opt->get_item_count();
				hud_vis_switch_opt->add_item(vformat("#%s: %s", k, label), idx);
				hud_vis_switch_opt->set_item_metadata(idx, label);
			}
		}
	}

	auto fill_vars = [&](OptionButton *p_opt) {
		if (!p_opt) return;
		p_opt->clear();
		p_opt->add_item(CG_DB_TR("(Escribir variable...)"), 0);
		p_opt->set_item_metadata(0, String(""));
		if (dm) {
			Dictionary vs = dm->get_variables_names();
			Array keys = vs.keys();
			keys.sort();
			for (int i = 0; i < keys.size(); i++) {
				String k = String(keys[i]);
				String label = String(vs[k]).strip_edges();
				if (label.is_empty()) continue;
				int idx = p_opt->get_item_count();
				p_opt->add_item(vformat("#%s: %s", k, label), idx);
				p_opt->set_item_metadata(idx, label);
			}
		}
	};
	fill_vars(hud_bar_var_opt);
	fill_vars(hud_text_var_opt);

	if (hud_item_db_opt) {
		hud_item_db_opt->clear();
		hud_item_db_opt->add_item(CG_DB_TR("(Ningún objeto)"), 0);
		hud_item_db_opt->set_item_metadata(0, String(""));
		if (dm) {
			static const int k_cats[4] = { CAT_ITEMS, CAT_WEAPONS, CAT_ARMORS, CAT_ACCESSORIES };
			for (int ci = 0; ci < 4; ci++) {
				int cat = k_cats[ci];
				Array arr = dm->get_category_array(cat);
				for (int i = 0; i < arr.size(); i++) {
					if (arr[i].get_type() != Variant::DICTIONARY) continue;
					Dictionary rec = arr[i];
					int rid = int(rec.get("id", 0));
					String rname = String(rec.get("name", "")).strip_edges();
					if (rid <= 0 || rname.is_empty()) continue;
					String inv_key = dm->get_record_inventory_key(cat, rid);
					if (inv_key.is_empty()) {
						inv_key = rname;
					}
					int idx = hud_item_db_opt->get_item_count();
					hud_item_db_opt->add_item(vformat("[%s #%d] %s", _get_category_name(cat), rid, rname), idx);
					hud_item_db_opt->set_item_metadata(idx, inv_key);
				}
			}
		}
	}
}

void ClockGearsDatabaseDialog::_rebuild_hud_widgets_list() {
	if (!hud_widgets_list) {
		return;
	}
	hud_widgets_list->clear();
	Array widgets = hud_config_draft.get("widgets", Array());
	for (int i = 0; i < widgets.size(); i++) {
		if (widgets[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		hud_widgets_list->add_item(_hud_format_widget_list_label(Dictionary(widgets[i]), i));
	}
	if (widgets.is_empty()) {
		hud_selected_widget_idx = -1;
	} else if (hud_selected_widget_idx < 0 || hud_selected_widget_idx >= widgets.size()) {
		hud_selected_widget_idx = 0;
	}
	if (hud_selected_widget_idx >= 0 && hud_selected_widget_idx < hud_widgets_list->get_item_count()) {
		hud_widgets_list->select(hud_selected_widget_idx);
	}
	if (hud_del_btn) hud_del_btn->set_disabled(hud_selected_widget_idx < 0);
	if (hud_dup_btn) hud_dup_btn->set_disabled(hud_selected_widget_idx < 0);
	if (hud_layer_down_btn) hud_layer_down_btn->set_disabled(hud_selected_widget_idx < 0);
	if (hud_layer_up_btn) hud_layer_up_btn->set_disabled(hud_selected_widget_idx < 0);
}

void ClockGearsDatabaseDialog::_refresh_hud_selected_widget_props() {
	Array widgets = hud_config_draft.get("widgets", Array());
	bool valid = (hud_selected_widget_idx >= 0 && hud_selected_widget_idx < widgets.size() && widgets[hud_selected_widget_idx].get_type() == Variant::DICTIONARY);
	if (hud_empty_props_lbl) hud_empty_props_lbl->set_visible(!valid);
	if (hud_common_box) hud_common_box->set_visible(valid);
	if (hud_type_card) hud_type_card->set_visible(valid);
	if (!valid) {
		return;
	}

	bool prev_upd = updating_ui;
	updating_ui = true;
	Dictionary w = ClockGearsDataManager::normalize_hud_widget(Dictionary(widgets[hud_selected_widget_idx]), hud_selected_widget_idx + 1);
	String wtype = String(w.get("type", "bar"));

	if (hud_name_edit) hud_name_edit->set_text(String(w.get("name", "")));
	String anchor = String(w.get("anchor", "top_left"));
	Array anchor_ids = ClockGearsDataManager::get_hud_anchor_ids();
	for (int i = 0; i < 9; i++) {
		if (!hud_anchor_btns[i]) continue;
		String aid = (i < anchor_ids.size()) ? String(anchor_ids[i]) : String();
		hud_anchor_btns[i]->set_pressed_no_signal(aid == anchor);
	}
	bool is_free = (anchor == "free");
	if (hud_free_anchor_btn) hud_free_anchor_btn->set_pressed_no_signal(is_free);
	if (hud_offset_x_spin) {
		hud_offset_x_spin->set_min(is_free ? 0 : -960);
		hud_offset_x_spin->set_max(960);
	}
	if (hud_offset_y_spin) {
		hud_offset_y_spin->set_min(is_free ? 0 : -540);
		hud_offset_y_spin->set_max(540);
	}
	if (hud_offset_x_spin) hud_offset_x_spin->set_value(int(w.get("offset_x", 0)));
	if (hud_offset_y_spin) hud_offset_y_spin->set_value(int(w.get("offset_y", 0)));
	if (hud_width_slider) hud_width_slider->set_value(int(w.get("width", 160)));
	if (hud_width_slider) hud_width_slider->set_editable(true);
	if (hud_width_spin) hud_width_spin->set_editable(true);
	if (hud_height_slider) hud_height_slider->set_editable(true);
	if (hud_height_spin) hud_height_spin->set_editable(true);
	if (hud_width_spin) hud_width_spin->set_value(int(w.get("width", 160)));
	if (hud_height_slider) hud_height_slider->set_value(int(w.get("height", 36)));
	if (hud_height_spin) hud_height_spin->set_value(int(w.get("height", 36)));
	if (hud_layer_spin) hud_layer_spin->set_value(int(w.get("layer_order", 0)));

	if (hud_window_style_opt) {
		String wsid = String(w.get("window_style_id", "")).strip_edges().to_lower();
		int sel = 0;
		for (int i = 0; i < hud_window_style_opt->get_item_count(); i++) {
			if (String(hud_window_style_opt->get_item_metadata(i)).to_lower() == wsid) {
				sel = i;
				break;
			}
		}
		hud_window_style_opt->select(sel);
	}

	String vmode = String(w.get("visibility_mode", "always"));
	if (hud_vis_mode_opt) hud_vis_mode_opt->select(vmode == "switch" ? 1 : 0);
	if (hud_vis_switch_row) hud_vis_switch_row->set_visible(vmode == "switch");
	String vsw = String(w.get("visibility_switch", ""));
	if (hud_vis_switch_edit) hud_vis_switch_edit->set_text(vsw);
	if (hud_vis_switch_opt) {
		int sel = 0;
		for (int i = 1; i < hud_vis_switch_opt->get_item_count(); i++) {
			if (String(hud_vis_switch_opt->get_item_metadata(i)) == vsw) {
				sel = i;
				break;
			}
		}
		hud_vis_switch_opt->select(sel);
	}
	if (hud_hide_ctx_chk) hud_hide_ctx_chk->set_pressed(bool(w.get("hide_during_messages_and_menus", true)));
	if (hud_pixel_art_chk) hud_pixel_art_chk->set_pressed(bool(w.get("pixel_art", true)));

	if (hud_bar_box) hud_bar_box->set_visible(wtype == "bar");
	if (hud_hearts_box) hud_hearts_box->set_visible(wtype == "hearts");
	if (hud_currency_box) hud_currency_box->set_visible(wtype == "currency");
	if (hud_item_box) hud_item_box->set_visible(wtype == "item");
	if (hud_text_box) hud_text_box->set_visible(wtype == "text");
	if (hud_image_box) hud_image_box->set_visible(wtype == "image");

	auto select_meta = [](OptionButton *p_opt, const String &p_meta) {
		if (!p_opt) return;
		int sel = 0;
		for (int i = 0; i < p_opt->get_item_count(); i++) {
			if (String(p_opt->get_item_metadata(i)) == p_meta) {
				sel = i;
				break;
			}
		}
		p_opt->select(sel);
	};

	if (wtype == "bar") {
		String vs = String(w.get("value_source", "hp"));
		select_meta(hud_bar_source_opt, vs);
		if (hud_bar_var_box) hud_bar_var_box->set_visible(vs == "variable");
		String vname = String(w.get("variable_name", ""));
		if (hud_bar_var_edit) hud_bar_var_edit->set_text(vname);
		select_meta(hud_bar_var_opt, vname);
		if (hud_bar_var_max_spin) hud_bar_var_max_spin->set_value(int(w.get("variable_max", 100)));
		if (hud_bar_bg_col_btn) hud_bar_bg_col_btn->set_pick_color(s_hud_parse_color(String(w.get("bg_color", "#1A1E2ECC")), Color::html("#1A1E2ECC")));
		if (hud_bar_fill_col_btn) hud_bar_fill_col_btn->set_pick_color(s_hud_parse_color(String(w.get("fill_color", "#D93B3BFF")), Color::html("#D93B3BFF")));
		if (hud_bar_border_col_btn) hud_bar_border_col_btn->set_pick_color(s_hud_parse_color(String(w.get("border_color", "#E2E8F0FF")), Color::html("#E2E8F0FF")));
		if (hud_bar_font_col_btn) hud_bar_font_col_btn->set_pick_color(s_hud_parse_color(String(w.get("font_color", "#FFFFFFFF")), Color::html("#FFFFFFFF")));
		if (hud_bar_label_edit) hud_bar_label_edit->set_text(String(w.get("label_text", "")));
		if (hud_bar_show_nums_chk) hud_bar_show_nums_chk->set_pressed(bool(w.get("show_numbers", true)));
		int fsz = int(w.get("font_size", 13));
		if (hud_bar_font_size_slider) hud_bar_font_size_slider->set_value(fsz);
		if (hud_bar_font_size_spin) hud_bar_font_size_spin->set_value(fsz);
		if (hud_bar_bg_img_edit) hud_bar_bg_img_edit->set_text(String(w.get("bg_image_path", "")));
		if (hud_bar_fill_img_edit) hud_bar_fill_img_edit->set_text(String(w.get("fill_image_path", "")));
	} else if (wtype == "hearts") {
		if (hud_hearts_show_bg_chk) hud_hearts_show_bg_chk->set_pressed(bool(w.get("show_background", false)));
		if (hud_hearts_auto_size_chk) {
			bool auto_sz = bool(w.get("auto_size", false));
			hud_hearts_auto_size_chk->set_pressed(auto_sz);
			if (hud_width_slider) hud_width_slider->set_editable(!auto_sz);
			if (hud_width_spin) hud_width_spin->set_editable(!auto_sz);
			if (hud_height_slider) hud_height_slider->set_editable(!auto_sz);
			if (hud_height_spin) hud_height_spin->set_editable(!auto_sz);
		}
		if (hud_hearts_bg_color_btn) hud_hearts_bg_color_btn->set_pick_color(Color::html(String(w.get("bg_color", "#1a1a1a"))));
		if (hud_hearts_bg_opacity_spin) hud_hearts_bg_opacity_spin->set_value(int(w.get("bg_opacity", 60)));
		if (hud_hearts_hp_spin) hud_hearts_hp_spin->set_value(int(w.get("hp_per_heart", 20)));
		int hsz = int(w.get("heart_size", 20));
		int hsp = int(w.get("spacing", 4));
		if (hud_hearts_size_slider) hud_hearts_size_slider->set_value(hsz);
		if (hud_hearts_size_spin) hud_hearts_size_spin->set_value(hsz);
		if (hud_hearts_spacing_slider) hud_hearts_spacing_slider->set_value(hsp);
		if (hud_hearts_spacing_spin) hud_hearts_spacing_spin->set_value(hsp);
		if (hud_hearts_per_row_spin) hud_hearts_per_row_spin->set_value(int(w.get("max_hearts_per_row", 10)));
		if (hud_hearts_max_rows_spin) hud_hearts_max_rows_spin->set_value(int(w.get("max_rows", 2)));
		if (hud_heart_full_edit) hud_heart_full_edit->set_text(String(w.get("full_image_path", "")));
		if (hud_heart_half_edit) hud_heart_half_edit->set_text(String(w.get("half_image_path", "")));
		if (hud_heart_empty_edit) hud_heart_empty_edit->set_text(String(w.get("empty_image_path", "")));
	} else if (wtype == "currency") {
		if (hud_cur_show_icon_chk) hud_cur_show_icon_chk->set_pressed(bool(w.get("show_icon", true)));
		if (hud_cur_show_name_chk) hud_cur_show_name_chk->set_pressed(bool(w.get("show_currency_name", true)));
		if (hud_cur_icon_edit) hud_cur_icon_edit->set_text(String(w.get("icon_path", "")));
		int fsz = int(w.get("font_size", 15));
		if (hud_cur_font_size_slider) hud_cur_font_size_slider->set_value(fsz);
		if (hud_cur_font_size_spin) hud_cur_font_size_spin->set_value(fsz);
		if (hud_cur_font_col_btn) hud_cur_font_col_btn->set_pick_color(s_hud_parse_color(String(w.get("font_color", "#FDE047FF")), Color::html("#FDE047FF")));
	} else if (wtype == "item") {
		select_meta(hud_item_db_opt, String(w.get("item_key", "")));
		if (hud_item_icon_edit) hud_item_icon_edit->set_text(String(w.get("icon_path", "")));
		if (hud_item_hide_zero_chk) hud_item_hide_zero_chk->set_pressed(bool(w.get("hide_when_zero", false)));
		if (hud_item_show_name_chk) hud_item_show_name_chk->set_pressed(bool(w.get("show_name", false)));
		int fsz = int(w.get("font_size", 15));
		if (hud_item_font_size_slider) hud_item_font_size_slider->set_value(fsz);
		if (hud_item_font_size_spin) hud_item_font_size_spin->set_value(fsz);
		if (hud_item_font_col_btn) hud_item_font_col_btn->set_pick_color(s_hud_parse_color(String(w.get("font_color", "#FFFFFFFF")), Color::html("#FFFFFFFF")));
	} else if (wtype == "text") {
		String tm = String(w.get("text_mode", "fixed"));
		select_meta(hud_text_mode_opt, tm);
		if (hud_text_fixed_row) hud_text_fixed_row->set_visible(tm != "variable");
		if (hud_text_var_row) hud_text_var_row->set_visible(tm == "variable");
		if (hud_text_edit) hud_text_edit->set_text(String(w.get("text", "")));
		String tv = String(w.get("variable_name", ""));
		if (hud_text_var_edit) hud_text_var_edit->set_text(tv);
		select_meta(hud_text_var_opt, tv);
		select_meta(hud_text_align_opt, String(w.get("align", "left")));
		int fsz = int(w.get("font_size", 16));
		if (hud_text_font_size_slider) hud_text_font_size_slider->set_value(fsz);
		if (hud_text_font_size_spin) hud_text_font_size_spin->set_value(fsz);
		if (hud_text_font_col_btn) hud_text_font_col_btn->set_pick_color(s_hud_parse_color(String(w.get("font_color", "#FFFFFFFF")), Color::html("#FFFFFFFF")));
	} else {
		if (hud_img_path_edit) hud_img_path_edit->set_text(String(w.get("image_path", "")));
		select_meta(hud_img_scale_opt, String(w.get("scale_mode", "fit")));
		int pct = CLAMP(int(Math::round(double(w.get("opacity", 1.0)) * 100.0)), 0, 100);
		if (hud_img_opacity_slider) hud_img_opacity_slider->set_value(pct);
		if (hud_img_opacity_spin) hud_img_opacity_spin->set_value(pct);
	}

	updating_ui = prev_upd;
}

void ClockGearsDatabaseDialog::_update_hud_live_preview() {
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		if (!hud_preview_has_saved_rt) {
			hud_preview_saved_rt = dm->export_runtime_state();
			hud_preview_has_saved_rt = true;
		}
		int sample_hp = hud_sample_hp_spin ? int(hud_sample_hp_spin->get_value()) : 75;
		int sample_gold = hud_sample_gold_spin ? int(hud_sample_gold_spin->get_value()) : 250;
		int sample_qty = hud_sample_item_qty_spin ? int(hud_sample_item_qty_spin->get_value()) : 3;

		int leader_id = 1;
		Array party = dm->get_party_actor_ids();
		if (!party.is_empty() && int(party[0]) > 0) {
			leader_id = int(party[0]);
		} else {
			Array actors = dm->get_category_array(CAT_ACTORS);
			if (!actors.is_empty() && actors[0].get_type() == Variant::DICTIONARY && int(Dictionary(actors[0]).get("id", 0)) > 0) {
				leader_id = int(Dictionary(actors[0]).get("id", 1));
			}
		}
		dm->get_actor_runtime_state(leader_id);
		dm->set_actor_runtime_hp(leader_id, sample_hp);
		dm->set_gold(sample_gold);

		Array widgets = hud_config_draft.get("widgets", Array());
		for (int i = 0; i < widgets.size(); i++) {
			if (widgets[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary w = widgets[i];
			if (String(w.get("type", "")) != "item") continue;
			String ikey = String(w.get("item_key", "")).strip_edges();
			if (ikey.is_empty()) continue;
			Dictionary info = dm->find_inventory_record(ikey);
			int cur_q = dm->get_item_qty(ikey);
			if (cur_q != sample_qty) {
				dm->change_item(ikey, sample_qty - cur_q);
			}
			if (bool(info.get("found", false))) {
				int cat = int(info.get("category", -1));
				int rid = int(info.get("id", 0));
				if (cat >= 0 && rid > 0) {
					String canon = dm->get_record_inventory_key(cat, rid);
					if (!canon.is_empty() && canon != ikey) {
						int cq2 = dm->get_item_qty(canon);
						if (cq2 != sample_qty) {
							dm->change_item(canon, sample_qty - cq2);
						}
					}
				}
			}
		}
	}

	if (hud_live_preview) {
		hud_live_preview->apply_hud_config(hud_config_draft);
		hud_live_preview->refresh_all_widgets("editor_preview");
		Array pw = hud_config_draft.get("widgets", Array());
		for (int i = 0; i < pw.size(); i++) {
			if (pw[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary w = pw[i];
			if (String(w.get("type", "")) == "item" && bool(w.get("hide_when_zero", false))) {
				String wid = String(w.get("id", ""));
				Control *wnode = hud_live_preview->get_widget_node(wid);
				Dictionary wst = hud_live_preview->get_widget_state(wid);
				if (wnode && int(wst.get("qty", 0)) <= 0) {
					wnode->set_visible(false);
				}
			}
		}
	}
}

void ClockGearsDatabaseDialog::_refresh_hud_editor_ui() {
	if (!hud_editor_panel) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	_refresh_hud_dynamic_options();
	_rebuild_hud_widgets_list();
	_refresh_hud_selected_widget_props();
	updating_ui = prev_upd;
	_update_hud_live_preview();
}

void ClockGearsDatabaseDialog::_hud_set_selected_field(const String &p_key, const Variant &p_val, bool p_rebuild_list) {
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (hud_selected_widget_idx < 0 || hud_selected_widget_idx >= widgets.size() || widgets[hud_selected_widget_idx].get_type() != Variant::DICTIONARY) {
		return;
	}
	Dictionary w = Dictionary(widgets[hud_selected_widget_idx]).duplicate(true);
	w[p_key] = p_val;
	widgets[hud_selected_widget_idx] = ClockGearsDataManager::normalize_hud_widget(w, hud_selected_widget_idx + 1);
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	if (p_rebuild_list) {
		_rebuild_hud_widgets_list();
	}
	_set_pending_changes(true);
	_update_hud_live_preview();
}

void ClockGearsDatabaseDialog::_on_hud_add_pressed() {
	String wtype = "bar";
	if (hud_add_type_opt && hud_add_type_opt->get_selected() >= 0) {
		wtype = String(hud_add_type_opt->get_selected_metadata());
	}
	add_hud_widget(wtype);
}

void ClockGearsDatabaseDialog::_on_hud_remove_pressed() {
	if (hud_selected_widget_idx >= 0) {
		remove_hud_widget(hud_selected_widget_idx);
	}
}

void ClockGearsDatabaseDialog::_on_hud_duplicate_pressed() {
	if (hud_selected_widget_idx >= 0) {
		duplicate_hud_widget(hud_selected_widget_idx);
	}
}

void ClockGearsDatabaseDialog::_on_hud_layer_move_pressed(int p_delta) {
	if (hud_selected_widget_idx >= 0) {
		move_hud_widget_layer(hud_selected_widget_idx, p_delta);
	}
}

void ClockGearsDatabaseDialog::_on_hud_widget_selected(int p_idx) {
	if (updating_ui) {
		return;
	}
	select_hud_widget(p_idx);
}

void ClockGearsDatabaseDialog::_on_hud_anchor_pressed(const String &p_anchor_id) {
	if (updating_ui) {
		return;
	}
	Array widgets = hud_config_draft.get("widgets", Array());
	if (hud_selected_widget_idx >= 0 && hud_selected_widget_idx < widgets.size() && hud_live_preview) {
		// Conserva la posición visible del widget al cambiar entre anclajes y modo libre.
		Dictionary w = widgets[hud_selected_widget_idx];
		Dictionary st = hud_live_preview->get_widget_state(String(w.get("id", "")));
		if (st.has("base_rect")) {
			Rect2 br = st["base_rect"];
			Vector2 af(0, 0);
			if (p_anchor_id.ends_with("_center") || p_anchor_id == "center") af.x = 0.5f;
			if (p_anchor_id.ends_with("_right")) af.x = 1.0f;
			if (p_anchor_id.begins_with("middle_") || p_anchor_id == "center") af.y = 0.5f;
			if (p_anchor_id.begins_with("bottom_")) af.y = 1.0f;
			int nox = int(Math::round(br.position.x - af.x * (960.0f - br.size.x)));
			int noy = int(Math::round(br.position.y - af.y * (540.0f - br.size.y)));
			_hud_set_selected_field("offset_x", nox, false);
			_hud_set_selected_field("offset_y", noy, false);
		}
	}
	_hud_set_selected_field("anchor", p_anchor_id);
	_refresh_hud_selected_widget_props();
}

void ClockGearsDatabaseDialog::_on_hud_preview_gui_input(const Ref<InputEvent> &p_event) {
	if (!hud_live_preview || !hud_preview_drag_overlay || !hud_preview_viewport) {
		return;
	}
	Size2 ov_sz = hud_preview_drag_overlay->get_size();
	if (ov_sz.x < 1.0f || ov_sz.y < 1.0f) {
		return;
	}
	Vector2 vp_sz = Vector2(hud_preview_viewport->get_size());
	Vector2 to_vp(vp_sz.x / ov_sz.x, vp_sz.y / ov_sz.y);
	Dictionary info = hud_live_preview->get_layout_bounds_info();
	float hud_scale = MAX(0.01f, float(info.get("scale", 1.0)));

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		MouseButton bi = mb->get_button_index();
		if (bi == MouseButton::WHEEL_UP || bi == MouseButton::WHEEL_DOWN || bi == MouseButton::WHEEL_LEFT || bi == MouseButton::WHEEL_RIGHT) {
			return; // La rueda no hace nada en la vista previa (sin zoom).
		}
		if (bi != MouseButton::LEFT) {
			return;
		}
		if (!mb->is_pressed()) {
			hud_drag_active = false;
			hud_preview_drag_overlay->accept_event();
			return;
		}
		Vector2 vp_pt = mb->get_position() * to_vp;
		Array rects = info.get("widget_rects", Array());
		String hit_id;
		for (int i = rects.size() - 1; i >= 0; i--) {
			Dictionary row = rects[i];
			if (!bool(row.get("visible", true))) {
				continue;
			}
			Rect2 r = row.get("screen_rect", Rect2());
			if (r.has_point(vp_pt)) {
				hit_id = String(row.get("id", ""));
				break;
			}
		}
		if (hit_id.is_empty()) {
			return;
		}
		Array widgets = hud_config_draft.get("widgets", Array());
		int idx = -1;
		for (int i = 0; i < widgets.size(); i++) {
			if (String(Dictionary(widgets[i]).get("id", "")) == hit_id) {
				idx = i;
				break;
			}
		}
		if (idx < 0) {
			return;
		}
		if (idx != hud_selected_widget_idx) {
			select_hud_widget(idx);
		}
		Dictionary w = Dictionary(Array(hud_config_draft.get("widgets", Array()))[idx]);
		if (String(w.get("anchor", "")) == "free") {
			Dictionary st = hud_live_preview->get_widget_state(hit_id);
			Rect2 br = st.get("base_rect", Rect2());
			hud_drag_active = true;
			hud_drag_start_mouse = mb->get_position();
			hud_drag_start_ox = int(Math::round(br.position.x));
			hud_drag_start_oy = int(Math::round(br.position.y));
			hud_drag_widget_size = br.size;
		}
		hud_preview_drag_overlay->accept_event();
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && hud_drag_active) {
		Vector2 d = (mm->get_position() - hud_drag_start_mouse) * to_vp / hud_scale;
		int max_x = MAX(0, 960 - int(Math::ceil(hud_drag_widget_size.x)));
		int max_y = MAX(0, 540 - int(Math::ceil(hud_drag_widget_size.y)));
		int nx = CLAMP(hud_drag_start_ox + int(Math::round(d.x)), 0, max_x);
		int ny = CLAMP(hud_drag_start_oy + int(Math::round(d.y)), 0, max_y);
		_hud_set_selected_field("offset_x", nx, false);
		_hud_set_selected_field("offset_y", ny);
		bool prev_upd = updating_ui;
		updating_ui = true;
		if (hud_offset_x_spin) hud_offset_x_spin->set_value(nx);
		if (hud_offset_y_spin) hud_offset_y_spin->set_value(ny);
		updating_ui = prev_upd;
		hud_preview_drag_overlay->accept_event();
	}
}

void ClockGearsDatabaseDialog::_on_hud_common_changed() {
	if (updating_ui) {
		return;
	}
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (hud_selected_widget_idx < 0 || hud_selected_widget_idx >= widgets.size()) {
		return;
	}
	Dictionary w = Dictionary(widgets[hud_selected_widget_idx]).duplicate(true);
	if (hud_name_edit) w["name"] = hud_name_edit->get_text();
	if (hud_offset_x_spin) w["offset_x"] = int(hud_offset_x_spin->get_value());
	if (hud_offset_y_spin) w["offset_y"] = int(hud_offset_y_spin->get_value());
	if (hud_width_spin) w["width"] = int(hud_width_spin->get_value());
	if (hud_height_spin) w["height"] = int(hud_height_spin->get_value());
	if (hud_layer_spin) w["layer_order"] = int(hud_layer_spin->get_value());
	if (hud_window_style_opt && hud_window_style_opt->get_selected() >= 0) {
		w["window_style_id"] = String(hud_window_style_opt->get_selected_metadata());
	}
	String vmode = (hud_vis_mode_opt && hud_vis_mode_opt->get_selected() == 1) ? String("switch") : String("always");
	w["visibility_mode"] = vmode;
	if (hud_vis_switch_row) hud_vis_switch_row->set_visible(vmode == "switch");
	if (hud_vis_switch_edit) w["visibility_switch"] = hud_vis_switch_edit->get_text();
	if (hud_hide_ctx_chk) w["hide_during_messages_and_menus"] = hud_hide_ctx_chk->is_pressed();
	if (hud_pixel_art_chk) w["pixel_art"] = hud_pixel_art_chk->is_pressed();

	widgets[hud_selected_widget_idx] = ClockGearsDataManager::normalize_hud_widget(w, hud_selected_widget_idx + 1);
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	if (hud_widgets_list && hud_selected_widget_idx < hud_widgets_list->get_item_count()) {
		hud_widgets_list->set_item_text(hud_selected_widget_idx, _hud_format_widget_list_label(Dictionary(Array(hud_config_draft["widgets"])[hud_selected_widget_idx]), hud_selected_widget_idx));
	}
	_set_pending_changes(true);
	_update_hud_live_preview();
}

void ClockGearsDatabaseDialog::_on_hud_slider_spin_sync(double p_val, const String &p_key, Slider *p_slider, SpinBox *p_spin, bool p_is_pct_opacity) {
	if (updating_ui) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;
	if (p_slider && Math::abs(p_slider->get_value() - p_val) > 0.001) {
		p_slider->set_value(p_val);
	}
	if (p_spin && Math::abs(p_spin->get_value() - p_val) > 0.001) {
		p_spin->set_value(p_val);
	}
	updating_ui = prev_upd;
	Variant stored = p_is_pct_opacity ? Variant(CLAMP(p_val / 100.0, 0.0, 1.0)) : Variant(int(Math::round(p_val)));
	_hud_set_selected_field(p_key, stored, false);
}

void ClockGearsDatabaseDialog::_on_hud_type_field_changed() {
	if (updating_ui) {
		return;
	}
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (hud_selected_widget_idx < 0 || hud_selected_widget_idx >= widgets.size()) {
		return;
	}
	Dictionary w = Dictionary(widgets[hud_selected_widget_idx]).duplicate(true);
	String wtype = String(w.get("type", "bar"));

	if (wtype == "bar") {
		String vs = (hud_bar_source_opt && hud_bar_source_opt->get_selected() >= 0) ? String(hud_bar_source_opt->get_selected_metadata()) : String("hp");
		w["value_source"] = vs;
		if (hud_bar_var_box) hud_bar_var_box->set_visible(vs == "variable");
		if (hud_bar_var_edit) w["variable_name"] = hud_bar_var_edit->get_text();
		if (hud_bar_var_max_spin) w["variable_max"] = int(hud_bar_var_max_spin->get_value());
		if (hud_bar_label_edit) w["label_text"] = hud_bar_label_edit->get_text();
		if (hud_bar_show_nums_chk) w["show_numbers"] = hud_bar_show_nums_chk->is_pressed();
		if (hud_bar_bg_img_edit) w["bg_image_path"] = hud_bar_bg_img_edit->get_text();
		if (hud_bar_fill_img_edit) w["fill_image_path"] = hud_bar_fill_img_edit->get_text();
	} else if (wtype == "hearts") {
		if (hud_hearts_show_bg_chk) w["show_background"] = hud_hearts_show_bg_chk->is_pressed();
		if (hud_hearts_auto_size_chk) {
			bool auto_sz = hud_hearts_auto_size_chk->is_pressed();
			w["auto_size"] = auto_sz;
			if (hud_width_slider) hud_width_slider->set_editable(!auto_sz);
			if (hud_width_spin) hud_width_spin->set_editable(!auto_sz);
			if (hud_height_slider) hud_height_slider->set_editable(!auto_sz);
			if (hud_height_spin) hud_height_spin->set_editable(!auto_sz);
		}
		if (hud_hearts_bg_color_btn) w["bg_color"] = "#" + hud_hearts_bg_color_btn->get_pick_color().to_html(false);
		if (hud_hearts_bg_opacity_spin) w["bg_opacity"] = int(hud_hearts_bg_opacity_spin->get_value());
		if (hud_hearts_hp_spin) w["hp_per_heart"] = int(hud_hearts_hp_spin->get_value());
		if (hud_hearts_per_row_spin) w["max_hearts_per_row"] = int(hud_hearts_per_row_spin->get_value());
		if (hud_hearts_max_rows_spin) w["max_rows"] = int(hud_hearts_max_rows_spin->get_value());
		if (hud_heart_full_edit) w["full_image_path"] = hud_heart_full_edit->get_text();
		if (hud_heart_half_edit) w["half_image_path"] = hud_heart_half_edit->get_text();
		if (hud_heart_empty_edit) w["empty_image_path"] = hud_heart_empty_edit->get_text();
	} else if (wtype == "currency") {
		if (hud_cur_show_icon_chk) w["show_icon"] = hud_cur_show_icon_chk->is_pressed();
		if (hud_cur_show_name_chk) w["show_currency_name"] = hud_cur_show_name_chk->is_pressed();
		if (hud_cur_icon_edit) w["icon_path"] = hud_cur_icon_edit->get_text();
	} else if (wtype == "item") {
		if (hud_item_db_opt && hud_item_db_opt->get_selected() >= 0) {
			w["item_key"] = String(hud_item_db_opt->get_selected_metadata());
		}
		if (hud_item_icon_edit) w["icon_path"] = hud_item_icon_edit->get_text();
		if (hud_item_hide_zero_chk) w["hide_when_zero"] = hud_item_hide_zero_chk->is_pressed();
		if (hud_item_show_name_chk) w["show_name"] = hud_item_show_name_chk->is_pressed();
	} else if (wtype == "text") {
		String tm = (hud_text_mode_opt && hud_text_mode_opt->get_selected() >= 0) ? String(hud_text_mode_opt->get_selected_metadata()) : String("fixed");
		w["text_mode"] = tm;
		if (hud_text_fixed_row) hud_text_fixed_row->set_visible(tm != "variable");
		if (hud_text_var_row) hud_text_var_row->set_visible(tm == "variable");
		if (hud_text_edit) w["text"] = hud_text_edit->get_text();
		if (hud_text_var_edit) w["variable_name"] = hud_text_var_edit->get_text();
		if (hud_text_align_opt && hud_text_align_opt->get_selected() >= 0) {
			w["align"] = String(hud_text_align_opt->get_selected_metadata());
		}
	} else {
		if (hud_img_path_edit) w["image_path"] = hud_img_path_edit->get_text();
		if (hud_img_scale_opt && hud_img_scale_opt->get_selected() >= 0) {
			w["scale_mode"] = String(hud_img_scale_opt->get_selected_metadata());
		}
	}

	widgets[hud_selected_widget_idx] = ClockGearsDataManager::normalize_hud_widget(w, hud_selected_widget_idx + 1);
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	_set_pending_changes(true);
	_update_hud_live_preview();
}

void ClockGearsDatabaseDialog::_on_hud_type_color_changed(const Color &p_col, const String &p_key) {
	if (updating_ui) {
		return;
	}
	_hud_set_selected_field(p_key, "#" + p_col.to_html(true));
}

void ClockGearsDatabaseDialog::_on_hud_switch_opt_selected(int p_idx) {
	if (updating_ui || !hud_vis_switch_opt || !hud_vis_switch_edit || p_idx < 0) {
		return;
	}
	String sw = String(hud_vis_switch_opt->get_item_metadata(p_idx));
	if (!sw.is_empty()) {
		hud_vis_switch_edit->set_text(sw);
		_on_hud_common_changed();
	}
}

void ClockGearsDatabaseDialog::_on_hud_var_opt_selected(int p_idx, OptionButton *p_opt, LineEdit *p_edit) {
	if (updating_ui || !p_opt || !p_edit || p_idx < 0) {
		return;
	}
	String vn = String(p_opt->get_item_metadata(p_idx));
	if (!vn.is_empty()) {
		p_edit->set_text(vn);
		_on_hud_type_field_changed();
	}
}

void ClockGearsDatabaseDialog::_on_hud_pick_asset_pressed(const String &p_key) {
	hud_asset_target_key = p_key;
	if (!hud_asset_file_dialog) {
		return;
	}
	String proj_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path("res://") : "";
	if (!proj_dir.is_empty()) {
		hud_asset_file_dialog->set_current_dir(proj_dir);
	}
	hud_asset_file_dialog->popup_centered_ratio(0.55f);
}

void ClockGearsDatabaseDialog::_on_hud_clear_asset_pressed(const String &p_key) {
	_hud_set_selected_field(p_key, String(""));
	_refresh_hud_selected_widget_props();
}

void ClockGearsDatabaseDialog::_on_hud_asset_file_selected(const String &p_path) {
	if (hud_asset_target_key.is_empty()) {
		return;
	}
	bool pixel = hud_pixel_art_chk ? hud_pixel_art_chk->is_pressed() : true;
	String final_path = _import_menu_asset_file(p_path, "img/hud", pixel);
	if (final_path.is_empty()) {
		return;
	}
	_hud_set_selected_field(hud_asset_target_key, final_path);
	_refresh_hud_selected_widget_props();
}

void ClockGearsDatabaseDialog::_on_hud_preview_res_selected(int p_idx) {
	if (!hud_preview_res_opt || p_idx < 0) {
		return;
	}
	Vector2i res = hud_preview_res_opt->get_item_metadata(p_idx);
	if (res.x >= 320 && res.y >= 180) {
		set_hud_preview_resolution(res.x, res.y);
	}
}

void ClockGearsDatabaseDialog::_on_hud_sample_value_changed(double p_unused) {
	if (updating_ui) {
		return;
	}
	_update_hud_live_preview();
}

Dictionary ClockGearsDatabaseDialog::get_hud_draft_copy() const {
	return hud_config_draft.duplicate(true);
}

int ClockGearsDatabaseDialog::add_hud_widget(const String &p_type, const Dictionary &p_fields) {
	Dictionary nw = ClockGearsDataManager::make_default_hud_widget(p_type);
	Array fkeys = p_fields.keys();
	for (int i = 0; i < fkeys.size(); i++) {
		nw[fkeys[i]] = p_fields[fkeys[i]];
	}
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	int insert_idx = widgets.size();
	if (!p_fields.has("layer_order")) {
		int max_lo = -1;
		for (int i = 0; i < widgets.size(); i++) {
			if (widgets[i].get_type() == Variant::DICTIONARY) {
				max_lo = MAX(max_lo, int(Dictionary(widgets[i]).get("layer_order", 0)));
			}
		}
		nw["layer_order"] = max_lo >= 0 ? (max_lo + 1) : 0;
	}
	widgets.push_back(ClockGearsDataManager::normalize_hud_widget(nw, insert_idx + 1));
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	hud_selected_widget_idx = Array(hud_config_draft["widgets"]).size() - 1;
	_set_pending_changes(true);
	_refresh_hud_editor_ui();
	return hud_selected_widget_idx;
}

bool ClockGearsDatabaseDialog::remove_hud_widget(int p_index) {
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (p_index < 0 || p_index >= widgets.size()) {
		return false;
	}
	widgets.remove_at(p_index);
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	int rem = Array(hud_config_draft["widgets"]).size();
	hud_selected_widget_idx = (rem > 0) ? MIN(p_index, rem - 1) : -1;
	_set_pending_changes(true);
	_refresh_hud_editor_ui();
	return true;
}

int ClockGearsDatabaseDialog::duplicate_hud_widget(int p_index) {
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (p_index < 0 || p_index >= widgets.size() || widgets[p_index].get_type() != Variant::DICTIONARY) {
		return -1;
	}
	Dictionary copy = Dictionary(widgets[p_index]).duplicate(true);
	copy["id"] = "";
	copy["name"] = String(copy.get("name", "Widget")) + " (Copia)";
	copy["offset_x"] = CLAMP(int(copy.get("offset_x", 0)) + 16, -960, 960);
	copy["offset_y"] = CLAMP(int(copy.get("offset_y", 0)) + 16, -540, 540);
	widgets.insert(p_index + 1, ClockGearsDataManager::normalize_hud_widget(copy, widgets.size() + 1));
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	hud_selected_widget_idx = MIN(p_index + 1, Array(hud_config_draft["widgets"]).size() - 1);
	_set_pending_changes(true);
	_refresh_hud_editor_ui();
	return hud_selected_widget_idx;
}

bool ClockGearsDatabaseDialog::move_hud_widget_layer(int p_index, int p_delta) {
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (p_index < 0 || p_index >= widgets.size() || p_delta == 0) {
		return false;
	}
	int step = (p_delta > 0) ? 1 : -1;
	int target = CLAMP(p_index + step, 0, widgets.size() - 1);
	Dictionary cur = Dictionary(widgets[p_index]).duplicate(true);
	int cur_lo = int(cur.get("layer_order", 0));

	if (target != p_index) {
		Dictionary other = Dictionary(widgets[target]).duplicate(true);
		int other_lo = int(other.get("layer_order", 0));
		if (step > 0) {
			cur["layer_order"] = CLAMP(MAX(cur_lo + 1, other_lo + 1), -100, 1000);
		} else {
			cur["layer_order"] = CLAMP(MIN(cur_lo - 1, other_lo - 1), -100, 1000);
		}
		widgets[p_index] = other;
		widgets[target] = cur;
		hud_selected_widget_idx = target;
	} else {
		cur["layer_order"] = CLAMP(cur_lo + step, -100, 1000);
		widgets[p_index] = cur;
	}

	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	_set_pending_changes(true);
	_refresh_hud_editor_ui();
	return true;
}

void ClockGearsDatabaseDialog::select_hud_widget(int p_index) {
	Array widgets = hud_config_draft.get("widgets", Array());
	if (p_index >= 0 && p_index < widgets.size()) {
		hud_selected_widget_idx = p_index;
	} else {
		hud_selected_widget_idx = widgets.is_empty() ? -1 : 0;
	}
	if (hud_widgets_list && hud_selected_widget_idx >= 0 && hud_selected_widget_idx < hud_widgets_list->get_item_count()) {
		hud_widgets_list->select(hud_selected_widget_idx);
	}
	if (hud_del_btn) hud_del_btn->set_disabled(hud_selected_widget_idx < 0);
	if (hud_dup_btn) hud_dup_btn->set_disabled(hud_selected_widget_idx < 0);
	if (hud_layer_down_btn) hud_layer_down_btn->set_disabled(hud_selected_widget_idx < 0);
	if (hud_layer_up_btn) hud_layer_up_btn->set_disabled(hud_selected_widget_idx < 0);
	_refresh_hud_selected_widget_props();
}

bool ClockGearsDatabaseDialog::set_hud_widget_field(int p_index, const String &p_key, const Variant &p_val) {
	Array widgets = Array(hud_config_draft.get("widgets", Array())).duplicate(true);
	if (p_index < 0 || p_index >= widgets.size() || widgets[p_index].get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary w = Dictionary(widgets[p_index]).duplicate(true);
	w[p_key] = p_val;
	widgets[p_index] = ClockGearsDataManager::normalize_hud_widget(w, p_index + 1);
	hud_config_draft["widgets"] = widgets;
	hud_config_draft = ClockGearsDataManager::normalize_hud_config(hud_config_draft);
	if (p_index == hud_selected_widget_idx) {
		_refresh_hud_selected_widget_props();
	}
	_rebuild_hud_widgets_list();
	_set_pending_changes(true);
	_update_hud_live_preview();
	return true;
}

void ClockGearsDatabaseDialog::set_hud_preview_resolution(int p_width, int p_height) {
	set_menus_preview_resolution(p_width, p_height);
}

void ClockGearsDatabaseDialog::set_hud_preview_sample_values(int p_hp, int p_gold, int p_item_qty) {
	bool prev_upd = updating_ui;
	updating_ui = true;
	if (hud_sample_hp_spin) hud_sample_hp_spin->set_value(MAX(0, p_hp));
	if (hud_sample_gold_spin) hud_sample_gold_spin->set_value(MAX(0, p_gold));
	if (hud_sample_item_qty_spin) hud_sample_item_qty_spin->set_value(MAX(0, p_item_qty));
	updating_ui = prev_upd;
	_update_hud_live_preview();
}

Dictionary ClockGearsDatabaseDialog::get_hud_editor_ui_handles() {
	_setup_hud_editor_ui();
	Dictionary out;
	out["panel"] = hud_editor_panel;
	out["main_split"] = hud_main_split;
	out["add_type_opt"] = hud_add_type_opt;
	out["add_btn"] = hud_add_btn;
	out["del_btn"] = hud_del_btn;
	out["dup_btn"] = hud_dup_btn;
	out["layer_up_btn"] = hud_layer_up_btn;
	out["layer_down_btn"] = hud_layer_down_btn;
	out["widgets_list"] = hud_widgets_list;
	out["name_edit"] = hud_name_edit;
	Array ab;
	for (int i = 0; i < 9; i++) {
		ab.push_back(hud_anchor_btns[i]);
	}
	out["anchor_btns"] = ab;
	out["free_anchor_btn"] = hud_free_anchor_btn;
	out["preview_drag_overlay"] = hud_preview_drag_overlay;
	out["offset_x_spin"] = hud_offset_x_spin;
	out["offset_y_spin"] = hud_offset_y_spin;
	out["width_slider"] = hud_width_slider;
	out["width_spin"] = hud_width_spin;
	out["height_slider"] = hud_height_slider;
	out["height_spin"] = hud_height_spin;
	out["layer_spin"] = hud_layer_spin;
	out["window_style_opt"] = hud_window_style_opt;
	out["vis_mode_opt"] = hud_vis_mode_opt;
	out["vis_switch_opt"] = hud_vis_switch_opt;
	out["vis_switch_edit"] = hud_vis_switch_edit;
	out["hide_ctx_chk"] = hud_hide_ctx_chk;
	out["pixel_art_chk"] = hud_pixel_art_chk;
	out["bar_source_opt"] = hud_bar_source_opt;
	out["bar_var_opt"] = hud_bar_var_opt;
	out["bar_var_edit"] = hud_bar_var_edit;
	out["bar_var_max_spin"] = hud_bar_var_max_spin;
	out["bar_bg_col_btn"] = hud_bar_bg_col_btn;
	out["bar_fill_col_btn"] = hud_bar_fill_col_btn;
	out["bar_border_col_btn"] = hud_bar_border_col_btn;
	out["bar_font_col_btn"] = hud_bar_font_col_btn;
	out["bar_bg_img_edit"] = hud_bar_bg_img_edit;
	out["bar_fill_img_edit"] = hud_bar_fill_img_edit;
	out["bar_label_edit"] = hud_bar_label_edit;
	out["bar_show_nums_chk"] = hud_bar_show_nums_chk;
	out["hearts_hp_spin"] = hud_hearts_hp_spin;
	out["hearts_size_slider"] = hud_hearts_size_slider;
	out["hearts_size_spin"] = hud_hearts_size_spin;
	out["hearts_spacing_slider"] = hud_hearts_spacing_slider;
	out["hearts_spacing_spin"] = hud_hearts_spacing_spin;
	out["hearts_per_row_spin"] = hud_hearts_per_row_spin;
	out["hearts_max_rows_spin"] = hud_hearts_max_rows_spin;
	out["heart_full_edit"] = hud_heart_full_edit;
	out["heart_half_edit"] = hud_heart_half_edit;
	out["heart_empty_edit"] = hud_heart_empty_edit;
	out["cur_show_icon_chk"] = hud_cur_show_icon_chk;
	out["cur_show_name_chk"] = hud_cur_show_name_chk;
	out["cur_icon_edit"] = hud_cur_icon_edit;
	out["item_db_opt"] = hud_item_db_opt;
	out["item_icon_edit"] = hud_item_icon_edit;
	out["item_hide_zero_chk"] = hud_item_hide_zero_chk;
	out["item_show_name_chk"] = hud_item_show_name_chk;
	out["text_mode_opt"] = hud_text_mode_opt;
	out["text_edit"] = hud_text_edit;
	out["text_var_opt"] = hud_text_var_opt;
	out["text_var_edit"] = hud_text_var_edit;
	out["text_align_opt"] = hud_text_align_opt;
	out["img_path_edit"] = hud_img_path_edit;
	out["img_scale_opt"] = hud_img_scale_opt;
	out["img_opacity_slider"] = hud_img_opacity_slider;
	out["img_opacity_spin"] = hud_img_opacity_spin;
	out["preview_res_opt"] = hud_preview_res_opt;
	out["preview_viewport"] = hud_preview_viewport;
	out["preview_map_bg"] = hud_preview_map_bg;
	out["live_preview"] = hud_live_preview;
	out["sample_hp_spin"] = hud_sample_hp_spin;
	out["sample_gold_spin"] = hud_sample_gold_spin;
	out["sample_item_qty_spin"] = hud_sample_item_qty_spin;
	out["selected_widget_idx"] = hud_selected_widget_idx;
	return out;
}

void ClockGearsDatabaseDialog::_setup_ui_design_editor_ui() {
	if (ui_design_editor_panel || !col2_box) {
		return;
	}
	ui_design_editor_panel = memnew(ClockGearsUiDesignEditor);
	ui_design_editor_panel->set_owner_database(this);
	ui_design_editor_panel->hide();
	ui_design_editor_panel->connect("design_modified", callable_mp(this, &ClockGearsDatabaseDialog::_on_ui_designer_modified));
	col2_box->add_child(ui_design_editor_panel);
	if (menus_common_preview_res != Vector2i(960, 540)) {
		ui_design_editor_panel->set_preview_resolution(menus_common_preview_res.x, menus_common_preview_res.y, false);
	}
}

void ClockGearsDatabaseDialog::_on_ui_designer_modified() {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		hud_config_draft = ClockGearsDataManager::normalize_hud_config(dm->get_hud_config());
	}
	ui_designer_hud_synced = true;
	_set_pending_changes(true);
}

String ClockGearsDatabaseDialog::import_menu_asset_for_ui_designer(const String &p_src_path, bool p_pixel_preset) {
	return _import_menu_asset_file(p_src_path, "img/ui", p_pixel_preset);
}

Dictionary ClockGearsDatabaseDialog::get_ui_designer_ui_handles() {
	_setup_ui_design_editor_ui();
	if (ui_design_editor_panel) {
		return ui_design_editor_panel->get_ui_handles();
	}
	return Dictionary();
}

// ============================================================================
// Menús -> Game Over (9B Punto 8)
// ============================================================================

void ClockGearsDatabaseDialog::_setup_game_over_editor_ui() {
	if (game_over_editor_panel || !col2_box) {
		return;
	}
	game_over_config_draft = ClockGearsDataManager::get_default_game_over_config();
	if (ClockGearsDataManager::get_singleton()) {
		game_over_config_draft = ClockGearsDataManager::normalize_game_over_config(ClockGearsDataManager::get_singleton()->get_game_over_config());
	}

	game_over_editor_panel = memnew(VBoxContainer);
	game_over_editor_panel->set_name("GameOverEditorPanel");
	game_over_editor_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	game_over_editor_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	game_over_editor_panel->add_theme_constant_override("separation", int(12 * EDSCALE));
	game_over_editor_panel->hide();
	col2_box->add_child(game_over_editor_panel);

	go_main_split = memnew(BoxContainer);
	go_main_split->set_vertical(false);
	go_main_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_main_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	go_main_split->add_theme_constant_override("separation", int(12 * EDSCALE));
	game_over_editor_panel->add_child(go_main_split);

	go_left_col = memnew(VBoxContainer);
	go_left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_left_col->set_stretch_ratio(1.1f);
	go_left_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	go_main_split->add_child(go_left_col);

	go_right_col = memnew(VBoxContainer);
	go_right_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	go_right_col->set_stretch_ratio(1.0f);
	go_right_col->add_theme_constant_override("separation", int(10 * EDSCALE));
	go_main_split->add_child(go_right_col);

	VBoxContainer *left_col = go_left_col;
	VBoxContainer *right_col = go_right_col;

	int row_h = int(28 * EDSCALE);

	auto make_card = [&](Control *p_parent, const String &p_title, VBoxContainer *&r_body) {
		_make_collapsible_menu_card(p_parent, p_title, r_body, true);
	};

	auto add_grid_label = [&](GridContainer *p_grid, const String &p_txt) {
		Label *l = memnew(Label(p_txt));
		l->set_custom_minimum_size(Size2(110 * EDSCALE, 0));
		p_grid->add_child(l);
	};

	// Card 1: Fondo
	VBoxContainer *bg_body = nullptr;
	make_card(left_col, CG_DB_TR("Fondo de Game Over"), bg_body);
	GridContainer *bg_grid = memnew(GridContainer);
	bg_grid->set_columns(2);
	bg_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_grid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	bg_grid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	bg_body->add_child(bg_grid);

	add_grid_label(bg_grid, CG_DB_TR("Tipo de fondo:"));
	go_bg_type_opt = memnew(OptionButton);
	go_bg_type_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_bg_type_opt->add_item(CG_DB_TR("Color sólido"), 0);
	go_bg_type_opt->set_item_metadata(0, String("color"));
	go_bg_type_opt->add_item(CG_DB_TR("Imagen"), 1);
	go_bg_type_opt->set_item_metadata(1, String("image"));
	go_bg_type_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	bg_grid->add_child(go_bg_type_opt);

	add_grid_label(bg_grid, CG_DB_TR("Color de fondo:"));
	go_bg_color_btn = memnew(ColorPickerButton);
	go_bg_color_btn->set_edit_alpha(false);
	go_bg_color_btn->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	go_bg_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	bg_grid->add_child(go_bg_color_btn);

	add_grid_label(bg_grid, CG_DB_TR("Imagen de fondo:"));
	HFlowContainer *img_hb = memnew(HFlowContainer);
	img_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	img_hb->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	img_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	bg_grid->add_child(img_hb);
	go_bg_img_edit = memnew(LineEdit);
	go_bg_img_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_bg_img_edit->set_placeholder("res://img/...");
	go_bg_img_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	img_hb->add_child(go_bg_img_edit);
	Button *btn_pick_bg = memnew(Button(CG_DB_TR("Elegir...")));
	btn_pick_bg->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_pick_asset_pressed).bind(0));
	img_hb->add_child(btn_pick_bg);
	Button *btn_clr_bg = memnew(Button(CG_DB_TR("Limpiar")));
	btn_clr_bg->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_clear_asset_pressed).bind(0));
	img_hb->add_child(btn_clr_bg);

	add_grid_label(bg_grid, CG_DB_TR("Modo de imagen:"));
	go_bg_mode_opt = memnew(OptionButton);
	go_bg_mode_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_bg_mode_opt->add_item(CG_DB_TR("Cubrir (Cover)"), 0);
	go_bg_mode_opt->set_item_metadata(0, String("cover"));
	go_bg_mode_opt->add_item(CG_DB_TR("Contener (Contain)"), 1);
	go_bg_mode_opt->set_item_metadata(1, String("contain"));
	go_bg_mode_opt->add_item(CG_DB_TR("Estirar (Stretch)"), 2);
	go_bg_mode_opt->set_item_metadata(2, String("stretch"));
	go_bg_mode_opt->add_item(CG_DB_TR("Centrar"), 3);
	go_bg_mode_opt->set_item_metadata(3, String("center"));
	go_bg_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	bg_grid->add_child(go_bg_mode_opt);

	add_grid_label(bg_grid, CG_DB_TR("Filtro:"));
	go_bg_filter_opt = memnew(OptionButton);
	go_bg_filter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_bg_filter_opt->add_item(CG_DB_TR("Pixel Art (Nearest)"), 0);
	go_bg_filter_opt->set_item_metadata(0, String("nearest"));
	go_bg_filter_opt->add_item(CG_DB_TR("Suave (Linear)"), 1);
	go_bg_filter_opt->set_item_metadata(1, String("linear"));
	go_bg_filter_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	bg_grid->add_child(go_bg_filter_opt);

	// Card 2: Título, Música y Estilo
	VBoxContainer *cfg_body = nullptr;
	make_card(left_col, CG_DB_TR("Título, música y botones"), cfg_body);
	GridContainer *cgrid = memnew(GridContainer);
	cgrid->set_columns(2);
	cgrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cgrid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	cgrid->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	cfg_body->add_child(cgrid);

	add_grid_label(cgrid, CG_DB_TR("Texto del título:"));
	go_title_text_edit = memnew(LineEdit);
	go_title_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_title_text_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	cgrid->add_child(go_title_text_edit);

	add_grid_label(cgrid, CG_DB_TR("Color del título:"));
	go_title_color_btn = memnew(ColorPickerButton);
	go_title_color_btn->set_edit_alpha(false);
	go_title_color_btn->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	go_title_color_btn->connect("color_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	cgrid->add_child(go_title_color_btn);

	add_grid_label(cgrid, CG_DB_TR("Tamaño del título:"));
	go_title_size_spin = memnew(SpinBox);
	go_title_size_spin->set_min(16);
	go_title_size_spin->set_max(128);
	go_title_size_spin->set_suffix(" px");
	go_title_size_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	cgrid->add_child(go_title_size_spin);

	add_grid_label(cgrid, CG_DB_TR("Música opcional:"));
	HFlowContainer *bgm_hb = memnew(HFlowContainer);
	bgm_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bgm_hb->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	bgm_hb->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	cgrid->add_child(bgm_hb);
	go_bgm_edit = memnew(LineEdit);
	go_bgm_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_bgm_edit->set_placeholder(CG_DB_TR("(Opcional) res://audio/..."));
	go_bgm_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	bgm_hb->add_child(go_bgm_edit);
	Button *btn_pick_bgm = memnew(Button(CG_DB_TR("Elegir...")));
	btn_pick_bgm->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_pick_asset_pressed).bind(1));
	bgm_hb->add_child(btn_pick_bgm);
	Button *btn_clr_bgm = memnew(Button(CG_DB_TR("Limpiar")));
	btn_clr_bgm->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_clear_asset_pressed).bind(1));
	bgm_hb->add_child(btn_clr_bgm);

	add_grid_label(cgrid, CG_DB_TR("Botón Reintentar:"));
	HBoxContainer *retry_hb = memnew(HBoxContainer);
	retry_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	retry_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	cgrid->add_child(retry_hb);
	go_retry_enabled_chk = memnew(CheckBox(CG_DB_TR("Activo")));
	go_retry_enabled_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	retry_hb->add_child(go_retry_enabled_chk);
	go_retry_label_edit = memnew(LineEdit);
	go_retry_label_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_retry_label_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	retry_hb->add_child(go_retry_label_edit);

	add_grid_label(cgrid, CG_DB_TR("Botón Volver al menú:"));
	HBoxContainer *title_hb = memnew(HBoxContainer);
	title_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_hb->add_theme_constant_override("separation", int(8 * EDSCALE));
	cgrid->add_child(title_hb);
	go_title_enabled_chk = memnew(CheckBox(CG_DB_TR("Activo")));
	go_title_enabled_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	title_hb->add_child(go_title_enabled_chk);
	go_title_label_edit = memnew(LineEdit);
	go_title_label_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_title_label_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	title_hb->add_child(go_title_label_edit);

	add_grid_label(cgrid, CG_DB_TR("Estilo de ventana:"));
	go_window_style_opt = memnew(OptionButton);
	go_window_style_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	go_window_style_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_field_changed).unbind(1));
	cgrid->add_child(go_window_style_opt);

	// Columna derecha: Vista previa 16:9
	VBoxContainer *prev_body = nullptr;
	make_card(right_col, CG_DB_TR("Vista previa en vivo (16:9)"), prev_body);
	_build_16_9_viewport_preview(prev_body, go_preview_viewport, go_prev_title_lbl, go_preview_res_opt);
	_sync_common_preview_res_options();

	go_live_preview = memnew(ClockGearsGameOverScreen);
	go_live_preview->set_preview_mode(true);
	go_preview_viewport->add_child(go_live_preview);

	go_asset_file_dialog = memnew(FileDialog);
	go_asset_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	go_asset_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	go_asset_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_game_over_asset_selected));
	game_over_editor_panel->add_child(go_asset_file_dialog);
}

void ClockGearsDatabaseDialog::_refresh_game_over_editor_ui() {
	if (!game_over_editor_panel) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;
	game_over_config_draft = ClockGearsDataManager::normalize_game_over_config(game_over_config_draft);
	Dictionary bg = game_over_config_draft.get("background", Dictionary());

	auto sel_meta = [](OptionButton *p_opt, const String &p_meta) {
		if (!p_opt) return;
		for (int i = 0; i < p_opt->get_item_count(); i++) {
			if (String(p_opt->get_item_metadata(i)).to_lower() == p_meta.to_lower()) {
				p_opt->select(i);
				return;
			}
		}
		if (p_opt->get_item_count() > 0) p_opt->select(0);
	};

	sel_meta(go_bg_type_opt, String(bg.get("type", "color")));
	if (go_bg_color_btn) go_bg_color_btn->set_pick_color(Color::html(String(bg.get("color", "#0c070cff"))));
	if (go_bg_img_edit) go_bg_img_edit->set_text(String(bg.get("image_path", "")));
	sel_meta(go_bg_mode_opt, String(bg.get("image_mode", "cover")));
	sel_meta(go_bg_filter_opt, String(bg.get("filter", "nearest")));

	if (go_title_text_edit) go_title_text_edit->set_text(String(game_over_config_draft.get("title_text", "GAME OVER")));
	if (go_title_color_btn) go_title_color_btn->set_pick_color(Color::html(String(game_over_config_draft.get("title_color", "#e84a5fff"))));
	if (go_title_size_spin) go_title_size_spin->set_value(int(game_over_config_draft.get("title_font_size", 48)));
	if (go_bgm_edit) go_bgm_edit->set_text(String(game_over_config_draft.get("bgm_path", "")));
	if (go_retry_enabled_chk) go_retry_enabled_chk->set_pressed(bool(game_over_config_draft.get("retry_enabled", true)));
	if (go_retry_label_edit) go_retry_label_edit->set_text(String(game_over_config_draft.get("retry_label", "Reintentar")));
	if (go_title_enabled_chk) go_title_enabled_chk->set_pressed(bool(game_over_config_draft.get("title_enabled", true)));
	if (go_title_label_edit) go_title_label_edit->set_text(String(game_over_config_draft.get("title_label", CG_DB_TR("Volver al menú"))));

	if (go_window_style_opt) {
		go_window_style_opt->clear();
		go_window_style_opt->add_item(CG_DB_TR("(Por defecto del proyecto)"), 0);
		go_window_style_opt->set_item_metadata(0, String("default"));
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Array styles = dm->get_window_styles();
			for (int i = 0; i < styles.size(); i++) {
				if (styles[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary st = styles[i];
				String sid = String(st.get("id", ""));
				String sname = String(st.get("name", sid));
				int idx = go_window_style_opt->get_item_count();
				go_window_style_opt->add_item(sname, idx);
				go_window_style_opt->set_item_metadata(idx, sid);
			}
		}
		sel_meta(go_window_style_opt, String(game_over_config_draft.get("window_style_id", "default")));
	}

	updating_ui = prev_upd;
	_update_game_over_live_preview();
}

void ClockGearsDatabaseDialog::_update_game_over_live_preview() {
	if (go_live_preview) {
		go_live_preview->apply_preview_config(game_over_config_draft);
	}
}

void ClockGearsDatabaseDialog::_on_game_over_field_changed() {
	if (updating_ui) {
		return;
	}
	Dictionary bg = Dictionary(game_over_config_draft.get("background", Dictionary())).duplicate(true);
	if (go_bg_type_opt && go_bg_type_opt->get_selected() >= 0) {
		bg["type"] = String(go_bg_type_opt->get_selected_metadata());
	}
	if (go_bg_color_btn) {
		bg["color"] = "#" + go_bg_color_btn->get_pick_color().to_html(false);
	}
	if (go_bg_img_edit) {
		bg["image_path"] = go_bg_img_edit->get_text();
	}
	if (go_bg_mode_opt && go_bg_mode_opt->get_selected() >= 0) {
		bg["image_mode"] = String(go_bg_mode_opt->get_selected_metadata());
	}
	if (go_bg_filter_opt && go_bg_filter_opt->get_selected() >= 0) {
		bg["filter"] = String(go_bg_filter_opt->get_selected_metadata());
	}
	game_over_config_draft["background"] = bg;

	if (go_title_text_edit) game_over_config_draft["title_text"] = go_title_text_edit->get_text();
	if (go_title_color_btn) game_over_config_draft["title_color"] = "#" + go_title_color_btn->get_pick_color().to_html(false);
	if (go_title_size_spin) game_over_config_draft["title_font_size"] = int(go_title_size_spin->get_value());
	if (go_bgm_edit) game_over_config_draft["bgm_path"] = go_bgm_edit->get_text();
	if (go_retry_enabled_chk) game_over_config_draft["retry_enabled"] = go_retry_enabled_chk->is_pressed();
	if (go_retry_label_edit) game_over_config_draft["retry_label"] = go_retry_label_edit->get_text();
	if (go_title_enabled_chk) game_over_config_draft["title_enabled"] = go_title_enabled_chk->is_pressed();
	if (go_title_label_edit) game_over_config_draft["title_label"] = go_title_label_edit->get_text();
	if (go_window_style_opt && go_window_style_opt->get_selected() >= 0) {
		game_over_config_draft["window_style_id"] = String(go_window_style_opt->get_selected_metadata());
	}

	game_over_config_draft = ClockGearsDataManager::normalize_game_over_config(game_over_config_draft);
	_set_pending_changes(true);
	_update_game_over_live_preview();
}

void ClockGearsDatabaseDialog::_on_game_over_pick_asset_pressed(int p_target) {
	if (!go_asset_file_dialog) {
		return;
	}
	go_asset_pick_target = p_target;
	go_asset_file_dialog->clear_filters();
	if (p_target == 0) {
		go_asset_file_dialog->add_filter("*.png, *.jpg, *.jpeg, *.webp ; Imagenes");
	} else {
		go_asset_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Audio");
	}
	go_asset_file_dialog->popup_centered_ratio(0.6f);
}

void ClockGearsDatabaseDialog::_on_game_over_clear_asset_pressed(int p_target) {
	if (p_target == 0 && go_bg_img_edit) {
		go_bg_img_edit->set_text("");
	} else if (p_target == 1 && go_bgm_edit) {
		go_bgm_edit->set_text("");
	}
	_on_game_over_field_changed();
}

void ClockGearsDatabaseDialog::_on_game_over_asset_selected(const String &p_path) {
	if (go_asset_pick_target == 0) {
		String imported = _import_menu_asset_file(p_path, "img/ui", true);
		if (go_bg_img_edit) go_bg_img_edit->set_text(imported);
	} else {
		String imported = _import_menu_asset_file(p_path, "audio/bgm", false);
		if (go_bgm_edit) go_bgm_edit->set_text(imported);
	}
	_on_game_over_field_changed();
}

Dictionary ClockGearsDatabaseDialog::get_game_over_editor_ui_handles() {
	_setup_game_over_editor_ui();
	Dictionary out;
	out["panel"] = game_over_editor_panel;
	out["bg_type_opt"] = go_bg_type_opt;
	out["bg_color_btn"] = go_bg_color_btn;
	out["bg_img_edit"] = go_bg_img_edit;
	out["bg_mode_opt"] = go_bg_mode_opt;
	out["bg_filter_opt"] = go_bg_filter_opt;
	out["title_text_edit"] = go_title_text_edit;
	out["title_color_btn"] = go_title_color_btn;
	out["title_size_spin"] = go_title_size_spin;
	out["bgm_edit"] = go_bgm_edit;
	out["retry_enabled_chk"] = go_retry_enabled_chk;
	out["retry_label_edit"] = go_retry_label_edit;
	out["title_enabled_chk"] = go_title_enabled_chk;
	out["title_label_edit"] = go_title_label_edit;
	out["window_style_opt"] = go_window_style_opt;
	out["live_preview"] = go_live_preview;
	return out;
}




