#include "clockgears_database.h"
#include "clockgears_damage_engine.h"
#include "clockgears_map.h"
#include "clockgears_workspace.h"
#include "core/clockgears_formulas.h"
#include "core/clockgears_pixel_art.h"
#include "core/clockgears_system_catalog.h"
#include "core/config/project_settings.h"
#include "editor/clockgears_project_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "editor/clockgears_theme.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "icons/clockgears_icons.h"
#include "scene/gui/center_container.h"
#include "scene/gui/separator.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

#define CG_DB_TR(m_str) String::utf8(m_str)

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
	ClassDB::bind_method(D_METHOD("_on_file_selected", "path"), &ClockGearsDatabaseDialog::_on_file_selected);
}

void ClockGearsDatabaseDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			if (anim_preview_playing && is_visible()) {
				_update_anim_playback(get_process_delta_time());
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
	system_config["start_map"] = "";
	system_config["start_x"] = 0;
	system_config["start_y"] = 0;
	system_config["camera_zoom"] = 2;
	system_config["camera_smooth"] = true;
	system_config["camera_clamp"] = true;
	system_config["party"] = Array();
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
			return CG_DB_TR("Interruptores y Variables");
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
	d["hp"] = is_equip ? 0 : 100;
	d["mp"] = is_equip ? 0 : 20;
	d["atk"] = is_equip ? 0 : 10;
	d["def"] = is_equip ? 0 : 10;
	d["mat"] = is_equip ? 0 : 10;
	d["mdf"] = is_equip ? 0 : 10;
	d["agi"] = is_equip ? 0 : 10;
	d["luk"] = is_equip ? 0 : 10;
	d["graphic_path"] = "";
	d["traits"] = Array();
	if (p_category == CAT_ACTORS) {
		d["class_id"] = 0;
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
		// Sección 5.3: Enemigos (EXP, Oro, Battler animado, 3 Drops, Patrones de Acción)
		d["exp"] = 0;
		d["gold"] = 0;
		d["battler_frames"] = 1;
		d["battler_fps"] = 8;
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

	String fname = p_path.get_file();
	Vector<String> candidates;
	candidates.push_back(p_path);
	if (ProjectSettings::get_singleton()) {
		candidates.push_back(ProjectSettings::get_singleton()->globalize_path(p_path));
	}
	candidates.push_back(String("res://img/battlers/").path_join(fname));
	candidates.push_back(String("res://img/characters/").path_join(fname));
	candidates.push_back(String("res://img/enemies/").path_join(fname));
	candidates.push_back(String("res://img/tilesets/").path_join(fname));
	candidates.push_back(String("res://img/system/").path_join(fname));
	candidates.push_back(String("res://img/").path_join(fname));
	candidates.push_back(String("res://").path_join(fname));

	const char *known_subdirs[] = { "img/battlers", "img/characters", "img/enemies", "img/tilesets", "img/system", "img" };
	for (size_t s = 0; s < sizeof(known_subdirs) / sizeof(known_subdirs[0]); s++) {
		String g_dir = ClockGearsProjectStructure::resolve_project_folder_global(known_subdirs[s]);
		if (!g_dir.is_empty()) {
			candidates.push_back(g_dir.path_join(fname));
		}
	}

	Ref<Image> img;
	img.instantiate();
	for (int i = 0; i < candidates.size(); i++) {
		const String &cand = candidates[i];
		if (cand.is_empty()) continue;
		if (ResourceLoader::exists(cand)) {
			Ref<Texture2D> res = ResourceLoader::load(cand);
			if (res.is_valid()) {
				return res;
			}
		}
		if (FileAccess::exists(cand)) {
			if (ImageLoader::load_image(cand, img) == OK && img.is_valid() && !img->is_empty()) {
				Ref<ImageTexture> itex = ImageTexture::create_from_image(img);
				itex->set_path(cand, true);
				return itex;
			}
		}
	}

	return Ref<Texture2D>();
}

void ClockGearsDatabaseDialog::_on_category_selected(int p_category) {
	if (p_category < 0 || p_category >= CAT_MAX) {
		return;
	}
	current_category = p_category;
	for (int i = 0; i < category_buttons.size(); i++) {
		category_buttons[i]->set_pressed_no_signal(i == current_category);
	}
	_refresh_category_view();
}

void ClockGearsDatabaseDialog::_refresh_category_view() {
	updating_ui = true;

	String cat_name = _get_category_name(current_category);
	if (col1_title) {
		col1_title->set_text(vformat(CG_DB_TR("Lista de %s"), cat_name));
	}
	if (col2_title) {
		col2_title->set_text(vformat(CG_DB_TR("Parámetros de %s"), cat_name));
	}

	bool is_system = (current_category == CAT_SYSTEM);
	bool is_tileset = (current_category == CAT_TILESETS);

	bool is_actor_or_enemy = (current_category == CAT_ACTORS || current_category == CAT_ENEMIES);

	if (col1_box) {
		col1_box->set_visible(!is_system);
	}
	if (params_grid) {
		params_grid->set_visible(!is_system);
	}
	if (system_panel) {
		system_panel->set_visible(is_system);
	}
	if (col3_title) {
		if (is_actor_or_enemy) {
			col3_title->set_text(CG_DB_TR("Sprite & Animaciones (SpriteFrames)"));
		} else if (is_tileset) {
			col3_title->set_text(CG_DB_TR("Visor de Tilesets & Colisiones"));
		} else {
			col3_title->set_text(CG_DB_TR("Gráfico / Sprite / Icono"));
		}
	}
	if (col3_box) {
		col3_box->set_visible(!is_system);
	}
	if (col3_scroll_container) {
		col3_scroll_container->set_visible(!is_system);
	}
	if (sprite_anim_editor_box) {
		sprite_anim_editor_box->set_visible(is_actor_or_enemy);
	}
	if (anim_editor_scroll) {
		anim_editor_scroll->set_visible(is_actor_or_enemy);
	}
	if (troop_members_box) {
		troop_members_box->set_visible(current_category == CAT_TROOPS);
	}
	if (standard_graphic_box) {
		standard_graphic_box->set_visible(!is_actor_or_enemy && !is_tileset);
	}
	if (tileset_collision_box) {
		tileset_collision_box->set_visible(is_tileset);
	}
	if (col2_scroll_container) {
		col2_scroll_container->set_stretch_ratio(is_system ? 1.0f : 0.36f);
		col2_scroll_container->set_horizontal_scroll_mode(is_system ? ScrollContainer::SCROLL_MODE_DISABLED : ScrollContainer::SCROLL_MODE_AUTO);
		col2_scroll_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	}
	if (col2_box) {
		col2_box->set_custom_minimum_size(is_system ? Size2(0, 0) : Size2(250 * EDSCALE, 0));
	}
	if (tileset_slots_box) {
		tileset_slots_box->set_visible(is_tileset);
	}
	if (enemy_drops_box) {
		enemy_drops_box->set_visible(current_category == CAT_ENEMIES);
	}
	if (enemy_actions_box) {
		enemy_actions_box->set_visible(current_category == CAT_ENEMIES);
	}
	if (traits_box) {
		traits_box->set_visible(!is_system);
	}
	if (effects_box) {
		effects_box->set_visible(!is_system);
	}

	if (is_system) {
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

	if (standard_graphic_box) {
		standard_graphic_box->set_visible(!is_tileset);
	}
	if (tileset_collision_box) {
		tileset_collision_box->set_visible(is_tileset);
	}
	if (col3_title) {
		col3_title->set_text(is_tileset ? CG_DB_TR("Hoja y Máscara de Colisión (O / X / ★)") : CG_DB_TR("Gráfico / Sprite / Icono"));
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
	bool is_classes = (current_category == CAT_CLASSES);
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
	bool is_enemy = (current_category == CAT_ENEMIES);

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

	// Sección 5.1: Animación de Ataque en Armas
	bool show_atk_anim = (current_category == CAT_WEAPONS);
	if (attack_anim_label) {
		attack_anim_label->set_visible(show_atk_anim);
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

	// Sección 5.3: Enemigos (EXP, Oro, Battler animado, 3 Drops, Patrones de Acción)
	if (enemy_exp_label) {
		enemy_exp_label->set_visible(is_enemy);
	}
	if (enemy_exp_spin) {
		enemy_exp_spin->set_visible(is_enemy);
	}
	if (enemy_gold_label) {
		enemy_gold_label->set_visible(is_enemy);
	}
	if (enemy_gold_spin) {
		enemy_gold_spin->set_visible(is_enemy);
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
	if (enemy_drops_box) {
		enemy_drops_box->set_visible(is_enemy);
	}
	if (enemy_actions_box) {
		enemy_actions_box->set_visible(is_enemy);
	}

	bool show_stats = (current_category == CAT_ACTORS || current_category == CAT_CLASSES ||
			is_equip || is_enemy);
	for (int i = 0; i < 8; i++) {
		stat_labels[i]->set_visible(show_stats);
		stat_spins[i]->set_visible(show_stats);
		// Sección 5.1: Equipamiento permite modificadores negativos (-999..9999)
		stat_spins[i]->set_min(is_equip ? -999 : 0);
		stat_spins[i]->set_max(is_equip ? 9999 : 999999);
	}

	bool show_formula = (is_skill || is_item || current_category == CAT_COMMON_EVENTS);
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

	// Sección 4.2 / 4.3 / 5.1 / 5.2 / 5.3: Mostrar editor de Rasgos (Traits) o Efectos en Columna 3
	bool show_traits = (current_category == CAT_ACTORS || current_category == CAT_CLASSES ||
			is_equip || is_enemy || current_category == CAT_STATES);
	if (traits_box) {
		traits_box->set_visible(show_traits);
	}
	if (effects_box) {
		effects_box->set_visible(show_invocation);
	}

	_refresh_record_list();
	updating_ui = false;

	if (record_list->get_item_count() > 0) {
		record_list->select(0);
		_on_record_selected(0);
	} else {
		_on_record_selected(-1);
	}
}

void ClockGearsDatabaseDialog::_refresh_record_list() {
	if (!record_list || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	int prev_sel = -1;
	Vector<int> sel = record_list->get_selected_items();
	if (!sel.is_empty()) {
		prev_sel = sel[0];
	}

	record_list->clear();
	const Array &arr = category_data[current_category];
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		int id = d.get("id", i + 1);
		String nm = d.get("name", "");
		record_list->add_item(vformat("%03d: %s", id, nm));
	}

	if (prev_sel >= 0 && prev_sel < record_list->get_item_count()) {
		record_list->select(prev_sel);
	}
}

void ClockGearsDatabaseDialog::_on_record_selected(int p_index) {
	updating_ui = true;
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
		if (enemy_exp_spin) {
			enemy_exp_spin->set_value(0);
		}
		if (enemy_gold_spin) {
			enemy_gold_spin->set_value(0);
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
		if (graphic_path_label) {
			graphic_path_label->set_text(CG_DB_TR("(Sin registro)"));
		}
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
		return;
	}

	Dictionary d = arr[p_index];
	name_edit->set_text(d.get("name", ""));
	desc_edit->set_text(d.get("desc", ""));
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
		_select_option_by_id_or_index(attack_anim_option, int(d.get("attack_anim_id", 0)));
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
	if (current_category == CAT_ENEMIES) {
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
		Array actions = d.get("actions", d.get("action_patterns", Array()));
		_refresh_action_patterns_tree(actions);
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

	if (current_category == CAT_TILESETS) {
		_refresh_tileset_slots_ui();
	} else if (current_category == CAT_TROOPS) {
		Array mems = d.get("members", Array());
		if (mems.is_empty()) {
			int old_en = int(d.get("type", 1));
			if (old_en <= 0) old_en = 1;
			Dictionary m0;
			m0["enemy_id"] = old_en;
			m0["x"] = 280;
			m0["y"] = 140;
			mems.push_back(m0);
			d["members"] = mems;
			arr[p_index] = d;
		}
		_refresh_troop_members_ui();
	} else if (current_category == CAT_ACTORS || current_category == CAT_ENEMIES) {
		Dictionary sp_dict = d.get("sprite_data", Dictionary());
		if (!sp_dict.is_empty()) {
			current_sprite_data.from_dict(sp_dict);
		} else {
			int old_frames = int(d.get("battler_frames", 1));
			float old_fps = float(d.get("battler_fps", 8));
			current_sprite_data = ClockGearsSpriteSheetData();
			current_sprite_data.cols = MAX(1, old_frames);
			current_sprite_data.rows = 1;
			ClockGearsSpriteAnimation idle_anim;
			idle_anim.name = "idle";
			idle_anim.spritesheet_path = d.get("graphic_path", "");
			idle_anim.fps = old_fps > 0 ? old_fps : 6.0f;
			idle_anim.loop = true;
			for (int f = 0; f < old_frames; f++) {
				idle_anim.frames.push_back(f);
			}
			current_sprite_data.animations["idle"] = idle_anim;
		}

		if (current_sprite_data.animations.is_empty()) {
			ClockGearsSpriteAnimation idle_anim;
			idle_anim.name = "idle";
			idle_anim.spritesheet_path = d.get("graphic_path", "");
			idle_anim.fps = 6.0f;
			idle_anim.loop = true;
			idle_anim.frames.push_back(0);
			current_sprite_data.animations["idle"] = idle_anim;
		}

		_refresh_anim_track_options();
	} else {
		String gpath = d.get("graphic_path", "");
		Ref<Texture2D> tex = _load_texture_from_path(gpath);
		if (graphic_preview) {
			graphic_preview->set_texture(tex);
		}
		if (graphic_path_label) {
			graphic_path_label->set_text(gpath.is_empty() ? CG_DB_TR("(Sin gráfico)") : gpath.get_file());
		}
	}

	updating_ui = false;
}

void ClockGearsDatabaseDialog::_on_add_record_pressed() {
	if (current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	int new_id = arr.size() + 1;
	Dictionary rec = _create_empty_record(current_category, new_id);
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
	_save_database_to_disk();
	if (current_category == CAT_TILESETS && ClockGearsMapPanel::get_singleton()) {
		ClockGearsMapPanel::get_singleton()->load_persisted_tilesets();
	}
	if (name_edit && name_edit->is_visible_in_tree()) {
		name_edit->grab_focus();
		name_edit->select_all();
	}
}

void ClockGearsDatabaseDialog::_on_remove_record_pressed() {
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
	_save_database_to_disk();
	if (current_category == CAT_TILESETS && ClockGearsMapPanel::get_singleton()) {
		ClockGearsMapPanel::get_singleton()->load_persisted_tilesets();
	}
}

void ClockGearsDatabaseDialog::_on_change_max_pressed() {
	if (!change_max_dialog || !change_max_spin) {
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
		arr.push_back(_create_empty_record(current_category, new_id));
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
	_save_database_to_disk();
}

void ClockGearsDatabaseDialog::_on_copy_record_pressed() {
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
		arr[target_idx] = pasted;
	} else {
		int new_id = arr.size() + 1;
		Dictionary pasted = clipboard_record.duplicate(true);
		pasted["id"] = new_id;
		arr.push_back(pasted);
		target_idx = arr.size() - 1;
	}
	_refresh_record_list();
	record_list->select(target_idx);
	_on_record_selected(target_idx);
	_save_database_to_disk();
}

void ClockGearsDatabaseDialog::_on_param_changed() {
	if (updating_ui || current_category < 0 || current_category >= CAT_MAX) {
		return;
	}
	Array &arr = category_data[current_category];
	Vector<int> sel = record_list->get_selected_items();
	int idx = -1;
	if (sel.is_empty()) {
		// Si el usuario escribe directamente cuando la lista está vacía, auto-creamos el registro 001
		if (arr.is_empty()) {
			arr.push_back(_create_empty_record(current_category, 1));
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

	Dictionary d = arr[idx];
	d["name"] = name_edit->get_text();
	d["desc"] = desc_edit->get_text();

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
	if (current_category == CAT_WEAPONS && attack_anim_option) {
		int sa = attack_anim_option->get_selected();
		d["attack_anim_id"] = (sa >= 0) ? attack_anim_option->get_item_id(sa) : 0;
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
	if (current_category == CAT_ENEMIES) {
		if (enemy_exp_spin) {
			d["exp"] = int(enemy_exp_spin->get_value());
		}
		if (enemy_gold_spin) {
			d["gold"] = int(enemy_gold_spin->get_value());
		}
	}
	if (current_category == CAT_ACTORS || current_category == CAT_ENEMIES) {
		d["sprite_data"] = current_sprite_data.to_dict();
		d["animations"] = current_sprite_data.to_dict().get("animations", Dictionary());
		d["battler_frames"] = current_sprite_data.cols;
		d["battler_fps"] = current_sprite_data.animations.has("idle") ? current_sprite_data.animations["idle"].fps : 6.0f;
	}

	const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	for (int i = 0; i < 8; i++) {
		d[stat_keys[i]] = (int)stat_spins[i]->get_value();
	}

	arr[idx] = d;
	int id = d.get("id", idx + 1);
	record_list->set_item_text(idx, vformat("%03d: %s", id, String(d["name"])));
	_save_database_to_disk();
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
	if (updating_ui || current_category != CAT_ENEMIES) {
		return;
	}
	Array &arr = category_data[CAT_ENEMIES];
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
	_save_database_to_disk();
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
			param_str = vformat("%d + %d*X", cond_a, cond_b);
		} else if (cond_type == CG_AI_COND_HP_RANGE || cond_type == CG_AI_COND_MP_RANGE) {
			param_str = vformat("%d%% ~ %d%%", cond_a, cond_b);
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
		_save_database_to_disk();
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
	_save_database_to_disk();
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
		_save_database_to_disk();
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
			arr.push_back(_create_empty_record(current_category, 1));
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
	_save_database_to_disk();
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
		_save_database_to_disk();
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
			arr.push_back(_create_empty_record(current_category, 1));
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
	_save_database_to_disk();
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
		_save_database_to_disk();
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
		_save_database_to_disk();
	}
}

void ClockGearsDatabaseDialog::_on_start_map_selected(int p_idx) {
	if (updating_ui || !sys_start_map_option) {
		return;
	}
	String map_path = sys_start_map_option->get_item_metadata(p_idx);
	system_config["start_map"] = map_path;
	system_config["has_start_pos"] = !map_path.is_empty();
	_save_database_to_disk();
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
	Ref<Texture2D> tex;
	if (idx >= 0 && idx < arr.size()) {
		Dictionary d = arr[idx];
		tex = _load_texture_from_path(d.get("graphic_path", ""));
	}
	if (tex.is_null() && !category_data[CAT_TILESETS].is_empty()) {
		Dictionary ts0 = category_data[CAT_TILESETS][0];
		tex = _load_texture_from_path(ts0.get("graphic_path", ""));
	}
	tile_picker_dialog->open_picker(tex, callable_mp(this, &ClockGearsDatabaseDialog::_on_tile_graphic_picked));
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
	int tsz = pk->get_tile_size();
	Dictionary d = arr[idx];
	d["graphic_path"] = pk->get_atlas_path();
	d["tile_coords"] = primary;
	d["tile_block_size"] = p_block;
	arr[idx] = d;

	Ref<Image> src_img = atlas_tex->get_image();
	if (src_img.is_valid() && !src_img->is_empty()) {
		Rect2i reg(primary.x * tsz, primary.y * tsz, p_block.x * tsz, p_block.y * tsz);
		reg = reg.intersection(Rect2i(0, 0, src_img->get_width(), src_img->get_height()));
		if (reg.size.x > 0 && reg.size.y > 0) {
			Ref<Image> sub = src_img->get_region(reg);
			if (graphic_preview) {
				graphic_preview->set_texture(ImageTexture::create_from_image(sub));
			}
		}
	}
	if (graphic_path_label) {
		graphic_path_label->set_text(vformat("%s [%d,%d] (%dx%d)", pk->get_atlas_path().get_file(), primary.x, primary.y, p_block.x, p_block.y));
	}
	_save_database_to_disk();
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
		active_slot_info_label->set_text(vformat(CG_DB_TR("Ranura Activa: %s (Rejilla 32x32 Anexo A4)"), active_key));
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
	d["graphic_path"] = p_path;
	arr[idx] = d;

	ClockGearsProjectStructure::write_pixel_art_import_preset(p_path);

	String target_folder = (current_category == CAT_ACTORS || current_category == CAT_FLORA || current_category == CAT_FAUNA)
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
	if (graphic_preview) {
		graphic_preview->set_texture(tex);
	}
	if (graphic_path_label) {
		graphic_path_label->set_text(p_path.get_file());
	}
	if (current_category == CAT_ACTORS || current_category == CAT_ENEMIES) {
		String cur_anim_name = _get_active_anim_name();
		if (cur_anim_name.is_empty()) cur_anim_name = "idle";
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
		arr[idx] = d;
	}
	_save_database_to_disk();
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
	if (cur_sz.x >= min_sz.x && cur_sz.y >= min_sz.y && ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("clockgears/editor/database_window_size", cur_sz);
		ProjectSettings::get_singleton()->save();
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
}

void ClockGearsDatabaseDialog::load_from_disk() {
	_load_database_from_disk();
	_refresh_category_view();
}

void ClockGearsDatabaseDialog::set_remembered_window_size(const Vector2i &p_size) {
	Size2i clamped = p_size.max(get_min_size());
	set_size(clamped);
	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("clockgears/editor/database_window_size", clamped);
		ProjectSettings::get_singleton()->save();
	}
}

Vector2i ClockGearsDatabaseDialog::get_remembered_window_size() const {
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("clockgears/editor/database_window_size")) {
		Variant v = ProjectSettings::get_singleton()->get_setting("clockgears/editor/database_window_size");
		if (v.get_type() == Variant::VECTOR2I || v.get_type() == Variant::VECTOR2) {
			return Vector2i(v).max(get_min_size());
		}
	}
	return get_size().max(get_min_size());
}

Dictionary ClockGearsDatabaseDialog::check_layout_fits_window(const Vector2i &p_window_size) const {
	Dictionary res;
	int win_w = MAX(p_window_size.x, get_min_size().x);
	int win_h = MAX(p_window_size.y, get_min_size().y);
	int left_min_w = left_sidebar_panel ? int(left_sidebar_panel->get_custom_minimum_size().x) : 140;
	int col1_min_w = col1_box ? int(col1_box->get_custom_minimum_size().x) : 140;
	int col2_min_w = col2_scroll_container ? int(col2_scroll_container->get_custom_minimum_size().x) : 200;
	int col3_min_w = col3_scroll_container ? int(col3_scroll_container->get_custom_minimum_size().x) : 220;
	int picker_min_w = db_tile_picker ? int(db_tile_picker->get_combined_minimum_size().x) : 180;
	int total_min_w = left_min_w + 10 + col1_min_w + 8 + col2_min_w + 8 + MAX(col3_min_w, picker_min_w) + 16;

	int right_avail = MAX(100, win_w - left_min_w - 26);
	int alloc_col1 = MAX(col1_min_w, int(right_avail * 0.20f));
	int alloc_col2 = MAX(col2_min_w, int(right_avail * 0.36f));
	int alloc_col3 = MAX(col3_min_w, right_avail - alloc_col1 - alloc_col2 - 16);

	res["window_size"] = Vector2i(win_w, win_h);
	res["left_min_w"] = left_min_w;
	res["col1_min_w"] = col1_min_w;
	res["col2_min_w"] = col2_min_w;
	res["col3_min_w"] = col3_min_w;
	res["picker_min_w"] = picker_min_w;
	res["total_min_w"] = total_min_w;
	res["alloc_col1_w"] = alloc_col1;
	res["alloc_col2_w"] = alloc_col2;
	res["alloc_col3_w"] = alloc_col3;
	res["help_wraps"] = true;
	res["picker_has_scrollbars"] = (db_tile_picker != nullptr);
	res["fits_horizontally"] = (total_min_w <= win_w) && (picker_min_w <= alloc_col3);
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
	if (!String(system_config.get("start_map", "")).is_empty()) {
		system_config["has_start_pos"] = true;
	}
	_save_database_to_disk();
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
	_save_database_to_disk();
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
		p_spin->set_value(parsed);
		p_spin->set_meta("cg_last_valid_val", p_spin->get_value());
		le->set_text(String::num(p_spin->get_value(), Math::range_step_decimals(p_spin->get_step())));
	}
}

ClockGearsDatabaseDialog::ClockGearsDatabaseDialog() {
	singleton = this;
	set_process(true);
	set_title(CG_DB_TR("Base de Datos - ClockGears Engine"));
	// Bug 4c / PROBLEMA A: Redimensionable, recuerda su tamaño y ajusta los 3 paneles sin desbordar
	set_wrap_controls(false);
	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_min_size(Size2(720, 440) * EDSCALE);
	connect("size_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_window_size_changed));

	_init_empty_database();
	if (EditorNode::get_singleton()) {
		_load_database_from_disk();
	}

	HSplitContainer *main_hsplit = memnew(HSplitContainer);
	main_hsplit->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_hsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_hsplit->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	main_hsplit->add_theme_constant_override("separation", 8);
	add_child(main_hsplit);

	// ========================================================================
	// LEFT SIDEBAR (~150px): 19 Categorías dentro de ScrollContainer
	// ========================================================================
	PanelContainer *left_panel = memnew(PanelContainer);
	left_sidebar_panel = left_panel;
	left_panel->set_custom_minimum_size(Size2(150 * EDSCALE, 0));
	left_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	left_panel->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	Ref<StyleBoxFlat> left_style = memnew(StyleBoxFlat);
	left_style->set_anti_aliased(false);
	left_style->set_bg_color(Color(0.12f, 0.13f, 0.15f, 1.0f));
	left_style->set_content_margin_all(5);
	left_panel->add_theme_style_override("panel", left_style);
	main_hsplit->add_child(left_panel);

	VBoxContainer *left_outer_vb = memnew(VBoxContainer);
	left_outer_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	left_outer_vb->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	left_outer_vb->add_theme_constant_override("separation", 4);
	left_panel->add_child(left_outer_vb);

	Label *cat_header = memnew(Label(CG_DB_TR("Categorías")));
	cat_header->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	left_outer_vb->add_child(cat_header);
	left_outer_vb->add_child(memnew(HSeparator));

	ScrollContainer *cat_scroll = memnew(ScrollContainer);
	cat_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cat_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	cat_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	cat_scroll->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	left_outer_vb->add_child(cat_scroll);

	VBoxContainer *cat_vbox = memnew(VBoxContainer);
	cat_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cat_vbox->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	cat_vbox->add_theme_constant_override("separation", 4);
	cat_scroll->add_child(cat_vbox);

	for (int i = 0; i < CAT_MAX; i++) {
		Button *btn = memnew(Button(_get_category_name(i)));
		btn->set_toggle_mode(true);
		btn->set_clip_text(true);
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		btn->set_custom_minimum_size(Size2(0, 26 * EDSCALE));
		if (ClockGearsIcons::get_singleton()) {
			btn->set_button_icon(ClockGearsIcons::get_singleton()->get_db_category_icon(i, EDSCALE));
		}
		btn->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_category_selected).bind(i));
		cat_vbox->add_child(btn);
		category_buttons.push_back(btn);
	}

	// ========================================================================
	// RIGHT EDITOR AREA: 3-Column HBoxContainer (List | Parameters | Sheet/Graphics)
	// ========================================================================
	HBoxContainer *right_hbox = memnew(HBoxContainer);
	right_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	right_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	right_hbox->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	right_hbox->add_theme_constant_override("separation", 8);
	main_hsplit->add_child(right_hbox);

	// --- COLUMN 1: Record List + "+", "-", "Max", "Copiar", "Pegar" (stretch 0.22) ---
	col1_box = memnew(VBoxContainer);
	col1_box->set_custom_minimum_size(Size2(140 * EDSCALE, 0));
	col1_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col1_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col1_box->set_stretch_ratio(0.20f);
	col1_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	col1_box->add_theme_constant_override("separation", 6);
	right_hbox->add_child(col1_box);

	col1_title = memnew(Label(CG_DB_TR("Lista de Actores")));
	col1_title->set_clip_text(true);
	col1_box->add_child(col1_title);

	record_list = memnew(ItemList);
	record_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	record_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	record_list->set_custom_minimum_size(Size2(0, 120 * EDSCALE));
	record_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_record_selected));
	col1_box->add_child(record_list);

	// Los 5 botones (+, -, Max, Copiar, Pegar) siempre visibles y compactos debajo de la lista
	HBoxContainer *list_btn_row = memnew(HBoxContainer);
	list_btn_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	list_btn_row->set_v_size_flags(Control::SIZE_SHRINK_END);
	list_btn_row->add_theme_constant_override("separation", 3);
	col1_box->add_child(list_btn_row);

	btn_add_record = memnew(Button("+"));
	btn_add_record->set_custom_minimum_size(Size2(26 * EDSCALE, 26 * EDSCALE));
	btn_add_record->set_tooltip_text(CG_DB_TR("Añadir nuevo registro"));
	btn_add_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_record_pressed));
	list_btn_row->add_child(btn_add_record);

	btn_remove_record = memnew(Button("-"));
	btn_remove_record->set_custom_minimum_size(Size2(26 * EDSCALE, 26 * EDSCALE));
	btn_remove_record->set_tooltip_text(CG_DB_TR("Eliminar registro seleccionado"));
	btn_remove_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_record_pressed));
	list_btn_row->add_child(btn_remove_record);

	btn_change_max = memnew(Button(CG_DB_TR("Max")));
	btn_change_max->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_change_max->set_clip_text(true);
	btn_change_max->set_tooltip_text(CG_DB_TR("Cambiar cantidad máxima de registros"));
	btn_change_max->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_change_max_pressed));
	list_btn_row->add_child(btn_change_max);

	btn_copy_record = memnew(Button(CG_DB_TR("Copiar")));
	btn_copy_record->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_copy_record->set_clip_text(true);
	btn_copy_record->set_tooltip_text(CG_DB_TR("Copiar el registro seleccionado"));
	btn_copy_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_copy_record_pressed));
	list_btn_row->add_child(btn_copy_record);

	btn_paste_record = memnew(Button(CG_DB_TR("Pegar")));
	btn_paste_record->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_paste_record->set_clip_text(true);
	btn_paste_record->set_tooltip_text(CG_DB_TR("Pegar el registro copiado"));
	btn_paste_record->set_disabled(true);
	btn_paste_record->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_paste_record_pressed));
	list_btn_row->add_child(btn_paste_record);

	// --- COLUMN 2: Parameters inside ScrollContainer (stretch 0.40) ---
	ScrollContainer *col2_scroll = memnew(ScrollContainer);
	col2_scroll_container = col2_scroll;
	col2_scroll->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
	col2_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col2_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col2_scroll->set_stretch_ratio(0.40f);
	col2_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	col2_scroll->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	right_hbox->add_child(col2_scroll);

	col2_box = memnew(VBoxContainer);
	col2_box->set_custom_minimum_size(Size2(0, 0));
	col2_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col2_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col2_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	col2_box->add_theme_constant_override("separation", 8);
	col2_scroll->add_child(col2_box);

	col2_title = memnew(Label(CG_DB_TR("Parámetros Generales")));
	col2_title->set_clip_text(true);
	col2_box->add_child(col2_title);
	col2_box->add_child(memnew(HSeparator));

	params_grid = memnew(GridContainer);
	params_grid->set_columns(2);
	params_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_grid->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	params_grid->add_theme_constant_override("h_separation", 8);
	params_grid->add_theme_constant_override("v_separation", 6);
	col2_box->add_child(params_grid);

	Label *lbl_nom = memnew(Label(CG_DB_TR("Nombre:")));
	lbl_nom->set_tooltip_text(CG_DB_TR("Nombre identificador"));
	params_grid->add_child(lbl_nom);
	name_edit = memnew(LineEdit);
	name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
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
	tertiary_option->set_clip_text(true);
	tertiary_option->set_focus_mode(Control::FOCUS_ALL);
	tertiary_option->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	tertiary_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(tertiary_option);

	// Retoque 2 (Sección 9.3): Campos numéricos de la fórmula de EXP para Clases (base=30, extra=20, acc_a=30, acc_b=30)
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
		params_grid->add_child(exp_labels[i]);

		exp_spins[i] = memnew(SpinBox);
		exp_spins[i]->set_min(exp_mins[i]);
		exp_spins[i]->set_max(exp_maxs[i]);
		exp_spins[i]->set_step(1);
		exp_spins[i]->set_value(exp_defaults[i]);
		exp_spins[i]->set_editable(true);
		exp_spins[i]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_bind_numeric_spinbox(exp_spins[i]);
		exp_spins[i]->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
		params_grid->add_child(exp_spins[i]);
	}

	exp_preview_title = memnew(Label(CG_DB_TR("Curva EXP:")));
	exp_preview_title->set_tooltip_text(CG_DB_TR("Muestra Curva EXP (9.3)"));
	params_grid->add_child(exp_preview_title);
	exp_preview_label = memnew(Label);
	exp_preview_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	exp_preview_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	exp_preview_label->add_theme_color_override("font_color", Color(0.55f, 0.85f, 0.98f));
	params_grid->add_child(exp_preview_label);

	// Sección 5.1 y 5.2: Precio de Compra, Precio de Venta (floor(precio/2)), Animación de Ataque y Consumible
	price_label = memnew(Label(CG_DB_TR("Precio:")));
	price_label->set_tooltip_text(CG_DB_TR("Precio de Compra"));
	params_grid->add_child(price_label);
	price_spin = memnew(SpinBox);
	price_spin->set_min(0);
	price_spin->set_max(999999);
	price_spin->set_step(1);
	price_spin->set_editable(true);
	price_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(price_spin);
	price_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(price_spin);

	sell_price_title = memnew(Label(CG_DB_TR("Venta:")));
	sell_price_title->set_tooltip_text(CG_DB_TR("Precio de Venta (50%)"));
	params_grid->add_child(sell_price_title);
	sell_price_label = memnew(Label("0 G (50%)"));
	sell_price_label->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	params_grid->add_child(sell_price_label);

	attack_anim_label = memnew(Label(CG_DB_TR("Animación:")));
	attack_anim_label->set_tooltip_text(CG_DB_TR("Animación de Ataque"));
	params_grid->add_child(attack_anim_label);
	attack_anim_option = memnew(OptionButton);
	attack_anim_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	attack_anim_option->set_clip_text(true);
	attack_anim_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(attack_anim_option);

	consumable_label = memnew(Label(CG_DB_TR("Consumible:")));
	consumable_label->set_tooltip_text(CG_DB_TR("Determina si se consume al usar"));
	params_grid->add_child(consumable_label);
	consumable_check = memnew(CheckBox(CG_DB_TR("Consumible")));
	consumable_check->set_tooltip_text(CG_DB_TR("Se consume al usar"));
	consumable_check->set_pressed(true);
	consumable_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_check_toggled));
	params_grid->add_child(consumable_check);

	// Sección 5.2: Invocación (Velocidad, Éxito %, Repeticiones, Ganancia TP, Tipo de Golpe)
	item_speed_label = memnew(Label(CG_DB_TR("Velocidad:")));
	item_speed_label->set_tooltip_text(CG_DB_TR("Velocidad de Acción"));
	params_grid->add_child(item_speed_label);
	item_speed_spin = memnew(SpinBox);
	item_speed_spin->set_min(-2000);
	item_speed_spin->set_max(2000);
	item_speed_spin->set_step(1);
	item_speed_spin->set_value(0);
	item_speed_spin->set_editable(true);
	item_speed_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(item_speed_spin);
	item_speed_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(item_speed_spin);

	item_success_label = memnew(Label(CG_DB_TR("Éxito (%):")));
	item_success_label->set_tooltip_text(CG_DB_TR("Tasa de Éxito (%)"));
	params_grid->add_child(item_success_label);
	item_success_spin = memnew(SpinBox);
	item_success_spin->set_min(0);
	item_success_spin->set_max(100);
	item_success_spin->set_step(1);
	item_success_spin->set_value(100);
	item_success_spin->set_editable(true);
	item_success_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(item_success_spin);
	item_success_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(item_success_spin);

	item_repeats_label = memnew(Label(CG_DB_TR("Repeticiones:")));
	item_repeats_label->set_tooltip_text(CG_DB_TR("Número de Repeticiones"));
	params_grid->add_child(item_repeats_label);
	item_repeats_spin = memnew(SpinBox);
	item_repeats_spin->set_min(1);
	item_repeats_spin->set_max(9);
	item_repeats_spin->set_step(1);
	item_repeats_spin->set_value(1);
	item_repeats_spin->set_editable(true);
	item_repeats_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(item_repeats_spin);
	item_repeats_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(item_repeats_spin);

	item_tp_gain_label = memnew(Label(CG_DB_TR("TP:")));
	item_tp_gain_label->set_tooltip_text(CG_DB_TR("Ganancia de PT (TP)"));
	params_grid->add_child(item_tp_gain_label);
	item_tp_gain_spin = memnew(SpinBox);
	item_tp_gain_spin->set_min(0);
	item_tp_gain_spin->set_max(100);
	item_tp_gain_spin->set_step(1);
	item_tp_gain_spin->set_value(0);
	item_tp_gain_spin->set_editable(true);
	item_tp_gain_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(item_tp_gain_spin);
	item_tp_gain_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(item_tp_gain_spin);

	hit_type_label = memnew(Label(CG_DB_TR("Tipo Golpe:")));
	hit_type_label->set_tooltip_text(CG_DB_TR("Tipo de Golpe"));
	params_grid->add_child(hit_type_label);
	hit_type_option = memnew(OptionButton);
	hit_type_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hit_type_option->set_clip_text(true);
	hit_type_option->add_item(CG_DB_TR("Golpe Seguro"), 0);
	hit_type_option->add_item(CG_DB_TR("Ataque Físico"), 1);
	hit_type_option->add_item(CG_DB_TR("Ataque Mágico"), 2);
	hit_type_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(hit_type_option);

	item_element_label = memnew(Label(CG_DB_TR("Elemento:")));
	item_element_label->set_tooltip_text(CG_DB_TR("Elemento de Daño"));
	params_grid->add_child(item_element_label);
	item_element_option = memnew(OptionButton);
	item_element_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_element_option->set_clip_text(true);
	item_element_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_option_changed));
	params_grid->add_child(item_element_option);

	formula_label = memnew(Label(CG_DB_TR("Fórmula Daño:")));
	formula_label->set_tooltip_text(CG_DB_TR("Fórmula de Daño"));
	params_grid->add_child(formula_label);
	formula_edit = memnew(ClockGearsFormulaLineEdit);
	formula_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	formula_edit->set_focus_mode(Control::FOCUS_ALL);
	formula_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	formula_edit->set_editable(true);
	formula_edit->set_placeholder("max(a.atk * 4 - b.def * 2, 0)");
	formula_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_text_changed));
	params_grid->add_child(formula_edit);

	variance_label = memnew(Label(CG_DB_TR("Varianza (%):")));
	variance_label->set_tooltip_text(CG_DB_TR("Varianza de Daño (%)"));
	params_grid->add_child(variance_label);
	variance_spin = memnew(SpinBox);
	variance_spin->set_min(0);
	variance_spin->set_max(100);
	variance_spin->set_step(1);
	variance_spin->set_value(20);
	variance_spin->set_editable(true);
	variance_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(variance_spin);
	variance_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(variance_spin);

	critical_label = memnew(Label(CG_DB_TR("Crítico:")));
	critical_label->set_tooltip_text(CG_DB_TR("Golpe Crítico"));
	params_grid->add_child(critical_label);
	critical_check = memnew(CheckBox(CG_DB_TR("Crítico")));
	critical_check->set_tooltip_text(CG_DB_TR("Permite golpe crítico"));
	critical_check->set_pressed(false);
	critical_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_check_toggled));
	params_grid->add_child(critical_check);

	// Sección 5.3: Enemigos (EXP, Oro, Battler animado)
	enemy_exp_label = memnew(Label(CG_DB_TR("EXP:")));
	enemy_exp_label->set_tooltip_text(CG_DB_TR("EXP Otorgada"));
	params_grid->add_child(enemy_exp_label);
	enemy_exp_spin = memnew(SpinBox);
	enemy_exp_spin->set_min(0);
	enemy_exp_spin->set_max(9999999);
	enemy_exp_spin->set_step(1);
	enemy_exp_spin->set_editable(true);
	enemy_exp_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
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
	_bind_numeric_spinbox(enemy_gold_spin);
	enemy_gold_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(enemy_gold_spin);

	battler_frames_label = memnew(Label(CG_DB_TR("Frames Battler:")));
	battler_frames_label->set_tooltip_text(CG_DB_TR("Frames Battler (Hoja)"));
	params_grid->add_child(battler_frames_label);
	battler_frames_spin = memnew(SpinBox);
	battler_frames_spin->set_min(1);
	battler_frames_spin->set_max(64);
	battler_frames_spin->set_step(1);
	battler_frames_spin->set_value(1);
	battler_frames_spin->set_editable(true);
	battler_frames_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(battler_frames_spin);
	battler_frames_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(battler_frames_spin);

	battler_fps_label = memnew(Label(CG_DB_TR("FPS Battler:")));
	battler_fps_label->set_tooltip_text(CG_DB_TR("FPS Battler Animado"));
	params_grid->add_child(battler_fps_label);
	battler_fps_spin = memnew(SpinBox);
	battler_fps_spin->set_min(1);
	battler_fps_spin->set_max(60);
	battler_fps_spin->set_step(1);
	battler_fps_spin->set_value(8);
	battler_fps_spin->set_editable(true);
	battler_fps_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_bind_numeric_spinbox(battler_fps_spin);
	battler_fps_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
	params_grid->add_child(battler_fps_spin);

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
		params_grid->add_child(stat_labels[i]);

		stat_spins[i] = memnew(SpinBox);
		stat_spins[i]->set_min(-999);
		stat_spins[i]->set_max(999999);
		stat_spins[i]->set_step(1);
		stat_spins[i]->set_editable(true);
		stat_spins[i]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_bind_numeric_spinbox(stat_spins[i]);
		stat_spins[i]->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_param_spin_changed));
		params_grid->add_child(stat_spins[i]);
	}

	// Sección Tropas (Fase 8A): Lista de Miembros con posición (X, Y)
	troop_members_box = memnew(VBoxContainer);
	troop_members_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_members_box->add_theme_constant_override("separation", 6);
	troop_members_box->hide();
	col2_box->add_child(troop_members_box);

	troop_members_box->add_child(memnew(HSeparator));
	HBoxContainer *t_hdr = memnew(HBoxContainer);
	t_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	Label *lbl_t_hdr = memnew(Label(CG_DB_TR("Miembros de la Tropa:")));
	lbl_t_hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lbl_t_hdr->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	t_hdr->add_child(lbl_t_hdr);

	btn_add_troop_member = memnew(Button(CG_DB_TR("+ Añadir Miembro")));
	btn_add_troop_member->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_troop_member_pressed));
	t_hdr->add_child(btn_add_troop_member);
	troop_members_box->add_child(t_hdr);

	troop_members_list_vbox = memnew(VBoxContainer);
	troop_members_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	troop_members_list_vbox->add_theme_constant_override("separation", 4);
	troop_members_box->add_child(troop_members_list_vbox);

	// PROBLEMA C: 9 Ranuras de Imagen de Tileset al estilo RPG Maker MZ (A1..A5, B, C, D, E - Rejilla 32x32 Anexo A4)
	tileset_slots_box = memnew(VBoxContainer);
	tileset_slots_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_slots_box->add_theme_constant_override("separation", 5);
	tileset_slots_box->hide();
	col2_box->add_child(tileset_slots_box);

	tileset_slots_box->add_child(memnew(HSeparator));
	HBoxContainer *slots_hdr_hb = memnew(HBoxContainer);
	slots_hdr_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slots_hdr_hb->add_theme_constant_override("separation", 4);
	Label *slots_header = memnew(Label(CG_DB_TR("Ranuras (A1-A5, B-E):")));
	slots_header->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slots_header->set_clip_text(true);
	slots_header->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	slots_hdr_hb->add_child(slots_header);
	Button *btn_slots_help = memnew(Button("?"));
	btn_slots_help->set_custom_minimum_size(Size2(18 * EDSCALE, 18 * EDSCALE));
	btn_slots_help->set_tooltip_text(CG_DB_TR("Ranuras de hojas de tiles (32x32 Anexo A4):\nA1: Agua / Animados\nA2: Suelo / Autotiles\nA3: Edificios / Fachadas\nA4: Muros / Techos\nA5: Suelo y muros normales\nB-E: Objetos, vegetación y decoración"));
	slots_hdr_hb->add_child(btn_slots_help);
	tileset_slots_box->add_child(slots_hdr_hb);

	for (int s = 0; s < TILESET_SLOT_COUNT; s++) {
		HBoxContainer *sl_row = memnew(HBoxContainer);
		sl_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sl_row->add_theme_constant_override("separation", 2);
		tileset_slots_box->add_child(sl_row);

		Label *sl_lbl = memnew(Label(_get_slot_key(s)));
		sl_lbl->set_custom_minimum_size(Size2(24 * EDSCALE, 0));
		sl_lbl->set_tooltip_text(_get_slot_label(s));
		sl_row->add_child(sl_lbl);

		tileset_slot_edits[s] = memnew(LineEdit);
		tileset_slot_edits[s]->set_editable(false);
		tileset_slot_edits[s]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		tileset_slot_edits[s]->set_custom_minimum_size(Size2(36 * EDSCALE, 20 * EDSCALE));
		tileset_slot_edits[s]->set_text(CG_DB_TR("(Vacío)"));
		sl_row->add_child(tileset_slot_edits[s]);

		btn_load_slot[s] = memnew(Button);
		if (has_theme_icon(SNAME("Load"), SNAME("EditorIcons"))) {
			btn_load_slot[s]->set_button_icon(get_editor_theme_icon(SNAME("Load")));
		} else {
			btn_load_slot[s]->set_text("+");
		}
		btn_load_slot[s]->set_tooltip_text(vformat(CG_DB_TR("Cargar hoja PNG 32x32 en la ranura %s (%s)"), _get_slot_key(s), _get_slot_label(s)));
		btn_load_slot[s]->set_custom_minimum_size(Size2(22 * EDSCALE, 20 * EDSCALE));
		btn_load_slot[s]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_load_slot_button_pressed).bind(s));
		sl_row->add_child(btn_load_slot[s]);

		btn_clear_slot[s] = memnew(Button);
		if (has_theme_icon(SNAME("Close"), SNAME("EditorIcons"))) {
			btn_clear_slot[s]->set_button_icon(get_editor_theme_icon(SNAME("Close")));
		} else {
			btn_clear_slot[s]->set_text("-");
		}
		btn_clear_slot[s]->set_tooltip_text(vformat(CG_DB_TR("Quitar la hoja asignada a la ranura %s"), _get_slot_key(s)));
		btn_clear_slot[s]->set_custom_minimum_size(Size2(22 * EDSCALE, 20 * EDSCALE));
		btn_clear_slot[s]->set_disabled(true);
		btn_clear_slot[s]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_clear_slot_button_pressed).bind(s));
		sl_row->add_child(btn_clear_slot[s]);
	}

	// Sección 5.3: 3 Ranuras de Botín (Drops) para Enemigos
	enemy_drops_box = memnew(VBoxContainer);
	enemy_drops_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_drops_box->add_theme_constant_override("separation", 6);
	enemy_drops_box->hide();
	col2_box->add_child(enemy_drops_box);

	enemy_drops_box->add_child(memnew(HSeparator));
	enemy_drops_box->add_child(memnew(Label(CG_DB_TR("Botín del Enemigo (3 Ranuras de Drop):"))));

	for (int s = 0; s < 3; s++) {
		VBoxContainer *dr_card = memnew(VBoxContainer);
		dr_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dr_card->add_theme_constant_override("separation", 2);
		enemy_drops_box->add_child(dr_card);

		HBoxContainer *dr_row1 = memnew(HBoxContainer);
		dr_row1->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dr_row1->add_theme_constant_override("separation", 3);
		dr_card->add_child(dr_row1);

		Label *dr_lbl = memnew(Label(vformat(CG_DB_TR("Drop %d:"), s + 1)));
		dr_lbl->set_custom_minimum_size(Size2(50 * EDSCALE, 0));
		dr_row1->add_child(dr_lbl);

		drop_kind_opt[s] = memnew(OptionButton);
		drop_kind_opt[s]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		drop_kind_opt[s]->set_clip_text(true);
		drop_kind_opt[s]->add_item(CG_DB_TR("Ninguno"), 0);
		drop_kind_opt[s]->add_item(CG_DB_TR("Objeto"), CAT_ITEMS);
		drop_kind_opt[s]->add_item(CG_DB_TR("Arma"), CAT_WEAPONS);
		drop_kind_opt[s]->add_item(CG_DB_TR("Armadura"), CAT_ARMORS);
		drop_kind_opt[s]->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_drop_kind_changed).bind(s));
		dr_row1->add_child(drop_kind_opt[s]);

		drop_prob_spin[s] = memnew(SpinBox);
		drop_prob_spin[s]->set_custom_minimum_size(Size2(58 * EDSCALE, 0));
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
		dr_row1->add_child(drop_prob_spin[s]);

		HBoxContainer *dr_row2 = memnew(HBoxContainer);
		dr_row2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dr_card->add_child(dr_row2);

		drop_item_opt[s] = memnew(OptionButton);
		drop_item_opt[s]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		drop_item_opt[s]->set_clip_text(true);
		drop_item_opt[s]->add_item(CG_DB_TR("(ninguno)"), 0);
		drop_item_opt[s]->set_disabled(true);
		drop_item_opt[s]->connect("item_selected", callable_mp_static(+[](int p_i, ClockGearsDatabaseDialog *p_self, int p_sl) {
			if (p_self) {
				p_self->_on_drop_field_changed(p_sl);
			}
		}).bind(this, s));
		dr_row2->add_child(drop_item_opt[s]);
	}

	// Sección 5.3: Patrones de Acción de IA (Tree + Diálogo)
	enemy_actions_box = memnew(VBoxContainer);
	enemy_actions_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemy_actions_box->add_theme_constant_override("separation", 6);
	enemy_actions_box->hide();
	col2_box->add_child(enemy_actions_box);

	enemy_actions_box->add_child(memnew(HSeparator));
	enemy_actions_box->add_child(memnew(Label(CG_DB_TR("Patrones de Acción (IA del Enemigo):"))));

	enemy_actions_tree = memnew(Tree);
	enemy_actions_tree->set_columns(4);
	enemy_actions_tree->set_column_titles_visible(true);
	enemy_actions_tree->set_column_title(0, CG_DB_TR("Habilidad"));
	enemy_actions_tree->set_column_title(1, CG_DB_TR("Condición"));
	enemy_actions_tree->set_column_title(2, CG_DB_TR("Parám."));
	enemy_actions_tree->set_column_title(3, CG_DB_TR("Prioridad"));
	enemy_actions_tree->set_hide_root(true);
	enemy_actions_tree->set_custom_minimum_size(Size2(0, 120 * EDSCALE));
	enemy_actions_box->add_child(enemy_actions_tree);

	HBoxContainer *ap_btn_row = memnew(HBoxContainer);
	ap_btn_row->add_theme_constant_override("separation", 6);
	enemy_actions_box->add_child(ap_btn_row);

	btn_add_action_pattern = memnew(Button(CG_DB_TR("+ Añadir Patrón de Acción")));
	btn_add_action_pattern->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_add_action_pattern->set_clip_text(true);
	btn_add_action_pattern->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_action_pattern_pressed));
	ap_btn_row->add_child(btn_add_action_pattern);

	btn_remove_action_pattern = memnew(Button(CG_DB_TR("- Eliminar Patrón")));
	btn_remove_action_pattern->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_remove_action_pattern->set_clip_text(true);
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

	// System Config Panel (only shown when CAT_SYSTEM is selected)
	system_panel = memnew(VBoxContainer);
	system_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	system_panel->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	system_panel->add_theme_constant_override("separation", 12);
	system_panel->hide();
	col2_box->add_child(system_panel);

	GridContainer *sys_grid = memnew(GridContainer);
	sys_grid->set_columns(2);
	sys_grid->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	sys_grid->add_theme_constant_override("h_separation", 16);
	sys_grid->add_theme_constant_override("v_separation", 10);
	system_panel->add_child(sys_grid);

	Label *lbl_sys_title = memnew(Label(CG_DB_TR("Título del Proyecto RPG:")));
	lbl_sys_title->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_title->set_clip_text(false);
	lbl_sys_title->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_title);
	sys_game_title_edit = memnew(LineEdit);
	sys_game_title_edit->set_custom_minimum_size(Size2(360 * EDSCALE, 0));
	sys_game_title_edit->set_h_size_flags(Control::SIZE_FILL);
	sys_game_title_edit->set_editable(true);
	sys_game_title_edit->set_focus_mode(Control::FOCUS_ALL);
	sys_game_title_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	sys_game_title_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_text_changed));
	sys_grid->add_child(sys_game_title_edit);

	Label *lbl_sys_curr = memnew(Label(CG_DB_TR("Unidad Monetaria:")));
	lbl_sys_curr->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_curr->set_clip_text(false);
	lbl_sys_curr->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_curr);
	sys_currency_edit = memnew(LineEdit);
	sys_currency_edit->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
	sys_currency_edit->set_h_size_flags(Control::SIZE_FILL);
	sys_currency_edit->set_text("G");
	sys_currency_edit->set_editable(true);
	sys_currency_edit->set_focus_mode(Control::FOCUS_ALL);
	sys_currency_edit->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	sys_currency_edit->connect("text_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_text_changed));
	sys_grid->add_child(sys_currency_edit);

	Label *lbl_sys_combat = memnew(Label(CG_DB_TR("Sistema de Combate:")));
	lbl_sys_combat->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_combat->set_clip_text(false);
	lbl_sys_combat->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_combat);
	combat_mode_selector = memnew(OptionButton);
	combat_mode_selector->set_custom_minimum_size(Size2(360 * EDSCALE, 0));
	combat_mode_selector->set_h_size_flags(Control::SIZE_FILL);
	combat_mode_selector->set_clip_text(false);
	combat_mode_selector->add_item(CG_DB_TR("Combate por Turnos Clásico (Turn-Based)"), 0);
	combat_mode_selector->add_item(CG_DB_TR("Combate en Tiempo Real (Action RPG)"), 1);
	combat_mode_selector->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_combat_mode_selected));
	sys_grid->add_child(combat_mode_selector);

	Label *lbl_sys_map = memnew(Label(CG_DB_TR("Mapa Inicial:")));
	lbl_sys_map->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_map->set_clip_text(false);
	lbl_sys_map->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_map);
	sys_start_map_option = memnew(OptionButton);
	sys_start_map_option->set_custom_minimum_size(Size2(360 * EDSCALE, 0));
	sys_start_map_option->set_h_size_flags(Control::SIZE_FILL);
	sys_start_map_option->set_clip_text(false);
	sys_start_map_option->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_start_map_selected));
	sys_grid->add_child(sys_start_map_option);

	Label *lbl_sys_x = memnew(Label(CG_DB_TR("Posición Inicial X (Celda):")));
	lbl_sys_x->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_x->set_clip_text(false);
	lbl_sys_x->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_x);
	sys_start_x_spin = memnew(SpinBox);
	sys_start_x_spin->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
	sys_start_x_spin->set_h_size_flags(Control::SIZE_FILL);
	sys_start_x_spin->set_max(999);
	sys_start_x_spin->set_editable(true);
	_bind_numeric_spinbox(sys_start_x_spin);
	sys_start_x_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_spin_changed));
	sys_grid->add_child(sys_start_x_spin);

	Label *lbl_sys_y = memnew(Label(CG_DB_TR("Posición Inicial Y (Celda):")));
	lbl_sys_y->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_y->set_clip_text(false);
	lbl_sys_y->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_y);
	sys_start_y_spin = memnew(SpinBox);
	sys_start_y_spin->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
	sys_start_y_spin->set_h_size_flags(Control::SIZE_FILL);
	sys_start_y_spin->set_max(999);
	sys_start_y_spin->set_editable(true);
	_bind_numeric_spinbox(sys_start_y_spin);
	sys_start_y_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_spin_changed));
	sys_grid->add_child(sys_start_y_spin);

	Label *lbl_sys_zoom = memnew(Label(CG_DB_TR("Zoom de Cámara (Pixel Art):")));
	lbl_sys_zoom->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_zoom->set_clip_text(false);
	lbl_sys_zoom->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_zoom);
	sys_camera_zoom_opt = memnew(OptionButton);
	sys_camera_zoom_opt->set_custom_minimum_size(Size2(360 * EDSCALE, 0));
	sys_camera_zoom_opt->set_h_size_flags(Control::SIZE_FILL);
	sys_camera_zoom_opt->add_item(CG_DB_TR("1x (Resolución Nativa 1:1)"), 1);
	sys_camera_zoom_opt->add_item(CG_DB_TR("2x (Pixel Art Estándar)"), 2);
	sys_camera_zoom_opt->add_item(CG_DB_TR("3x (Pixel Art Aumentado)"), 3);
	sys_camera_zoom_opt->add_item(CG_DB_TR("4x (Máximo Aumento)"), 4);
	sys_camera_zoom_opt->select(1);
	sys_camera_zoom_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	sys_grid->add_child(sys_camera_zoom_opt);

	Label *lbl_sys_cam_opts = memnew(Label(CG_DB_TR("Comportamiento de Cámara:")));
	lbl_sys_cam_opts->set_custom_minimum_size(Size2(170 * EDSCALE, 0));
	lbl_sys_cam_opts->set_clip_text(false);
	lbl_sys_cam_opts->set_h_size_flags(Control::SIZE_FILL);
	sys_grid->add_child(lbl_sys_cam_opts);

	VBoxContainer *cam_flags_vb = memnew(VBoxContainer);
	cam_flags_vb->set_h_size_flags(Control::SIZE_FILL);
	sys_camera_smooth_check = memnew(CheckBox(CG_DB_TR("Seguimiento Suave (Interpolación fluida al jugador)")));
	sys_camera_smooth_check->set_pressed(true);
	sys_camera_smooth_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	cam_flags_vb->add_child(sys_camera_smooth_check);

	sys_camera_clamp_check = memnew(CheckBox(CG_DB_TR("Restringir Cámara a los Límites del Mapa")));
	sys_camera_clamp_check->set_pressed(true);
	sys_camera_clamp_check->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_system_field_changed).unbind(1));
	cam_flags_vb->add_child(sys_camera_clamp_check);
	sys_grid->add_child(cam_flags_vb);

	system_panel->add_child(memnew(HSeparator));
	system_panel->add_child(memnew(Label(CG_DB_TR("Grupo Inicial (Party - Máx. 1 Líder + 8 Seguidores):"))));

	sys_party_list = memnew(ItemList);
	sys_party_list->set_custom_minimum_size(Size2(540 * EDSCALE, 120 * EDSCALE));
	system_panel->add_child(sys_party_list);

	HBoxContainer *party_ctrl_row = memnew(HBoxContainer);
	party_ctrl_row->add_theme_constant_override("separation", 6);
	system_panel->add_child(party_ctrl_row);

	sys_party_actor_opt = memnew(OptionButton);
	sys_party_actor_opt->set_custom_minimum_size(Size2(260 * EDSCALE, 0));
	sys_party_actor_opt->set_clip_text(false);
	party_ctrl_row->add_child(sys_party_actor_opt);

	btn_add_party_member = memnew(Button(CG_DB_TR("+ Añadir al Grupo")));
	btn_add_party_member->set_clip_text(false);
	btn_add_party_member->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_party_member_pressed));
	party_ctrl_row->add_child(btn_add_party_member);

	btn_remove_party_member = memnew(Button(CG_DB_TR("- Quitar")));
	btn_remove_party_member->set_clip_text(false);
	btn_remove_party_member->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_party_member_pressed));
	party_ctrl_row->add_child(btn_remove_party_member);

	// --- COLUMN 3: Graphics OR Tileset Sheet & Collision Editor inside ScrollContainer (stretch 0.40) ---
	ScrollContainer *col3_scroll = memnew(ScrollContainer);
	col3_scroll_container = col3_scroll;
	col3_scroll->set_custom_minimum_size(Size2(180 * EDSCALE, 0));
	col3_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col3_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col3_scroll->set_stretch_ratio(0.40f);
	col3_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	col3_scroll->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	right_hbox->add_child(col3_scroll);

	col3_box = memnew(VBoxContainer);
	col3_box->set_custom_minimum_size(Size2(0, 0));
	col3_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col3_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col3_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	col3_box->add_theme_constant_override("separation", 6);
	col3_scroll->add_child(col3_box);

	col3_title = memnew(Label(CG_DB_TR("Gráfico / Sprite / Icono")));
	col3_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col3_title->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	col3_box->add_child(col3_title);
	col3_box->add_child(memnew(HSeparator));

	// Standard Sprite / Icon Box
	standard_graphic_box = memnew(VBoxContainer);
	standard_graphic_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	standard_graphic_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	standard_graphic_box->add_theme_constant_override("separation", 6);
	col3_box->add_child(standard_graphic_box);

	CenterContainer *graphic_center = memnew(CenterContainer);
	graphic_center->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	standard_graphic_box->add_child(graphic_center);

	graphic_frame = memnew(PanelContainer);
	graphic_frame->set_custom_minimum_size(Size2(96 * EDSCALE, 96 * EDSCALE));
	Ref<StyleBoxFlat> dark_frame = memnew(StyleBoxFlat);
	dark_frame->set_anti_aliased(false);
	dark_frame->set_bg_color(Color(0.08f, 0.09f, 0.11f, 1.0f));
	dark_frame->set_border_width_all(1);
	dark_frame->set_border_color(Color(0.28f, 0.31f, 0.37f, 1.0f));
	dark_frame->set_content_margin_all(4);
	graphic_frame->add_theme_style_override("panel", dark_frame);
	graphic_center->add_child(graphic_frame);

	graphic_preview = memnew(TextureRect);
	graphic_preview->set_custom_minimum_size(Size2(88 * EDSCALE, 88 * EDSCALE));
	graphic_preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	graphic_preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	graphic_preview->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST); // A4.5: Vista previa en NEAREST
	graphic_preview->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	graphic_preview->set_tooltip_text(CG_DB_TR("Doble clic para abrir el explorador y asignar Sprite o Icono (96x96)"));
	graphic_preview->connect("gui_input", callable_mp(this, &ClockGearsDatabaseDialog::_on_graphic_gui_input));
	graphic_frame->add_child(graphic_preview);

	graphic_path_label = memnew(Label(CG_DB_TR("(Sin gráfico)")));
	graphic_path_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	graphic_path_label->set_clip_text(true);
	graphic_path_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	graphic_path_label->set_tooltip_text(CG_DB_TR("Ruta del gráfico asignado. Doble clic para asignar."));
	standard_graphic_box->add_child(graphic_path_label);

	btn_select_graphic = memnew(Button(CG_DB_TR("Seleccionar Gráfico...")));
	btn_select_graphic->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_select_graphic->set_clip_text(true);
	btn_select_graphic->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_select_graphic_pressed));
	standard_graphic_box->add_child(btn_select_graphic);

	btn_pick_tile_graphic = memnew(Button(CG_DB_TR("TilePicker (32x32)...")));
	btn_pick_tile_graphic->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_pick_tile_graphic->set_clip_text(true);
	btn_pick_tile_graphic->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_pick_tile_graphic_pressed));
	standard_graphic_box->add_child(btn_pick_tile_graphic);

	tile_picker_dialog = memnew(ClockGearsTilePickerDialog);
	add_child(tile_picker_dialog);

	// Sección B: Editor de Sprites y Animaciones (Actores y Enemigos - 4 Bloques con Scroll)
	anim_editor_scroll = memnew(ScrollContainer);
	anim_editor_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_editor_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	anim_editor_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	anim_editor_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	anim_editor_scroll->hide();
	col3_box->add_child(anim_editor_scroll);

	sprite_anim_editor_box = memnew(VBoxContainer);
	sprite_anim_editor_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sprite_anim_editor_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	sprite_anim_editor_box->add_theme_constant_override("separation", 8);
	anim_editor_scroll->add_child(sprite_anim_editor_box);

	// ==========================================
	// BLOQUE 1: GESTIÓN DE ANIMACIONES (Lista estilo Godot + desplegable sincronizado)
	// ==========================================
	Label *lbl_b1 = memnew(Label(CG_DB_TR("1. Animaciones:")));
	lbl_b1->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	sprite_anim_editor_box->add_child(lbl_b1);

	HBoxContainer *track_ctrl_hb = memnew(HBoxContainer);
	track_ctrl_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	track_ctrl_hb->add_theme_constant_override("separation", 4);
	sprite_anim_editor_box->add_child(track_ctrl_hb);

	anim_track_opt = memnew(OptionButton);
	anim_track_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_track_opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_track_selected));
	track_ctrl_hb->add_child(anim_track_opt);

	btn_add_anim = memnew(Button(CG_DB_TR("+ Nueva")));
	btn_add_anim->set_tooltip_text(CG_DB_TR("Añadir una nueva animación vacía"));
	btn_add_anim->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_anim_pressed));
	track_ctrl_hb->add_child(btn_add_anim);

	btn_rename_anim = memnew(Button(CG_DB_TR("✏")));
	btn_rename_anim->set_tooltip_text(CG_DB_TR("Renombrar animación (o doble clic en la lista)"));
	btn_rename_anim->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_rename_anim_pressed));
	track_ctrl_hb->add_child(btn_rename_anim);

	btn_remove_anim = memnew(Button("-"));
	btn_remove_anim->set_tooltip_text(CG_DB_TR("Eliminar animación seleccionada"));
	btn_remove_anim->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_anim_pressed));
	track_ctrl_hb->add_child(btn_remove_anim);

	// Lista de animaciones (estilo Godot AnimatedSprite2D SpriteFrames panel)
	anim_item_list = memnew(ItemList);
	anim_item_list->set_custom_minimum_size(Size2(0, 80 * EDSCALE));
	anim_item_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	anim_item_list->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_item_selected));
	anim_item_list->connect("item_activated", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_item_activated)); // Doble clic para renombrar
	sprite_anim_editor_box->add_child(anim_item_list);

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

	// ==========================================
	// BLOQUE 2: AJUSTES DE REPRODUCCIÓN (FPS y Loop propios por animación)
	// ==========================================
	sprite_anim_editor_box->add_child(memnew(HSeparator));
	HBoxContainer *settings_row = memnew(HBoxContainer);
	settings_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	settings_row->add_theme_constant_override("separation", 6);
	sprite_anim_editor_box->add_child(settings_row);

	Label *lbl_fps = memnew(Label(CG_DB_TR("FPS:")));
	settings_row->add_child(lbl_fps);

	anim_fps_spin = memnew(SpinBox);
	anim_fps_spin->set_min(1);
	anim_fps_spin->set_max(60);
	anim_fps_spin->set_value(6);
	_bind_numeric_spinbox(anim_fps_spin);
	anim_fps_spin->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_playback_settings_changed).unbind(1));
	settings_row->add_child(anim_fps_spin);

	anim_loop_chk = memnew(CheckBox(CG_DB_TR("Bucle (Loop)")));
	anim_loop_chk->set_pressed(true);
	anim_loop_chk->connect("toggled", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_playback_settings_changed).unbind(1));
	settings_row->add_child(anim_loop_chk);

	btn_anim_play = memnew(Button(CG_DB_TR("▶ Probar")));
	btn_anim_play->set_custom_minimum_size(Size2(75 * EDSCALE, 0));
	btn_anim_play->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_anim_play_pressed));
	settings_row->add_child(btn_anim_play);

	// ==========================================
	// BLOQUE 3: FRAMES (LÍNEA DE TIEMPO)
	// ==========================================
	sprite_anim_editor_box->add_child(memnew(HSeparator));
	HBoxContainer *frames_bar = memnew(HBoxContainer);
	frames_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frames_bar->add_theme_constant_override("separation", 6);
	sprite_anim_editor_box->add_child(frames_bar);

	Label *lbl_b3 = memnew(Label(CG_DB_TR("Frames:")));
	lbl_b3->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	frames_bar->add_child(lbl_b3);

	btn_open_sheet_importer = memnew(Button(CG_DB_TR("🎞 Añadir frames desde spritesheet...")));
	btn_open_sheet_importer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_open_sheet_importer->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_open_sheet_importer_pressed));
	frames_bar->add_child(btn_open_sheet_importer);

	frames_strip_scroll = memnew(ScrollContainer);
	frames_strip_scroll->set_custom_minimum_size(Size2(0, 92 * EDSCALE));
	frames_strip_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frames_strip_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	frames_strip_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	sprite_anim_editor_box->add_child(frames_strip_scroll);

	frames_strip_hbox = memnew(HBoxContainer);
	frames_strip_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frames_strip_hbox->add_theme_constant_override("separation", 6);
	frames_strip_scroll->add_child(frames_strip_hbox);

	// ==========================================
	// BLOQUE 4: VISTA PREVIA
	// ==========================================
	sprite_anim_editor_box->add_child(memnew(HSeparator));
	HBoxContainer *prev_row = memnew(HBoxContainer);
	prev_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_row->add_theme_constant_override("separation", 8);
	sprite_anim_editor_box->add_child(prev_row);

	Label *lbl_prev = memnew(Label(CG_DB_TR("Vista previa:")));
	lbl_prev->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.3f));
	prev_row->add_child(lbl_prev);

	CenterContainer *prev_center = memnew(CenterContainer);
	prev_center->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prev_row->add_child(prev_center);

	PanelContainer *prev_panel = memnew(PanelContainer);
	prev_panel->set_custom_minimum_size(Size2(80 * EDSCALE, 80 * EDSCALE));
	Ref<StyleBoxFlat> prev_sb;
	prev_sb.instantiate();
	prev_sb->set_bg_color(Color(0.12f, 0.14f, 0.18f, 0.95f));
	prev_sb->set_border_width_all(1);
	prev_sb->set_border_color(Color(0.3f, 0.35f, 0.45f, 0.8f));
	prev_sb->set_corner_radius_all(4);
	prev_panel->add_theme_style_override("panel", prev_sb);
	prev_center->add_child(prev_panel);

	anim_preview_rect = memnew(TextureRect);
	anim_preview_rect->set_custom_minimum_size(Size2(72 * EDSCALE, 72 * EDSCALE));
	anim_preview_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	anim_preview_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	anim_preview_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	anim_preview_atlas.instantiate();
	anim_preview_rect->set_texture(anim_preview_atlas);
	prev_panel->add_child(anim_preview_rect);

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

	// Sección 4.2 / 5.1 / 5.3: Editor de Rasgos (Traits) en Columna 3 inferior
	traits_box = memnew(VBoxContainer);
	traits_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	traits_box->add_theme_constant_override("separation", 6);
	col3_box->add_child(traits_box);

	traits_box->add_child(memnew(HSeparator));
	traits_box->add_child(memnew(Label(CG_DB_TR("Rasgos (Traits):"))));

	traits_tree = memnew(Tree);
	traits_tree->set_columns(3);
	traits_tree->set_column_titles_visible(true);
	traits_tree->set_column_title(0, CG_DB_TR("Tipo"));
	traits_tree->set_column_title(1, CG_DB_TR("Dato"));
	traits_tree->set_column_title(2, CG_DB_TR("Valor"));
	traits_tree->set_hide_root(true);
	traits_tree->set_custom_minimum_size(Size2(0, 125 * EDSCALE));
	traits_box->add_child(traits_tree);

	HBoxContainer *tr_btn_row = memnew(HBoxContainer);
	tr_btn_row->add_theme_constant_override("separation", 6);
	traits_box->add_child(tr_btn_row);

	btn_add_trait = memnew(Button(CG_DB_TR("+ Rasgo")));
	btn_add_trait->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_add_trait->set_clip_text(true);
	btn_add_trait->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_trait_pressed));
	tr_btn_row->add_child(btn_add_trait);

	btn_remove_trait = memnew(Button(CG_DB_TR("- Quitar")));
	btn_remove_trait->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_remove_trait->set_clip_text(true);
	btn_remove_trait->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_trait_pressed));
	tr_btn_row->add_child(btn_remove_trait);

	trait_dialog = memnew(AcceptDialog);
	trait_dialog->set_title(CG_DB_TR("Añadir Rasgo (Trait)"));
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

	// Sección 4.3 / 5.2: Editor de Efectos (Effects) en Columna 3 inferior
	effects_box = memnew(VBoxContainer);
	effects_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	effects_box->add_theme_constant_override("separation", 6);
	effects_box->hide();
	col3_box->add_child(effects_box);

	effects_box->add_child(memnew(HSeparator));
	effects_box->add_child(memnew(Label(CG_DB_TR("Efectos de Uso (Effects):"))));

	effects_tree = memnew(Tree);
	effects_tree->set_columns(3);
	effects_tree->set_column_titles_visible(true);
	effects_tree->set_column_title(0, CG_DB_TR("Efecto"));
	effects_tree->set_column_title(1, CG_DB_TR("Dato"));
	effects_tree->set_column_title(2, CG_DB_TR("Val 1 / Val 2"));
	effects_tree->set_hide_root(true);
	effects_tree->set_custom_minimum_size(Size2(0, 125 * EDSCALE));
	effects_box->add_child(effects_tree);

	HBoxContainer *ef_btn_row = memnew(HBoxContainer);
	ef_btn_row->add_theme_constant_override("separation", 6);
	effects_box->add_child(ef_btn_row);

	btn_add_effect = memnew(Button(CG_DB_TR("+ Efecto")));
	btn_add_effect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_add_effect->set_clip_text(true);
	btn_add_effect->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_add_effect_pressed));
	ef_btn_row->add_child(btn_add_effect);

	btn_remove_effect = memnew(Button(CG_DB_TR("- Quitar")));
	btn_remove_effect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_remove_effect->set_clip_text(true);
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

	// Tileset Collision Box (shown when CAT_TILESETS is active)
	tileset_collision_box = memnew(VBoxContainer);
	tileset_collision_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_collision_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_collision_box->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	tileset_collision_box->add_theme_constant_override("separation", 5);
	tileset_collision_box->hide();
	col3_box->add_child(tileset_collision_box);

	// Pestañas principales de hojas: A | B | C | D | E
	tileset_group_tabs_box = memnew(HBoxContainer);
	tileset_group_tabs_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_group_tabs_box->add_theme_constant_override("separation", 3);
	tileset_collision_box->add_child(tileset_group_tabs_box);

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
		btn_ts_group_tab[g]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		btn_ts_group_tab[g]->set_clip_text(true);
		btn_ts_group_tab[g]->set_tooltip_text(CG_DB_TR(group_tips[g]));
		btn_ts_group_tab[g]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_group_tab_pressed).bind(g));
		tileset_group_tabs_box->add_child(btn_ts_group_tab[g]);
	}

	// Sub-pestañas cuando A está activa: A1 | A2 | A3 | A4 | A5
	tileset_a_subtabs_box = memnew(HBoxContainer);
	tileset_a_subtabs_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_a_subtabs_box->add_theme_constant_override("separation", 3);
	tileset_collision_box->add_child(tileset_a_subtabs_box);

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
		btn_ts_a_subtab[a]->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		btn_ts_a_subtab[a]->set_clip_text(true);
		btn_ts_a_subtab[a]->set_tooltip_text(CG_DB_TR(sub_tips[a]));
		btn_ts_a_subtab[a]->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_a_subtab_pressed).bind(a));
		tileset_a_subtabs_box->add_child(btn_ts_a_subtab[a]);
	}

	HBoxContainer *slot_status_row = memnew(HBoxContainer);
	slot_status_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	slot_status_row->add_theme_constant_override("separation", 4);
	tileset_collision_box->add_child(slot_status_row);

	active_slot_info_label = memnew(Label(CG_DB_TR("Ranura: A5 (32x32)")));
	active_slot_info_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	active_slot_info_label->set_clip_text(false);
	active_slot_info_label->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	slot_status_row->add_child(active_slot_info_label);

	btn_load_tileset_sheet = memnew(Button(CG_DB_TR("Cargar Hoja...")));
	btn_load_tileset_sheet->set_tooltip_text(CG_DB_TR("Cargar archivo PNG 32x32 para la ranura activa"));
	btn_load_tileset_sheet->set_custom_minimum_size(Size2(80 * EDSCALE, 20 * EDSCALE));
	btn_load_tileset_sheet->set_clip_text(false);
	btn_load_tileset_sheet->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_load_tileset_sheet_pressed));
	slot_status_row->add_child(btn_load_tileset_sheet);

	Button *btn_picker_help = memnew(Button("?"));
	btn_picker_help->set_custom_minimum_size(Size2(20 * EDSCALE, 20 * EDSCALE));
	btn_picker_help->set_tooltip_text(CG_DB_TR("Clic en cada tile para alternar pasabilidad:\n○ (Círculo) = Pasable (Suelo)\n× (Cruz) = Bloqueado (Colisión)\n★ (Estrella) = Sobre personaje (Techo/Pasa detrás)\n4-Dir = Pasabilidad por flechas (↓ ← → ↑)\nTerr = Etiqueta de Terreno (0 a 7)\nArb = Arbusto (~)\nMost = Mostrador (♦)\nEsc = Escalera (H)\nDaño = Daño de suelo (▲)"));
	slot_status_row->add_child(btn_picker_help);

	db_tile_picker = memnew(ClockGearsTilePicker);
	db_tile_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	db_tile_picker->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	db_tile_picker->set_custom_minimum_size(Size2(240 * EDSCALE, 220 * EDSCALE));
	db_tile_picker->set_metadata_editing_enabled(true);
	db_tile_picker->set_edit_mode(ClockGearsTilePicker::MODE_PASSAGE_BASIC);
	db_tile_picker->set_on_metadata_changed(callable_mp(this, &ClockGearsDatabaseDialog::_on_tileset_collision_changed));
	db_tile_picker->set_on_request_load(callable_mp(this, &ClockGearsDatabaseDialog::_on_load_tileset_sheet_pressed));
	tileset_collision_box->add_child(db_tile_picker);

	tileset_scroll = memnew(ScrollContainer);
	tileset_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
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

	_on_category_selected(CAT_ACTORS);
}

void ClockGearsDatabaseDialog::open_database() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	_load_database_from_disk();
	_refresh_category_view();

	// PROBLEMA A: Restaurar el tamaño recordado por la ventana si existe; de lo contrario, 90% de la ventana padre
	Size2i target_size(1160, 680);
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("clockgears/editor/database_window_size")) {
		Variant saved_sz = ProjectSettings::get_singleton()->get_setting("clockgears/editor/database_window_size");
		if (saved_sz.get_type() == Variant::VECTOR2I || saved_sz.get_type() == Variant::VECTOR2) {
			target_size = Size2i(saved_sz);
		}
	} else {
		Size2i avail_size(1280, 720);
		Window *parent_win = get_parent_visible_window();
		if (parent_win) {
			avail_size = parent_win->get_size();
		}
		target_size = Size2i(int(avail_size.x * 0.90f), int(avail_size.y * 0.90f));
	}
	target_size = target_size.max(get_min_size());
	popup_centered(target_size);

	if (name_edit && name_edit->is_visible_in_tree()) {
		name_edit->grab_focus();
	}
}

void ClockGearsDatabaseDialog::_save_database_to_disk() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	for (int i = 0; i < CAT_MAX; i++) {
		dm->set_category_array(i, category_data[i]);
	}
	dm->set_system_config(system_config);
	dm->save_all("res://clockgears_data");
}

void ClockGearsDatabaseDialog::_load_database_from_disk() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	dm->load_all("res://clockgears_data");
	for (int i = 0; i < CAT_MAX; i++) {
		category_data[i] = dm->get_category_array(i);
	}
	system_config = dm->get_system_config();
	current_combat_mode = (int)system_config.get("combat_mode", COMBAT_MODE_TURN_BASED);
}

// ============================================================================
// Métodos de Tropas (Fase 8A)
// ============================================================================

void ClockGearsDatabaseDialog::_refresh_troop_members_ui() {
	if (!troop_members_list_vbox) return;
	while (troop_members_list_vbox->get_child_count() > 0) {
		Node *c = troop_members_list_vbox->get_child(0);
		troop_members_list_vbox->remove_child(c);
		memdelete(c);
	}
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());

	for (int i = 0; i < mems.size(); i++) {
		Dictionary m = mems[i];
		int enemy_id = int(m.get("enemy_id", 1));
		int mx = int(m.get("x", 200 + i * 110));
		int my = int(m.get("y", 140));

		HBoxContainer *row = memnew(HBoxContainer);
		row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row->add_theme_constant_override("separation", 4);

		Label *lbl_n = memnew(Label(vformat("#%d", i + 1)));
		lbl_n->set_custom_minimum_size(Size2(24 * EDSCALE, 0));
		row->add_child(lbl_n);

		OptionButton *opt = memnew(OptionButton);
		opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_populate_db_records_option(opt, CAT_ENEMIES, CG_DB_TR("(ninguno)"), false);
		_select_option_by_id_or_index(opt, enemy_id);
		opt->connect("item_selected", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_member_enemy_changed).bind(i));
		row->add_child(opt);

		Label *lbl_x = memnew(Label("X:"));
		row->add_child(lbl_x);

		SpinBox *sp_x = memnew(SpinBox);
		sp_x->set_min(0);
		sp_x->set_max(1920);
		sp_x->set_value(mx);
		_bind_numeric_spinbox(sp_x);
		sp_x->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_member_x_changed).bind(i));
		row->add_child(sp_x);

		Label *lbl_y = memnew(Label("Y:"));
		row->add_child(lbl_y);

		SpinBox *sp_y = memnew(SpinBox);
		sp_y->set_min(0);
		sp_y->set_max(1080);
		sp_y->set_value(my);
		_bind_numeric_spinbox(sp_y);
		sp_y->connect("value_changed", callable_mp(this, &ClockGearsDatabaseDialog::_on_troop_member_y_changed).bind(i));
		row->add_child(sp_y);

		Button *btn_del = memnew(Button("-"));
		btn_del->set_tooltip_text(CG_DB_TR("Eliminar este enemigo de la tropa"));
		btn_del->connect("pressed", callable_mp(this, &ClockGearsDatabaseDialog::_on_remove_troop_member_pressed).bind(i));
		row->add_child(btn_del);

		troop_members_list_vbox->add_child(row);
	}
}

void ClockGearsDatabaseDialog::_on_add_troop_member_pressed() {
	Vector<int> sel = record_list ? record_list->get_selected_items() : Vector<int>();
	if (sel.is_empty()) return;
	int idx = sel[0];
	Array &arr = category_data[CAT_TROOPS];
	if (idx < 0 || idx >= arr.size()) return;

	Dictionary d = arr[idx];
	Array mems = d.get("members", Array());

	int enemy_id = 1;
	const Array &enemies = category_data[CAT_ENEMIES];
	if (!enemies.is_empty()) {
		Dictionary e0 = enemies[0];
		enemy_id = int(e0.get("id", 1));
	}

	Dictionary new_m;
	new_m["enemy_id"] = enemy_id;
	new_m["x"] = 180 + mems.size() * 120;
	new_m["y"] = 140;
	mems.push_back(new_m);

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
		d["members"] = mems;
		arr[idx] = d;
		_refresh_troop_members_ui();
		_on_param_changed();
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
		_on_param_changed();
	}
}

// ============================================================================
// Métodos del Editor de Sprites y Animaciones (Fase 8A)
// ============================================================================

// ============================================================================
// Métodos del Editor de Sprites y Animaciones (SpriteFrames Style - Fase 8A)
// ============================================================================

String ClockGearsDatabaseDialog::_get_active_anim_name() const {
	if (anim_track_opt && anim_track_opt->get_selected() >= 0 && anim_track_opt->get_selected() < anim_track_opt->get_item_count()) {
		return anim_track_opt->get_item_text(anim_track_opt->get_selected());
	}
	if (anim_item_list) {
		Vector<int> sel = anim_item_list->get_selected_items();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < anim_item_list->get_item_count()) {
			return anim_item_list->get_item_text(sel[0]);
		}
	}
	return "";
}

void ClockGearsDatabaseDialog::_refresh_anim_track_options() {
	if (!anim_track_opt) {
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
	for (const KeyValue<String, ClockGearsSpriteAnimation> &E : current_sprite_data.animations) {
		anim_track_opt->add_item(E.key, idx);
		if (anim_item_list) {
			anim_item_list->add_item(E.key);
		}
		if (E.key == prev_sel || (prev_sel.is_empty() && E.key.to_lower() == "idle")) {
			sel_idx = idx;
		}
		idx++;
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
		_refresh_frames_timeline();
		_refresh_anim_preview();
	}
}

void ClockGearsDatabaseDialog::_on_anim_track_selected(int p_idx) {
	if (updating_ui) return;
	if (!anim_track_opt || p_idx < 0 || p_idx >= anim_track_opt->get_item_count()) {
		return;
	}
	String anim_name = anim_track_opt->get_item_text(p_idx);
	if (!current_sprite_data.animations.has(anim_name)) {
		return;
	}

	updating_ui = true;
	const ClockGearsSpriteAnimation &a = current_sprite_data.animations[anim_name];
	if (anim_fps_spin) anim_fps_spin->set_value(a.fps > 0 ? a.fps : 6.0f);
	if (anim_loop_chk) anim_loop_chk->set_pressed(a.loop);
	if (anim_item_list) {
		for (int i = 0; i < anim_item_list->get_item_count(); i++) {
			if (anim_item_list->get_item_text(i) == anim_name) {
				anim_item_list->select(i);
				break;
			}
		}
	}
	anim_preview_playing = false;
	anim_preview_timer = 0.0f;
	if (btn_anim_play) {
		btn_anim_play->set_text(CG_DB_TR("▶ Probar"));
	}
	updating_ui = false;

	_refresh_frames_timeline();
	_refresh_anim_preview();
}

void ClockGearsDatabaseDialog::_on_anim_item_selected(int p_idx) {
	if (updating_ui) return;
	if (!anim_item_list || p_idx < 0 || p_idx >= anim_item_list->get_item_count()) {
		return;
	}
	String anim_name = anim_item_list->get_item_text(p_idx);
	if (anim_track_opt) {
		for (int i = 0; i < anim_track_opt->get_item_count(); i++) {
			if (anim_track_opt->get_item_text(i) == anim_name) {
				anim_track_opt->select(i);
				break;
			}
		}
		_on_anim_track_selected(anim_track_opt->get_selected());
	}
}

void ClockGearsDatabaseDialog::_on_anim_item_activated(int p_idx) {
	if (!anim_item_list || p_idx < 0 || p_idx >= anim_item_list->get_item_count()) return;
	String old_name = anim_item_list->get_item_text(p_idx);
	if (rename_anim_dialog && rename_anim_name_edit) {
		rename_anim_name_edit->set_text(old_name);
		rename_anim_name_edit->set_meta("old_anim_name", old_name);
		rename_anim_dialog->popup_centered();
		rename_anim_name_edit->grab_focus();
		rename_anim_name_edit->select_all();
	}
}

void ClockGearsDatabaseDialog::_on_add_anim_pressed() {
	if (new_anim_dialog) {
		if (new_anim_name_edit) new_anim_name_edit->set_text("");
		new_anim_dialog->popup_centered();
	}
}

void ClockGearsDatabaseDialog::_on_new_anim_dialog_confirmed() {
	String anim_name = new_anim_name_edit ? new_anim_name_edit->get_text().strip_edges() : "";
	if (anim_name.is_empty()) {
		int c = 1;
		anim_name = vformat("anim_%d", c);
		while (current_sprite_data.animations.has(anim_name)) {
			c++;
			anim_name = vformat("anim_%d", c);
		}
	}
	ClockGearsSpriteAnimation a;
	a.name = anim_name;
	a.fps = 6.0f;
	a.loop = true;
	a.frames_data.clear();
	a.frames.clear();
	current_sprite_data.animations[anim_name] = a;

	_refresh_anim_track_options();
	for (int i = 0; i < anim_track_opt->get_item_count(); i++) {
		if (anim_track_opt->get_item_text(i) == anim_name) {
			anim_track_opt->select(i);
			_on_anim_track_selected(i);
			break;
		}
	}
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_rename_anim_pressed() {
	String cur_name = _get_active_anim_name();
	if (cur_name.is_empty()) return;
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

	if (current_sprite_data.animations.has(old_name)) {
		ClockGearsSpriteAnimation a = current_sprite_data.animations[old_name];
		a.name = new_name;
		current_sprite_data.animations.erase(old_name);
		current_sprite_data.animations[new_name] = a;

		_refresh_anim_track_options();
		for (int i = 0; i < anim_track_opt->get_item_count(); i++) {
			if (anim_track_opt->get_item_text(i) == new_name) {
				anim_track_opt->select(i);
				_on_anim_track_selected(i);
				break;
			}
		}
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_remove_anim_pressed() {
	String name_to_remove = _get_active_anim_name();
	if (name_to_remove.is_empty() || !current_sprite_data.animations.has(name_to_remove)) {
		return;
	}
	current_sprite_data.animations.erase(name_to_remove);
	_refresh_anim_track_options();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_open_sheet_importer_pressed() {
	if (!sheet_importer_dialog) return;

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

	callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
	callable_mp(this, &ClockGearsDatabaseDialog::_refresh_anim_preview).call_deferred();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_refresh_frames_timeline() {
	if (!frames_strip_hbox) return;
	while (frames_strip_hbox->get_child_count() > 0) {
		Node *c = frames_strip_hbox->get_child(0);
		frames_strip_hbox->remove_child(c);
		c->queue_free();
	}

	String cur_anim_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_anim_name)) {
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

	for (int s = 0; s < total_frames; s++) {
		ClockGearsAnimationFrame frame = a.get_frame(s, 0, 0);
		PanelContainer *pnl = memnew(PanelContainer);
		pnl->set_custom_minimum_size(Size2(54 * EDSCALE, 72 * EDSCALE));

		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_bg_color(Color(0.14f, 0.16f, 0.22f, 0.95f));
		sb->set_border_width_all(1);
		sb->set_border_color(Color(0.35f, 0.4f, 0.55f, 0.8f));
		sb->set_corner_radius_all(4);
		pnl->add_theme_style_override("panel", sb);

		VBoxContainer *vb = memnew(VBoxContainer);
		vb->add_theme_constant_override("separation", 2);
		pnl->add_child(vb);

		Label *lbl = memnew(Label(vformat("#%d", s + 1)));
		lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		lbl->add_theme_font_size_override("font_size", 9);
		vb->add_child(lbl);

		TextureRect *tr = memnew(TextureRect);
		tr->set_custom_minimum_size(Size2(36 * EDSCALE, 36 * EDSCALE));
		tr->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		tr->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		tr->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);

		Ref<Texture2D> tex = _load_texture_from_path(frame.sheet_path);
		if (tex.is_valid()) {
			Ref<AtlasTexture> at;
			at.instantiate();
			at->set_atlas(tex);
			at->set_region(Rect2(frame.region));
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

		frames_strip_hbox->add_child(pnl);
	}
}

void ClockGearsDatabaseDialog::_refresh_anim_preview() {
	String cur_name = _get_active_anim_name();
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
	ClockGearsAnimationFrame f0 = a.get_frame(0, 0, 0);
	Ref<Texture2D> tex = _load_texture_from_path(f0.sheet_path);
	if (tex.is_valid()) {
		if (anim_preview_atlas.is_null()) anim_preview_atlas.instantiate();
		anim_preview_atlas->set_atlas(tex);
		anim_preview_atlas->set_region(Rect2(f0.region));
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
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_frames_timeline).call_deferred();
		callable_mp(this, &ClockGearsDatabaseDialog::_refresh_anim_preview).call_deferred();
		_on_param_changed();
	}
}

void ClockGearsDatabaseDialog::_on_anim_playback_settings_changed() {
	if (updating_ui) return;
	String cur_anim_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_anim_name)) return;
	ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_anim_name];
	if (anim_fps_spin) a.fps = float(anim_fps_spin->get_value());
	if (anim_loop_chk) a.loop = anim_loop_chk->is_pressed();
	_on_param_changed();
}

void ClockGearsDatabaseDialog::_on_anim_play_pressed() {
	anim_preview_playing = !anim_preview_playing;
	anim_preview_timer = 0.0f;
	if (btn_anim_play) {
		btn_anim_play->set_text(anim_preview_playing ? CG_DB_TR("⏹ Detener") : CG_DB_TR("▶ Probar"));
	}
}

void ClockGearsDatabaseDialog::_update_anim_playback(float p_delta) {
	if (!anim_preview_playing) return;
	String cur_name = _get_active_anim_name();
	if (!current_sprite_data.animations.has(cur_name)) return;
	const ClockGearsSpriteAnimation &a = current_sprite_data.animations[cur_name];
	int total = a.get_frame_count();
	if (total == 0) return;

	float speed = MAX(0.5f, a.fps);
	float frame_duration = 1.0f / speed;
	anim_preview_timer += p_delta;
	int step = int(anim_preview_timer / frame_duration);

	if (a.loop) {
		step = step % total;
	} else if (step >= total) {
		step = total - 1;
		anim_preview_playing = false;
		if (btn_anim_play) {
			btn_anim_play->set_text(CG_DB_TR("▶ Probar"));
		}
	}

	ClockGearsAnimationFrame f = a.get_frame(step, 0, 0);
	Ref<Texture2D> tex = _load_texture_from_path(f.sheet_path);
	if (tex.is_valid()) {
		if (anim_preview_atlas.is_null()) anim_preview_atlas.instantiate();
		if (anim_preview_atlas->get_atlas() != tex) {
			anim_preview_atlas->set_atlas(tex);
		}
		anim_preview_atlas->set_region(Rect2(f.region));
		if (anim_preview_rect) {
			anim_preview_rect->set_texture(anim_preview_atlas);
		}
	}
}
